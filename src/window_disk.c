#include "window_disk.h"
#include "variables_utils.h"
#include "config.h"

static WindowDiskData *g_window_disk = NULL;

/* ── helpers ────────────────────────────────────────────────────────────── */

/* Lee el valor de una variable del archivo variables.sh.
 * Retorna cadena recién allocada o NULL si no existe. */
static gchar *disk_read_var(const gchar *name)
{
    gchar *content = NULL;
    if (!g_file_get_contents(VARIABLES_FILE_PATH, &content, NULL, NULL))
        return NULL;

    gchar **lines = g_strsplit(content, "\n", -1);
    g_free(content);

    gsize needle_len = strlen(name);
    gchar *result = NULL;

    for (int i = 0; lines[i]; i++) {
        if (g_str_has_prefix(lines[i], name) && lines[i][needle_len] == '=') {
            const gchar *val = lines[i] + needle_len + 1;
            if (val[0] == '"') {
                gchar *tmp = g_strdup(val + 1);
                gsize len = strlen(tmp);
                if (len > 0 && tmp[len - 1] == '"')
                    tmp[len - 1] = '\0';
                result = tmp;
            } else {
                result = g_strdup(val);
            }
            break;
        }
    }

    g_strfreev(lines);
    return result;
}

/* Obtiene el tamaño del disco en bytes usando lsblk */
static guint64 disk_get_size_bytes(const gchar *disk_path)
{
    if (!disk_path || disk_path[0] == '\0') return 0;

    gchar *command = g_strdup_printf("lsblk -b -d -n -o SIZE %s", disk_path);
    gchar *output = NULL;
    guint64 result = 0;

    if (g_spawn_command_line_sync(command, &output, NULL, NULL, NULL) && output) {
        g_strstrip(output);
        result = g_ascii_strtoull(output, NULL, 10);
        g_free(output);
    }

    g_free(command);
    return result;
}

static void update_root_size_labels(WindowDiskData *data, int value)
{
    gchar *root_text = g_strdup_printf("%d GB", value);
    if (data->root_size_label)
        gtk_label_set_text(data->root_size_label, root_text);
    if (data->root_size_suffix_label)
        gtk_label_set_text(data->root_size_suffix_label, root_text);
    g_free(root_text);

    int total = (data->disk_total_gb > 0) ? (int)data->disk_total_gb : 30;
    int home_val = total - value;
    if (home_val < 1) home_val = 1;
    gchar *home_text = g_strdup_printf("%d GB", home_val);
    if (data->home_size_label)
        gtk_label_set_text(data->home_size_label, home_text);
    g_free(home_text);
}

static void update_filesystem_home_constraints(WindowDiskData *data)
{
    gboolean is_btrfs = data->btrfs_radio &&
                        gtk_check_button_get_active(data->btrfs_radio);

    if (is_btrfs) {
        /* BTRFS: no, subvolumen y partición disponibles */
        if (data->home_no_radio)
            gtk_widget_set_sensitive(GTK_WIDGET(data->home_no_radio), TRUE);
        if (data->home_subvolume_radio)
            gtk_widget_set_sensitive(GTK_WIDGET(data->home_subvolume_radio), TRUE);
        if (data->home_partition_radio)
            gtk_widget_set_sensitive(GTK_WIDGET(data->home_partition_radio), TRUE);
    } else {
        /* EXT4 / XFS: deshabilitar subvolumen, habilitar No y partición */
        if (data->home_subvolume_radio && gtk_check_button_get_active(data->home_subvolume_radio))
            gtk_check_button_set_active(data->home_no_radio, TRUE);
        if (data->home_subvolume_radio)
            gtk_widget_set_sensitive(GTK_WIDGET(data->home_subvolume_radio), FALSE);
        if (data->home_no_radio)
            gtk_widget_set_sensitive(GTK_WIDGET(data->home_no_radio), TRUE);
        if (data->home_partition_radio)
            gtk_widget_set_sensitive(GTK_WIDGET(data->home_partition_radio), TRUE);
    }
}

static void update_swap_custom_sensitivity(WindowDiskData *data)
{
    gboolean active = data->swap_custom_radio &&
                      gtk_check_button_get_active(data->swap_custom_radio);
    if (data->swap_decrease_button)
        gtk_widget_set_sensitive(GTK_WIDGET(data->swap_decrease_button), active);
    if (data->swap_increase_button)
        gtk_widget_set_sensitive(GTK_WIDGET(data->swap_increase_button), active);
    if (data->swap_size_label)
        gtk_widget_set_sensitive(GTK_WIDGET(data->swap_size_label), active);
}

/* Calcula disk_total_gb desde SELECTED_DISK y actualiza el suffix del expander.
 * Se llama siempre al abrir la ventana, independientemente del modo home. */
static void window_disk_refresh_disk_size(WindowDiskData *data)
{
    gchar *disk_path = disk_read_var("SELECTED_DISK");
    guint64 size_bytes = disk_path ? disk_get_size_bytes(disk_path) : 0;
    g_free(disk_path);

    guint total_gb = (size_bytes > 0) ? (guint)(size_bytes / (1024ULL * 1024 * 1024)) : 60;
    if (total_gb < 2) total_gb = 2;
    data->disk_total_gb = total_gb;

    /* Mostrar mitad del disco en el suffix del expander como valor de referencia */
    if (data->root_size_suffix_label) {
        gchar *text = g_strdup_printf("%u GB", total_gb / 2);
        gtk_label_set_text(data->root_size_suffix_label, text);
        g_free(text);
    }
}

static void update_home_partition_sensitivity(WindowDiskData *data)
{
    gboolean partition = data->home_partition_radio &&
                         gtk_check_button_get_active(data->home_partition_radio);

    if (!data->root_size_expander) return;

    if (!partition) {
        /* Colapsar y deshabilitar el expander al volver a No/subvolumen */
        adw_expander_row_set_expanded(data->root_size_expander, FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->root_size_expander), FALSE);
        return;
    }

    /* Activar expander — usa disk_total_gb ya calculado por refresh_disk_size */
    gtk_widget_set_sensitive(GTK_WIDGET(data->root_size_expander), TRUE);

    guint total_gb = (data->disk_total_gb > 0) ? data->disk_total_gb : 60;
    guint half = total_gb / 2;

    if (data->root_size_scale) {
        GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(data->root_size_scale));

        if (total_gb <= 30) {
            /* Disco ≤ 30 GB: mostrar slider fijo en la mitad, sin poder moverlo */
            gtk_adjustment_set_lower(adj, 1.0);
            gtk_adjustment_set_upper(adj, (gdouble)(total_gb - 1));
            gtk_range_set_value(GTK_RANGE(data->root_size_scale), (gdouble)half);
            gtk_widget_set_sensitive(GTK_WIDGET(data->root_size_scale), FALSE);
        } else {
            /* Disco > 30 GB: mínimo 30 GB para raíz, máximo total-1 GB */
            guint default_root = MAX(30u, half);
            gtk_adjustment_set_lower(adj, 30.0);
            gtk_adjustment_set_upper(adj, (gdouble)(total_gb - 1));
            gtk_range_set_value(GTK_RANGE(data->root_size_scale), (gdouble)default_root);
            gtk_widget_set_sensitive(GTK_WIDGET(data->root_size_scale), TRUE);
        }

        update_root_size_labels(data,
            (int)gtk_range_get_value(GTK_RANGE(data->root_size_scale)));
    }
}

/* ── init variables ─────────────────────────────────────────────────────── */

static void apply_disk_defaults(GString *content, gpointer user_data)
{
    (void)user_data;
    /* Anclas en cascada para que queden en orden debajo de PARTITION_MODE */
    vars_upsert_after(content, "FILESYSTEM_TYPE",  "ext4",  "PARTITION_MODE");
    vars_upsert_after(content, "HOME_PARTITION",   "no",    "FILESYSTEM_TYPE");
    vars_upsert_after(content, "ROOT_SIZE",        "15",    "HOME_PARTITION");
    vars_upsert_after(content, "SWAP_TYPE",        "zram",  "ROOT_SIZE");
    vars_upsert_after(content, "SWAP_CUSTOM_SIZE", "1",     "SWAP_TYPE");
}

void window_disk_init_variables(void)
{
    if (vars_update(apply_disk_defaults, NULL))
        LOG_INFO("window_disk: variables por defecto escritas en variables.sh");
    else
        LOG_ERROR("window_disk: error al escribir variables por defecto");
}

/* ── allocación ─────────────────────────────────────────────────────────── */

WindowDiskData *window_disk_new(void)
{
    return g_malloc0(sizeof(WindowDiskData));
}

/* ── carga de widgets ────────────────────────────────────────────────────── */

void window_disk_load_widgets(WindowDiskData *data)
{
    data->window       = GTK_WINDOW(gtk_builder_get_object(data->builder, "DiskConfigWindow"));
    data->close_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button  = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));

    /* Sistema de archivos */
    data->ext4_radio  = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "ext4_radio"));
    data->btrfs_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "btrfs_radio"));
    data->xfs_radio   = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "xfs_radio"));

    /* Directorio personal */
    data->home_no_radio          = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "home_no_radio"));
    data->home_subvolume_radio   = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "home_subvolume_radio"));
    data->home_partition_radio   = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "home_partition_radio"));
    data->root_size_expander     = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "root_size_expander"));
    data->root_size_scale        = GTK_SCALE(gtk_builder_get_object(data->builder, "root_size_scale"));
    data->root_size_label        = GTK_LABEL(gtk_builder_get_object(data->builder, "root_size_label"));
    data->root_size_suffix_label = GTK_LABEL(gtk_builder_get_object(data->builder, "root_size_suffix_label"));
    data->home_size_label        = GTK_LABEL(gtk_builder_get_object(data->builder, "home_size_label"));

    /* Swap */
    data->swap_none_radio      = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_none_radio"));
    data->swap_half_radio      = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_half_radio"));
    data->swap_equal_radio     = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_equal_radio"));
    data->swap_custom_radio    = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_custom_radio"));
    data->swap_decrease_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "swap_decrease_button"));
    data->swap_increase_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "swap_increase_button"));
    data->swap_size_label      = GTK_LABEL(gtk_builder_get_object(data->builder, "swap_size_label"));
}

/* ── conexión de señales ─────────────────────────────────────────────────── */

void window_disk_connect_signals(WindowDiskData *data)
{
    if (data->close_button)
        g_signal_connect(data->close_button, "clicked",
                         G_CALLBACK(on_disk_close_button_clicked), data);
    if (data->save_button)
        g_signal_connect(data->save_button, "clicked",
                         G_CALLBACK(on_disk_save_button_clicked), data);

    /* Filesystem — aplica restricciones de home según el tipo */
    if (data->ext4_radio)
        g_signal_connect(data->ext4_radio, "toggled",
                         G_CALLBACK(on_disk_filesystem_radio_toggled), data);
    if (data->btrfs_radio)
        g_signal_connect(data->btrfs_radio, "toggled",
                         G_CALLBACK(on_disk_filesystem_radio_toggled), data);
    if (data->xfs_radio)
        g_signal_connect(data->xfs_radio, "toggled",
                         G_CALLBACK(on_disk_filesystem_radio_toggled), data);

    /* Home — actualiza sensibilidad del expander */
    if (data->home_no_radio)
        g_signal_connect(data->home_no_radio, "toggled",
                         G_CALLBACK(on_disk_home_radio_toggled), data);
    if (data->home_subvolume_radio)
        g_signal_connect(data->home_subvolume_radio, "toggled",
                         G_CALLBACK(on_disk_home_radio_toggled), data);
    if (data->home_partition_radio)
        g_signal_connect(data->home_partition_radio, "toggled",
                         G_CALLBACK(on_disk_home_radio_toggled), data);

    /* Scale de tamaño raíz */
    if (data->root_size_scale)
        g_signal_connect(GTK_RANGE(data->root_size_scale), "value-changed",
                         G_CALLBACK(on_disk_root_scale_value_changed), data);

    /* Swap — guardar y actualizar sensibilidad en cualquier cambio */
    if (data->swap_none_radio)
        g_signal_connect(data->swap_none_radio, "toggled",
                         G_CALLBACK(on_disk_swap_radio_toggled), data);
    if (data->swap_half_radio)
        g_signal_connect(data->swap_half_radio, "toggled",
                         G_CALLBACK(on_disk_swap_radio_toggled), data);
    if (data->swap_equal_radio)
        g_signal_connect(data->swap_equal_radio, "toggled",
                         G_CALLBACK(on_disk_swap_radio_toggled), data);
    if (data->swap_custom_radio)
        g_signal_connect(data->swap_custom_radio, "toggled",
                         G_CALLBACK(on_disk_swap_radio_toggled), data);

    if (data->swap_decrease_button)
        g_signal_connect(data->swap_decrease_button, "clicked",
                         G_CALLBACK(on_disk_swap_decrease_clicked), data);
    if (data->swap_increase_button)
        g_signal_connect(data->swap_increase_button, "clicked",
                         G_CALLBACK(on_disk_swap_increase_clicked), data);
}

/* ── inicialización ──────────────────────────────────────────────────────── */

void window_disk_init(WindowDiskData *data)
{
    data->builder = gtk_builder_new_from_resource("/org/gtk/arcris/window_disk.ui");
    if (!data->builder) {
        LOG_ERROR("window_disk: no se pudo cargar window_disk.ui");
        return;
    }

    window_disk_load_widgets(data);

    if (!data->window) {
        LOG_ERROR("window_disk: no se pudo obtener DiskConfigWindow");
        return;
    }

    window_disk_connect_signals(data);

    data->is_initialized = TRUE;
    g_window_disk = data;

    LOG_INFO("window_disk inicializado correctamente");
}

/* ── carga desde variables.sh ────────────────────────────────────────────── */

void window_disk_load_from_variables(WindowDiskData *data)
{
    if (!data || !data->is_initialized) return;

    /* Sistema de archivos */
    gchar *fs = disk_read_var("FILESYSTEM_TYPE");
    if (fs) {
        if (g_strcmp0(fs, "btrfs") == 0 && data->btrfs_radio)
            gtk_check_button_set_active(data->btrfs_radio, TRUE);
        else if (g_strcmp0(fs, "xfs") == 0 && data->xfs_radio)
            gtk_check_button_set_active(data->xfs_radio, TRUE);
        else if (data->ext4_radio)
            gtk_check_button_set_active(data->ext4_radio, TRUE);
        g_free(fs);
    }

    /* Aplicar restricciones de home según filesystem cargado */
    update_filesystem_home_constraints(data);

    /* Directorio personal — el signal toggled dispara update_home_partition_sensitivity
     * que ya calcula el tamaño del disco y configura el slider correctamente */
    gchar *home = disk_read_var("HOME_PARTITION");
    gboolean loaded_partition = FALSE;
    if (home) {
        if (g_strcmp0(home, "subvolume") == 0 && data->home_subvolume_radio)
            gtk_check_button_set_active(data->home_subvolume_radio, TRUE);
        else if (g_strcmp0(home, "partition") == 0 && data->home_partition_radio) {
            gtk_check_button_set_active(data->home_partition_radio, TRUE);
            loaded_partition = TRUE;
        } else if (data->home_no_radio)
            gtk_check_button_set_active(data->home_no_radio, TRUE);
        g_free(home);
    }

    /* Tamaño raíz — solo aplicar si el modo es partición, disk_total_gb ya fue
     * calculado por el signal, y el valor guardado cabe en el rango real del disco */
    if (loaded_partition && data->root_size_scale && data->disk_total_gb > 0) {
        gchar *root_size = disk_read_var("ROOT_SIZE");
        if (root_size) {
            int val = atoi(root_size);
            int min_root = (data->disk_total_gb <= 30) ? (int)(data->disk_total_gb / 2) : 30;
            int max_root = (int)(data->disk_total_gb - 1);
            if (val >= min_root && val <= max_root) {
                gtk_range_set_value(GTK_RANGE(data->root_size_scale), (gdouble)val);
                update_root_size_labels(data, val);
            }
            g_free(root_size);
        }
    }

    /* Tipo de swap */
    gchar *swap = disk_read_var("SWAP_TYPE");
    if (swap) {
        if (g_strcmp0(swap, "half") == 0 && data->swap_half_radio)
            gtk_check_button_set_active(data->swap_half_radio, TRUE);
        else if (g_strcmp0(swap, "equal") == 0 && data->swap_equal_radio)
            gtk_check_button_set_active(data->swap_equal_radio, TRUE);
        else if (g_strcmp0(swap, "custom") == 0 && data->swap_custom_radio)
            gtk_check_button_set_active(data->swap_custom_radio, TRUE);
        else if (data->swap_none_radio)
            gtk_check_button_set_active(data->swap_none_radio, TRUE);
        g_free(swap);
    }

    /* Tamaño swap personalizado */
    gchar *swap_custom = disk_read_var("SWAP_CUSTOM_SIZE");
    if (swap_custom && data->swap_size_label) {
        int val = atoi(swap_custom);
        if (val < 1) val = 1;
        if (val > 32) val = 32;
        gchar *text = g_strdup_printf("%d GB", val);
        gtk_label_set_text(data->swap_size_label, text);
        g_free(text);
        g_free(swap_custom);
    }

    update_home_partition_sensitivity(data);
    update_swap_custom_sensitivity(data);
}

/* ── guardar en variables.sh ─────────────────────────────────────────────── */

typedef struct {
    const gchar *filesystem;
    const gchar *home;
    gchar        root_size_buf[8];
    const gchar *swap;
    const gchar *swap_custom;
} DiskSaveCtx;

static void apply_disk_save(GString *content, gpointer user_data)
{
    DiskSaveCtx *ctx = user_data;
    vars_upsert(content, "FILESYSTEM_TYPE",  ctx->filesystem);
    vars_upsert(content, "HOME_PARTITION",   ctx->home);
    vars_upsert(content, "ROOT_SIZE",        ctx->root_size_buf);
    vars_upsert(content, "SWAP_TYPE",        ctx->swap);
    vars_upsert(content, "SWAP_CUSTOM_SIZE", ctx->swap_custom);
}

gboolean window_disk_save_to_variables(WindowDiskData *data)
{
    if (!data) return FALSE;

    DiskSaveCtx ctx;

    /* Filesystem */
    if (data->btrfs_radio && gtk_check_button_get_active(data->btrfs_radio))
        ctx.filesystem = "btrfs";
    else if (data->xfs_radio && gtk_check_button_get_active(data->xfs_radio))
        ctx.filesystem = "xfs";
    else
        ctx.filesystem = "ext4";

    /* Home partition */
    if (data->home_subvolume_radio && gtk_check_button_get_active(data->home_subvolume_radio))
        ctx.home = "subvolume";
    else if (data->home_partition_radio && gtk_check_button_get_active(data->home_partition_radio))
        ctx.home = "partition";
    else
        ctx.home = "no";

    /* Root size */
    int root_val = 15;
    if (data->root_size_scale)
        root_val = (int)gtk_range_get_value(GTK_RANGE(data->root_size_scale));
    g_snprintf(ctx.root_size_buf, sizeof(ctx.root_size_buf), "%d", root_val);

    /* Swap */
    if (data->swap_half_radio && gtk_check_button_get_active(data->swap_half_radio))
        ctx.swap = "half";
    else if (data->swap_equal_radio && gtk_check_button_get_active(data->swap_equal_radio))
        ctx.swap = "equal";
    else if (data->swap_custom_radio && gtk_check_button_get_active(data->swap_custom_radio))
        ctx.swap = "custom";
    else
        ctx.swap = "zram";

    /* Swap custom size — guardar solo el número, sin " GB" */
    static gchar swap_custom_buf[8];
    int swap_val = 1;
    if (data->swap_size_label)
        swap_val = atoi(gtk_label_get_text(data->swap_size_label));
    if (swap_val < 1) swap_val = 1;
    if (swap_val > 32) swap_val = 32;
    g_snprintf(swap_custom_buf, sizeof(swap_custom_buf), "%d", swap_val);
    ctx.swap_custom = swap_custom_buf;

    gboolean ok = vars_update(apply_disk_save, &ctx);

    if (ok)
        LOG_INFO("window_disk: guardado — fs=%s home=%s root=%s swap=%s swap_custom=%s",
                 ctx.filesystem, ctx.home, ctx.root_size_buf, ctx.swap, ctx.swap_custom);
    else
        LOG_ERROR("window_disk: error al guardar configuración");

    return ok;
}

/* ── mostrar ventana ─────────────────────────────────────────────────────── */

void window_disk_show(WindowDiskData *data, GtkWindow *parent)
{
    if (!data || !data->window) return;

    /* Leer tamaño del disco seleccionado antes de cargar el estado guardado */
    window_disk_refresh_disk_size(data);
    window_disk_load_from_variables(data);

    if (parent)
        gtk_window_set_transient_for(data->window, parent);

    gtk_window_present(data->window);
    LOG_INFO("window_disk mostrada");
}

WindowDiskData *window_disk_get_instance(void)
{
    return g_window_disk;
}

/* ── callbacks ───────────────────────────────────────────────────────────── */

void on_disk_close_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    WindowDiskData *data = user_data;
    if (data && data->window)
        gtk_window_close(data->window);
}

void on_disk_save_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    WindowDiskData *data = user_data;
    if (!data) return;

    window_disk_save_to_variables(data);
    if (data->window)
        gtk_window_close(data->window);
}

void on_disk_filesystem_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    (void)radio;
    WindowDiskData *data = user_data;
    update_filesystem_home_constraints(data);
    window_disk_save_to_variables(data);
}

void on_disk_home_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    (void)radio;
    WindowDiskData *data = user_data;
    update_home_partition_sensitivity(data);
    window_disk_save_to_variables(data);
}

void on_disk_swap_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    (void)radio;
    WindowDiskData *data = user_data;
    update_swap_custom_sensitivity(data);
    window_disk_save_to_variables(data);
}

void on_disk_root_scale_value_changed(GtkRange *range, gpointer user_data)
{
    WindowDiskData *data = user_data;
    update_root_size_labels(data, (int)gtk_range_get_value(range));
    window_disk_save_to_variables(data);
}

void on_disk_swap_decrease_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    WindowDiskData *data = user_data;
    if (!data || !data->swap_size_label) return;

    int val = atoi(gtk_label_get_text(data->swap_size_label));
    if (val > 1) val--;
    gchar text[12];
    g_snprintf(text, sizeof(text), "%d GB", val);
    gtk_label_set_text(data->swap_size_label, text);
    window_disk_save_to_variables(data);
}

void on_disk_swap_increase_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    WindowDiskData *data = user_data;
    if (!data || !data->swap_size_label) return;

    int val = atoi(gtk_label_get_text(data->swap_size_label));
    if (val < 32) val++;
    gchar text[12];
    g_snprintf(text, sizeof(text), "%d GB", val);
    gtk_label_set_text(data->swap_size_label, text);
    window_disk_save_to_variables(data);
}
