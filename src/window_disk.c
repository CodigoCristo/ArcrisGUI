#include "window_disk.h"
#include "variables_utils.h"
#include "config.h"
#include "i18n.h"

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

static void update_encryption_sensitivity(WindowDiskData *data);       /* forward decl */
static gboolean encryption_is_valid(WindowDiskData *data);             /* forward decl */
static void encryption_disable_switch(WindowDiskData *data);           /* forward decl */
static void on_encryption_close_dialog_response(AdwAlertDialog *dialog,
                                                const gchar    *response,
                                                gpointer        user_data); /* forward decl */
static void show_encryption_close_dialog(WindowDiskData *data);            /* forward decl */

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
    vars_upsert_after(content, "ENCRYPTION",     "false", "SWAP_CUSTOM_SIZE");
    vars_upsert_after(content, "ENCRYPTION_KEY", "",      "ENCRYPTION");
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

    /* Cifrado */
    data->encryption_switch         = GTK_SWITCH(gtk_builder_get_object(data->builder, "encryption_switch"));
    data->encryption_expander       = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "encryption_expander"));
    data->encryption_password_entry = ADW_ENTRY_ROW(gtk_builder_get_object(data->builder, "encryption_password_entry"));
    data->encryption_confirm_entry  = ADW_ENTRY_ROW(gtk_builder_get_object(data->builder, "encryption_confirm_entry"));
    data->encryption_error_row      = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "encryption_error_row"));

    /* Swap */
    data->swap_none_radio      = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_none_radio"));
    data->swap_half_radio      = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_half_radio"));
    data->swap_equal_radio     = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_equal_radio"));
    data->swap_custom_radio    = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_custom_radio"));
    data->swap_disabled_radio  = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "swap_disabled_radio"));
    data->swap_decrease_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "swap_decrease_button"));
    data->swap_increase_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "swap_increase_button"));
    data->swap_size_label      = GTK_LABEL(gtk_builder_get_object(data->builder, "swap_size_label"));

    /* Widgets para traducción */
    data->title_widget       = ADW_WINDOW_TITLE(gtk_builder_get_object(data->builder, "disk_window_title"));
    data->filesystem_group   = ADW_PREFERENCES_GROUP(gtk_builder_get_object(data->builder, "filesystem_group"));
    data->home_group         = ADW_PREFERENCES_GROUP(gtk_builder_get_object(data->builder, "home_group"));
    data->swap_group         = ADW_PREFERENCES_GROUP(gtk_builder_get_object(data->builder, "swap_group"));
    data->home_subvolume_row = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "home_subvolume_row"));
    data->home_partition_row = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "home_partition_row"));
    data->slider_row         = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "slider_row"));
    data->root_label_row     = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "root_label_row"));
    data->home_label_row     = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "home_label_row"));
    data->swap_none_row      = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "swap_none_row"));
    data->swap_half_row      = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "swap_half_row"));
    data->swap_equal_row     = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "swap_equal_row"));
    data->swap_custom_row    = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "swap_custom_row"));
    data->swap_disabled_row   = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "swap_disabled_row"));
    data->encryption_group    = ADW_PREFERENCES_GROUP(gtk_builder_get_object(data->builder, "encryption_group"));
    data->encryption_toggle_row = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "encryption_toggle_row"));
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

    /* Interceptar cualquier intento de cerrar la ventana */
    if (data->window)
        g_signal_connect(data->window, "close-request",
                         G_CALLBACK(on_disk_window_close_request), data);

    /* Cifrado */
    if (data->encryption_switch)
        g_signal_connect(data->encryption_switch, "state-set",
                         G_CALLBACK(on_disk_encryption_switch_toggled), data);
    if (data->encryption_password_entry)
        g_signal_connect(data->encryption_password_entry, "changed",
                         G_CALLBACK(on_disk_encryption_password_changed), data);
    if (data->encryption_confirm_entry)
        g_signal_connect(data->encryption_confirm_entry, "changed",
                         G_CALLBACK(on_disk_encryption_confirm_changed), data);

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
    if (data->swap_disabled_radio)
        g_signal_connect(data->swap_disabled_radio, "toggled",
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
        else if (g_strcmp0(swap, "none") == 0 && data->swap_disabled_radio)
            gtk_check_button_set_active(data->swap_disabled_radio, TRUE);
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

    /* Cifrado */
    gchar *enc = disk_read_var("ENCRYPTION");
    gboolean enc_active = (g_strcmp0(enc, "true") == 0);
    g_free(enc);
    if (data->encryption_switch)
        gtk_switch_set_active(data->encryption_switch, enc_active);
    if (data->encryption_expander) {
        gtk_widget_set_sensitive(GTK_WIDGET(data->encryption_expander), enc_active);
        if (!enc_active)
            adw_expander_row_set_expanded(data->encryption_expander, FALSE);
    }
    if (enc_active) {
        gchar *key = disk_read_var("ENCRYPTION_KEY");
        if (key && data->encryption_password_entry) {
            gtk_editable_set_text(GTK_EDITABLE(data->encryption_password_entry), key);
            if (data->encryption_confirm_entry)
                gtk_editable_set_text(GTK_EDITABLE(data->encryption_confirm_entry), key);
        }
        g_free(key);
    }

    update_home_partition_sensitivity(data);
    update_swap_custom_sensitivity(data);
    update_encryption_sensitivity(data);
}

/* ── guardar en variables.sh ─────────────────────────────────────────────── */

typedef struct {
    const gchar *filesystem;
    const gchar *home;
    gchar        root_size_buf[8];
    const gchar *swap;
    const gchar *swap_custom;
    const gchar *encryption;
    const gchar *encryption_key;
} DiskSaveCtx;

static void apply_disk_save(GString *content, gpointer user_data)
{
    DiskSaveCtx *ctx = user_data;
    vars_upsert(content, "FILESYSTEM_TYPE",  ctx->filesystem);
    vars_upsert(content, "HOME_PARTITION",   ctx->home);
    vars_upsert(content, "ROOT_SIZE",        ctx->root_size_buf);
    vars_upsert(content, "SWAP_TYPE",        ctx->swap);
    vars_upsert(content, "SWAP_CUSTOM_SIZE", ctx->swap_custom);
    vars_upsert(content, "ENCRYPTION",       ctx->encryption);
    vars_upsert(content, "ENCRYPTION_KEY",   ctx->encryption_key);
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
    else if (data->swap_disabled_radio && gtk_check_button_get_active(data->swap_disabled_radio))
        ctx.swap = "none";
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

    /* Cifrado */
    if (data->encryption_switch && gtk_switch_get_active(data->encryption_switch)) {
        ctx.encryption = "true";
        ctx.encryption_key = (data->encryption_password_entry)
            ? gtk_editable_get_text(GTK_EDITABLE(data->encryption_password_entry))
            : "";
    } else {
        ctx.encryption = "false";
        ctx.encryption_key = "";
    }

    gboolean ok = vars_update(apply_disk_save, &ctx);

    if (ok)
        LOG_INFO("window_disk: guardado — fs=%s home=%s root=%s swap=%s swap_custom=%s encryption=%s",
                 ctx.filesystem, ctx.home, ctx.root_size_buf, ctx.swap, ctx.swap_custom, ctx.encryption);
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

static void disk_window_defocus_and_close(WindowDiskData *data)
{
    /* Fuerza que cualquier GtkText activo suelte el foco antes de cerrar
     * la ventana, evitando el warning "did not receive a focus-out event" */
    if (data->window)
        gtk_widget_grab_focus(GTK_WIDGET(data->window));
    gtk_window_close(data->window);
}

static void show_encryption_close_dialog(WindowDiskData *data)
{
    AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
        i18n_t("Cifrado sin contraseña válida",
               "Encryption without valid password",
               "Шифрование без корректного пароля"),
        i18n_t("El cifrado está activo pero la contraseña está vacía o no coincide. "
               "Si cierra, el cifrado se desactivará automáticamente.",
               "Encryption is active but the password is empty or does not match. "
               "If you close, encryption will be disabled automatically.",
               "Шифрование активно, но пароль пуст или не совпадает. "
               "При закрытии шифрование будет отключено автоматически.")
    ));
    adw_alert_dialog_add_response(dialog, "cancel",
        i18n_t("Cancelar", "Cancel", "Отмена"));
    adw_alert_dialog_add_response(dialog, "close",
        i18n_t("Cerrar de todas formas", "Close anyway", "Закрыть всё равно"));
    adw_alert_dialog_set_default_response(dialog, "cancel");
    adw_alert_dialog_set_response_appearance(dialog, "close", ADW_RESPONSE_DESTRUCTIVE);
    g_signal_connect(dialog, "response",
                     G_CALLBACK(on_encryption_close_dialog_response), data);
    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(data->window));
}

void on_disk_close_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    WindowDiskData *data = user_data;
    if (!data || !data->window) return;

    if (!encryption_is_valid(data)) {
        show_encryption_close_dialog(data);
        return;
    }

    disk_window_defocus_and_close(data);
}

gboolean on_disk_window_close_request(GtkWindow *window, gpointer user_data)
{
    (void)window;
    WindowDiskData *data = user_data;
    if (!data) return FALSE;

    if (!encryption_is_valid(data)) {
        show_encryption_close_dialog(data);
        return TRUE;  /* bloquea el cierre hasta que el diálogo decida */
    }

    return FALSE;  /* permite el cierre normal */
}

void on_disk_save_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    WindowDiskData *data = user_data;
    if (!data) return;

    if (!encryption_is_valid(data)) {
        const gchar *pass = data->encryption_password_entry
            ? gtk_editable_get_text(GTK_EDITABLE(data->encryption_password_entry)) : "";
        gboolean empty = (strlen(pass) == 0);

        AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
            i18n_t("Contraseña inválida",
                   "Invalid password",
                   "Неверный пароль"),
            empty
                ? i18n_t("El campo de contraseña está vacío. Ingresa una contraseña de 3 a 15 caracteres.",
                          "The password field is empty. Enter a password between 3 and 15 characters.",
                          "Поле пароля пусто. Введите пароль от 3 до 15 символов.")
                : i18n_t("Las contraseñas no coinciden. Asegúrate de escribir la misma contraseña en ambos campos.",
                          "The passwords do not match. Make sure you type the same password in both fields.",
                          "Пароли не совпадают. Убедитесь, что оба поля содержат одинаковый пароль.")
        ));
        adw_alert_dialog_add_response(dialog, "ok",
            i18n_t("Aceptar", "OK", "ОК"));
        adw_alert_dialog_set_default_response(dialog, "ok");
        adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(data->window));
        return;
    }

    window_disk_save_to_variables(data);
    disk_window_defocus_and_close(data);
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

/* ── Validación de cifrado ───────────────────────────────────────────────── */

/* Retorna TRUE si el cifrado no está activo, o si está activo con contraseña válida */
static gboolean encryption_is_valid(WindowDiskData *data)
{
    if (!data->encryption_switch || !gtk_switch_get_active(data->encryption_switch))
        return TRUE;

    const gchar *pass = data->encryption_password_entry
        ? gtk_editable_get_text(GTK_EDITABLE(data->encryption_password_entry)) : "";
    const gchar *confirm = data->encryption_confirm_entry
        ? gtk_editable_get_text(GTK_EDITABLE(data->encryption_confirm_entry)) : "";

    gsize len = strlen(pass);
    return (len >= 3 && len <= 15 && g_strcmp0(pass, confirm) == 0);
}

static void encryption_disable_switch(WindowDiskData *data)
{
    gtk_switch_set_active(data->encryption_switch, FALSE);
    adw_expander_row_set_expanded(data->encryption_expander, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(data->encryption_expander), FALSE);
    if (data->encryption_password_entry)
        gtk_editable_set_text(GTK_EDITABLE(data->encryption_password_entry), "");
    if (data->encryption_confirm_entry)
        gtk_editable_set_text(GTK_EDITABLE(data->encryption_confirm_entry), "");
    if (data->encryption_error_row)
        gtk_widget_set_visible(GTK_WIDGET(data->encryption_error_row), FALSE);
    window_disk_save_to_variables(data);
}

static void update_encryption_sensitivity(WindowDiskData *data)
{
    if (!data->encryption_switch || !gtk_switch_get_active(data->encryption_switch)) {
        if (data->encryption_error_row)
            gtk_widget_set_visible(GTK_WIDGET(data->encryption_error_row), FALSE);
        return;
    }

    const gchar *pass    = data->encryption_password_entry
        ? gtk_editable_get_text(GTK_EDITABLE(data->encryption_password_entry)) : "";
    const gchar *confirm = data->encryption_confirm_entry
        ? gtk_editable_get_text(GTK_EDITABLE(data->encryption_confirm_entry)) : "";

    gsize len          = strlen(pass);
    gboolean empty     = (len == 0);
    gboolean too_short = (len > 0 && len < 3);
    gboolean too_long  = (len > 15);
    gboolean mismatch  = (!empty && g_strcmp0(pass, confirm) != 0);
    gboolean valid     = (!empty && !too_short && !too_long && !mismatch);

    if (valid)
        window_disk_save_to_variables(data);

    if (data->encryption_error_row) {
        const gchar *msg = NULL;
        if (too_short || too_long) {
            msg = i18n_t("La contraseña debe tener entre 3 y 15 caracteres",
                         "Password must be between 3 and 15 characters",
                         "Пароль должен содержать от 3 до 15 символов");
        } else if (mismatch) {
            msg = i18n_t("Las contraseñas no coinciden",
                         "Passwords do not match",
                         "Пароли не совпадают");
        }
        if (msg) {
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->encryption_error_row), msg);
            gtk_widget_set_visible(GTK_WIDGET(data->encryption_error_row), TRUE);
        } else {
            gtk_widget_set_visible(GTK_WIDGET(data->encryption_error_row), FALSE);
        }
    }
}

/* Callback de respuesta al diálogo de cierre con cifrado inválido */
static void on_encryption_close_dialog_response(AdwAlertDialog *dialog,
                                                 const gchar    *response,
                                                 gpointer        user_data)
{
    (void)dialog;
    WindowDiskData *data = user_data;
    if (g_strcmp0(response, "close") == 0) {
        encryption_disable_switch(data);
        disk_window_defocus_and_close(data);
    }
}

gboolean on_disk_encryption_switch_toggled(GtkSwitch *sw, gboolean active, gpointer user_data)
{
    (void)sw;
    WindowDiskData *data = user_data;
    if (!data || !data->encryption_expander) return FALSE;

    if (active) {
        gtk_widget_set_sensitive(GTK_WIDGET(data->encryption_expander), TRUE);
        adw_expander_row_set_expanded(data->encryption_expander, TRUE);
    } else {
        adw_expander_row_set_expanded(data->encryption_expander, FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->encryption_expander), FALSE);
        if (data->encryption_password_entry)
            gtk_editable_set_text(GTK_EDITABLE(data->encryption_password_entry), "");
        if (data->encryption_confirm_entry)
            gtk_editable_set_text(GTK_EDITABLE(data->encryption_confirm_entry), "");
        if (data->encryption_error_row)
            gtk_widget_set_visible(GTK_WIDGET(data->encryption_error_row), FALSE);
    }

    update_encryption_sensitivity(data);
    window_disk_save_to_variables(data);
    return FALSE;  /* permite que el estado visual del switch se confirme */
}

void on_disk_encryption_password_changed(AdwEntryRow *entry, gpointer user_data)
{
    (void)entry;
    update_encryption_sensitivity((WindowDiskData *)user_data);
}

void on_disk_encryption_confirm_changed(AdwEntryRow *entry, gpointer user_data)
{
    (void)entry;
    update_encryption_sensitivity((WindowDiskData *)user_data);
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

/* ── traducción en tiempo real ───────────────────────────────────────────── */

void window_disk_update_language(WindowDiskData *data)
{
    if (!data) return;

    if (data->title_widget)
        adw_window_title_set_title(data->title_widget,
            i18n_t("Disco", "Disk", "Диск"));
    if (data->window)
        gtk_window_set_title(data->window,
            i18n_t("Disco", "Disk", "Диск"));
    if (data->close_button)
        gtk_button_set_label(data->close_button,
            i18n_t("Cerrar", "Close", "Закрыть"));
    if (data->save_button)
        gtk_button_set_label(data->save_button,
            i18n_t("Guardar", "Save", "Сохранить"));

    /* Grupo: Sistema de Archivos */
    if (data->filesystem_group) {
        adw_preferences_group_set_title(data->filesystem_group,
            i18n_t("Sistema de Archivos", "File System", "Файловая система"));
        adw_preferences_group_set_description(data->filesystem_group,
            i18n_t("Formatear el almacenamiento como:",
                   "Format storage as:",
                   "Форматировать хранилище как:"));
    }

    /* Grupo: Directorio Personal */
    if (data->home_group) {
        adw_preferences_group_set_title(data->home_group,
            i18n_t("Separación del Directorio Personal",
                   "Home Directory Separation",
                   "Разделение домашнего каталога"));
        adw_preferences_group_set_description(data->home_group,
            i18n_t("¿Desea crear una partición separada para '/home'?",
                   "Do you want to create a separate partition for '/home'?",
                   "Хотите создать отдельный раздел для '/home'?"));
    }
    if (data->home_subvolume_row)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->home_subvolume_row),
            i18n_t("Sí, como un subvolumen", "Yes, as a subvolume", "Да, как субтом"));
    if (data->home_partition_row)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->home_partition_row),
            i18n_t("Sí, como una partición", "Yes, as a partition", "Да, как раздел"));

    /* Expander: tamaño de partición raíz */
    if (data->root_size_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->root_size_expander),
            i18n_t("Tamaño de la partición raíz",
                   "Root partition size",
                   "Размер корневого раздела"));
        adw_expander_row_set_subtitle(data->root_size_expander,
            i18n_t("Ajustar el espacio para '/' y '/home'",
                   "Adjust space for '/' and '/home'",
                   "Настройте пространство для '/' и '/home'"));
    }
    if (data->slider_row)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->slider_row),
            i18n_t("Raíz ('/')", "Root ('/')", "Корень ('/')"));
    if (data->root_label_row)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->root_label_row),
            i18n_t("Tamaño partición raíz ('/')",
                   "Root partition size ('/')",
                   "Размер корневого раздела ('/')"));
    if (data->home_label_row)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->home_label_row),
            i18n_t("Tamaño partición '/home'",
                   "Home partition size ('/home')",
                   "Размер раздела '/home'"));

    /* Grupo: Swap */
    if (data->swap_group) {
        adw_preferences_group_set_title(data->swap_group,
            i18n_t("Memoria de Intercambio", "Swap Memory", "Память подкачки"));
        adw_preferences_group_set_description(data->swap_group,
            i18n_t("Selecciona el método de memoria virtual: Zram comprime datos en RAM, Swap usa una partición en disco y permite hibernación",
                   "Select virtual memory method: Zram compresses data in RAM, Swap uses a disk partition and enables hibernation",
                   "Выберите метод виртуальной памяти: Zram сжимает данные в ОЗУ, Swap использует раздел диска и поддерживает гибернацию"));
    }
    if (data->swap_none_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->swap_none_row),
            i18n_t("Zram (compresión en RAM)", "Zram (RAM compression)", "Zram (сжатие в ОЗУ)"));
        adw_action_row_set_subtitle(data->swap_none_row,
            i18n_t("Recomendado — no usa espacio en disco",
                   "Recommended — no disk space used",
                   "Рекомендуется — не использует место на диске"));
    }
    if (data->swap_half_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->swap_half_row),
            i18n_t("Swap: mitad de la RAM del sistema",
                   "Swap: half system RAM",
                   "Подкачка: половина ОЗУ системы"));
        adw_action_row_set_subtitle(data->swap_half_row,
            i18n_t("Zram activo + partición swap de la mitad de la RAM",
                   "Zram active + swap partition of half the RAM",
                   "Zram активен + раздел подкачки половины ОЗУ"));
    }
    if (data->swap_equal_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->swap_equal_row),
            i18n_t("Swap: igual a la RAM del sistema",
                   "Swap: equal to system RAM",
                   "Подкачка: равна ОЗУ системы"));
        adw_action_row_set_subtitle(data->swap_equal_row,
            i18n_t("Zram activo + partición swap igual a la RAM",
                   "Zram active + swap partition equal to RAM",
                   "Zram активен + раздел подкачки равный ОЗУ"));
    }
    if (data->swap_custom_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->swap_custom_row),
            i18n_t("Swap: tamaño personalizado",
                   "Swap: custom size",
                   "Подкачка: пользовательский размер"));
        adw_action_row_set_subtitle(data->swap_custom_row,
            i18n_t("Zram activo + partición swap de tamaño definido manualmente",
                   "Zram active + swap partition of manually defined size",
                   "Zram активен + раздел подкачки заданного вручную размера"));
    }
    if (data->swap_disabled_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->swap_disabled_row),
            i18n_t("No", "No", "Нет"));
        adw_action_row_set_subtitle(data->swap_disabled_row,
            i18n_t("Sin Zram ni Swap - no recomendado en sistemas con poca RAM",
                   "No Zram or Swap - not recommended on low-RAM systems",
                   "Без Zram и Swap - не рекомендуется на системах с малым объёмом ОЗУ"));
    }

    /* Grupo: Cifrado */
    if (data->encryption_group) {
        adw_preferences_group_set_title(data->encryption_group,
            i18n_t("Disco Cifrado", "Encrypted Disk", "Шифрование диска"));
        adw_preferences_group_set_description(data->encryption_group,
            i18n_t("Protege tus datos con LUKS y LVM\nSe pedirá una clave al arrancar el sistema",
                   "Protect your data with LUKS and LVM\nA passphrase will be required at boot",
                   "Защитите данные с LUKS и LVM\nПри загрузке потребуется ввести пароль"));
    }
    if (data->encryption_toggle_row)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->encryption_toggle_row),
            i18n_t("Cifrar el disco", "Encrypt the disk", "Зашифровать диск"));
    if (data->encryption_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->encryption_expander),
            i18n_t("Elija una clave para su disco",
                   "Choose a passphrase for your disk",
                   "Выберите ключ для диска"));
        adw_expander_row_set_subtitle(data->encryption_expander,
            i18n_t("El cifrado del disco protege sus archivos en caso de extravío del equipo",
                   "Disk encryption protects your files if the device is lost or stolen",
                   "Шифрование диска защищает файлы в случае утери или кражи устройства"));
    }
    if (data->encryption_password_entry)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->encryption_password_entry),
            i18n_t("Elija una contraseña", "Choose a password", "Введите пароль"));
    if (data->encryption_confirm_entry)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->encryption_confirm_entry),
            i18n_t("Confirme su contraseña", "Confirm your password", "Подтвердите пароль"));
}

