#include "window_repos.h"
#include "config.h"
#include "variables_utils.h"
#include "i18n.h"
#include <string.h>


static WindowReposData *global_repos_data = NULL;

// ---------------------------------------------------------------------------
// Helpers internos
// ---------------------------------------------------------------------------

static void show_error_dialog(WindowReposData *data, const gchar *title, const gchar *body)
{
    AdwDialog *dialog = adw_alert_dialog_new(title, body);
    adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "ok", "Aceptar");
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "ok");
    adw_dialog_present(dialog, GTK_WIDGET(data->window));
}




/* Descomenta líneas "#Server =" → "Server =". */
static gchar* process_mirrorlist(const gchar *text)
{
    gchar **lines = g_strsplit(text, "\n", -1);
    GString *result = g_string_new("");

    for (int i = 0; lines[i] != NULL; i++) {
        gchar *line = lines[i];
        if (g_str_has_prefix(line, "#Server ="))
            line = line + 1;
        g_string_append(result, line);
        if (lines[i + 1] != NULL)
            g_string_append_c(result, '\n');
    }

    g_strfreev(lines);
    return g_string_free(result, FALSE);
}

/* Valida que el texto contenga al menos "$repo/os/$arch". */
static gboolean validate_mirrorlist(const gchar *text)
{
    if (!text) return FALSE;
    return strstr(text, "$repo/os/$arch") != NULL;
}

/* Escapa newlines y comillas para almacenar en una variable bash. */
static gchar* escape_for_bash_var(const gchar *text)
{
    GString *result = g_string_new("");
    for (const gchar *p = text; *p; p++) {
        if (*p == '\n')
            g_string_append(result, "\\n");
        else if (*p == '"')
            g_string_append(result, "\\\"");
        else if (*p == '\\')
            g_string_append(result, "\\\\");
        else if (*p == '$')
            g_string_append(result, "\\$");
        else
            g_string_append_c(result, *p);
    }
    return g_string_free(result, FALSE);
}

// ---------------------------------------------------------------------------
// Auto-guardado de toggles/switches
// ---------------------------------------------------------------------------

static void apply_toggles(GString *content, gpointer user_data)
{
    WindowReposData *data = (WindowReposData *)user_data;

    gboolean chaotic = data->chaotic_aur_switch &&
                       adw_switch_row_get_active(data->chaotic_aur_switch);
    gboolean archcn  = data->archlinuxcn_switch &&
                       adw_switch_row_get_active(data->archlinuxcn_switch);
    gboolean cachyos = data->cachyos_switch &&
                       adw_switch_row_get_active(data->cachyos_switch);
    gboolean is_manual = data->manual_button &&
                         gtk_toggle_button_get_active(data->manual_button);

    vars_upsert(content, "REPOS_CHAOTIC_AUR", chaotic   ? "true" : "false");
    vars_upsert(content, "REPOS_ARCHLINUXCN", archcn    ? "true" : "false");
    vars_upsert(content, "REPOS_CACHYOS",     cachyos   ? "true" : "false");
    vars_upsert(content, "REPOS_MIRROR_MODE", is_manual ? "manual" : "auto");

    if (!is_manual)
        vars_remove(content, "REPOS_MIRROR_CUSTOM");
}

static void on_switch_or_toggle_changed(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
    (void)obj; (void)pspec;
    WindowReposData *data = (WindowReposData *)user_data;

    if (!vars_update(apply_toggles, data))
        LOG_WARNING("No se pudo auto-guardar cambio de repositorio");
    else
        LOG_INFO("Repositorios (switches/toggles) auto-guardados");
}

static void on_mirror_mode_toggled(GtkToggleButton *button, gpointer user_data)
{
    (void)button;
    WindowReposData *data = (WindowReposData *)user_data;

    gboolean is_manual = gtk_toggle_button_get_active(data->manual_button);

    if (data->mirrorlist_expander) {
        adw_expander_row_set_enable_expansion(data->mirrorlist_expander, is_manual);
        adw_expander_row_set_expanded(data->mirrorlist_expander, is_manual);
    }
    if (data->mirrorlist_textview)
        gtk_text_view_set_editable(data->mirrorlist_textview, is_manual);

    // Auto-guardar el cambio de modo
    if (!vars_update(apply_toggles, data))
        LOG_WARNING("No se pudo auto-guardar modo de mirror");
    else
        LOG_INFO("Modo mirror auto-guardado: %s", is_manual ? "manual" : "auto");
}

// ---------------------------------------------------------------------------
// Cierre con validación
// ---------------------------------------------------------------------------

static void on_close_validation_dialog_response(AdwAlertDialog *dialog,
                                                 const gchar    *response,
                                                 gpointer        user_data)
{
    (void)dialog;
    WindowReposData *data = (WindowReposData *)user_data;

    if (g_strcmp0(response, "close") == 0) {
        if (data->auto_button)
            gtk_toggle_button_set_active(data->auto_button, TRUE);
        if (data->window)
            gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
    }
}

static void do_close_with_validation(WindowReposData *data)
{
    if (!data) return;

    gboolean is_manual = data->manual_button &&
                         gtk_toggle_button_get_active(data->manual_button);

    if (is_manual && data->mirrorlist_textview) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(data->mirrorlist_textview);
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buf, &start, &end);
        gchar *raw_text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
        gboolean valid = validate_mirrorlist(raw_text);
        g_free(raw_text);

        if (!valid) {
            AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
                i18n_t("Mirrorlist sin contenido válido"),
                i18n_t("La mirrorlist está vacía o no contiene entradas válidas. "
                       "Si cierra, se volverá al modo automático.")
            ));
            adw_alert_dialog_add_response(dialog, "cancel", i18n_t("Cancelar"));
            adw_alert_dialog_add_response(dialog, "close",  i18n_t("Cerrar de todas formas"));
            adw_alert_dialog_set_default_response(dialog, "cancel");
            adw_alert_dialog_set_response_appearance(dialog, "close", ADW_RESPONSE_DESTRUCTIVE);
            g_signal_connect(dialog, "response",
                             G_CALLBACK(on_close_validation_dialog_response), data);
            adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(data->window));
            return;
        }
    }

    gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
}

static gboolean on_repos_close_request(GtkWindow *window, gpointer user_data)
{
    (void)window;
    do_close_with_validation((WindowReposData *)user_data);
    return TRUE; // Siempre interceptamos; do_close_with_validation oculta la ventana
}

// ---------------------------------------------------------------------------
// Visibilidad inicial
// ---------------------------------------------------------------------------

static void update_manual_visibility(WindowReposData *data)
{
    gboolean is_manual = gtk_toggle_button_get_active(data->manual_button);

    if (data->mirrorlist_expander) {
        adw_expander_row_set_enable_expansion(data->mirrorlist_expander, is_manual);
        adw_expander_row_set_expanded(data->mirrorlist_expander, is_manual);
    }
    if (data->mirrorlist_textview)
        gtk_text_view_set_editable(data->mirrorlist_textview, is_manual);
}

// ---------------------------------------------------------------------------
// API pública
// ---------------------------------------------------------------------------

WindowReposData* window_repos_new(void)
{
    if (global_repos_data)
        return global_repos_data;

    WindowReposData *data = g_new0(WindowReposData, 1);
    data->is_initialized = FALSE;
    global_repos_data = data;
    return data;
}

WindowReposData* window_repos_get_instance(void)
{
    return global_repos_data;
}

void window_repos_init(WindowReposData *data)
{
    if (!data || data->is_initialized) return;

    LOG_INFO("Inicializando ventana de repositorios");

    data->builder = gtk_builder_new();
    GError *error = NULL;

    if (!gtk_builder_add_from_resource(data->builder, "/org/gtk/arcris/window_repos.ui", &error)) {
        LOG_ERROR("Error cargando UI de repositorios: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    }

    data->window              = GTK_WINDOW(gtk_builder_get_object(data->builder, "repositoriosWindow"));
    data->close_button        = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button         = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));
    data->chaotic_aur_switch  = ADW_SWITCH_ROW(gtk_builder_get_object(data->builder, "chaotic_aur_switch"));
    data->archlinuxcn_switch  = ADW_SWITCH_ROW(gtk_builder_get_object(data->builder, "archlinuxcn_switch"));
    data->cachyos_switch      = ADW_SWITCH_ROW(gtk_builder_get_object(data->builder, "cachyos_switch"));
    data->window_title        = ADW_WINDOW_TITLE(gtk_builder_get_object(data->builder, "repos_window_title"));
    data->mirrorlist_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "mirrorlist_expander"));
    data->mirrorlist_link     = GTK_LINK_BUTTON(gtk_builder_get_object(data->builder, "mirrorlist_link"));
    data->auto_button         = GTK_TOGGLE_BUTTON(gtk_builder_get_object(data->builder, "auto_button"));
    data->manual_button       = GTK_TOGGLE_BUTTON(gtk_builder_get_object(data->builder, "manual_button"));
    data->repos_manual_row    = GTK_LIST_BOX_ROW(gtk_builder_get_object(data->builder, "repos_manual"));
    data->repos_textview_row  = GTK_LIST_BOX_ROW(gtk_builder_get_object(data->builder, "repos_textview_row"));
    data->mirrorlist_textview = GTK_TEXT_VIEW(gtk_builder_get_object(data->builder, "mirrorlist_textview"));

    if (data->close_button)
        g_signal_connect(data->close_button, "clicked", G_CALLBACK(on_repos_close_button_clicked), data);

    if (data->window)
        g_signal_connect(data->window, "close-request", G_CALLBACK(on_repos_close_request), data);

    if (data->save_button)
        g_signal_connect(data->save_button, "clicked", G_CALLBACK(on_repos_save_button_clicked), data);

    // Auto-guardado en switches
    if (data->chaotic_aur_switch)
        g_signal_connect(data->chaotic_aur_switch, "notify::active",
                         G_CALLBACK(on_switch_or_toggle_changed), data);
    if (data->archlinuxcn_switch)
        g_signal_connect(data->archlinuxcn_switch, "notify::active",
                         G_CALLBACK(on_switch_or_toggle_changed), data);
    if (data->cachyos_switch)
        g_signal_connect(data->cachyos_switch, "notify::active",
                         G_CALLBACK(on_switch_or_toggle_changed), data);

    // Toggle mode (visibilidad + auto-guardado)
    if (data->auto_button)
        g_signal_connect(data->auto_button, "toggled", G_CALLBACK(on_mirror_mode_toggled), data);
    if (data->manual_button)
        g_signal_connect(data->manual_button, "toggled", G_CALLBACK(on_mirror_mode_toggled), data);

    update_manual_visibility(data);

    data->is_initialized = TRUE;
    window_repos_update_language(data);
    LOG_INFO("Ventana de repositorios inicializada");
}

void window_repos_show(WindowReposData *data, GtkWindow *parent)
{
    if (!data) return;

    if (!data->is_initialized)
        window_repos_init(data);

    if (!data->window) return;

    if (parent) {
        gtk_window_set_transient_for(data->window, parent);
        gtk_window_set_modal(data->window, TRUE);
    }

    gtk_window_present(data->window);
    LOG_INFO("Ventana de repositorios mostrada");
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

/* Escribe las variables de repos con sus valores por defecto si no existen. */
void window_repos_init_defaults(void)
{
    GError *error = NULL;
    gchar *file_content = NULL;

    if (!g_file_get_contents(VARIABLES_FILE_PATH, &file_content, NULL, &error)) {
        LOG_ERROR("No se pudo leer variables.sh para defaults de repos: %s",
                  error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    }

    GString *content = g_string_new(file_content);
    g_free(file_content);

    // Solo inserta cada variable si no existe ya
    gchar *needle_chaotic  = "REPOS_CHAOTIC_AUR=";
    gchar *needle_archcn   = "REPOS_ARCHLINUXCN=";
    gchar *needle_cachyos  = "REPOS_CACHYOS=";
    gchar *needle_mode     = "REPOS_MIRROR_MODE=";
    gchar *needle_custom   = "REPOS_MIRROR_CUSTOM=";

    gboolean has_chaotic = strstr(content->str, needle_chaotic) != NULL;
    gboolean has_archcn  = strstr(content->str, needle_archcn)  != NULL;
    gboolean has_cachyos = strstr(content->str, needle_cachyos) != NULL;
    gboolean has_mode    = strstr(content->str, needle_mode)    != NULL;
    gboolean has_custom  = strstr(content->str, needle_custom)  != NULL;

    if (!has_chaotic || !has_archcn || !has_cachyos || !has_mode || !has_custom) {
        g_string_append(content, "\n# Configuración de repositorios\n");

        if (!has_chaotic) g_string_append(content, "REPOS_CHAOTIC_AUR=\"false\"\n");
        if (!has_archcn)  g_string_append(content, "REPOS_ARCHLINUXCN=\"false\"\n");
        if (!has_cachyos) g_string_append(content, "REPOS_CACHYOS=\"false\"\n");
        if (!has_mode)    g_string_append(content, "REPOS_MIRROR_MODE=\"auto\"\n");
        if (!has_custom)  g_string_append(content, "\nREPOS_MIRROR_CUSTOM=\"\"\n");

        if (!g_file_set_contents(VARIABLES_FILE_PATH, content->str, -1, &error)) {
            LOG_ERROR("Error escribiendo defaults de repos: %s",
                      error ? error->message : "Unknown error");
            if (error) g_error_free(error);
        } else {
            LOG_INFO("Variables de repositorios inicializadas con defaults");
        }
    }

    g_string_free(content, TRUE);
}

void on_repos_close_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    do_close_with_validation((WindowReposData *)user_data);
}

void on_repos_save_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    WindowReposData *data = (WindowReposData *)user_data;
    if (!data) return;

    gboolean is_manual = data->manual_button &&
                         gtk_toggle_button_get_active(data->manual_button);

    // En modo automático no hay nada que validar en el textview
    if (!is_manual) {
        gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
        return;
    }

    // -----------------------------------------------------------------------
    // Validar y guardar mirrorlist (solo en modo manual)
    // -----------------------------------------------------------------------
    if (!data->mirrorlist_textview) {
        show_error_dialog(data, "Error", "No se pudo acceder al campo de mirrorlist.");
        return;
    }

    GtkTextBuffer *buf = gtk_text_view_get_buffer(data->mirrorlist_textview);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    gchar *raw_text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

    if (!validate_mirrorlist(raw_text)) {
        show_error_dialog(data,
            i18n_t("Mirrorlist inválida"),
            i18n_t("El contenido no es válido. Debe contener al menos una entrada "
                   "con \"$repo/os/$arch\" (formato estándar de Arch Linux)."));
        g_free(raw_text);
        return;
    }

    gchar *processed     = process_mirrorlist(raw_text);
    gchar *mirror_escaped = escape_for_bash_var(processed);
    g_free(raw_text);
    g_free(processed);

    // Leer → actualizar REPOS_MIRROR_CUSTOM → guardar
    GError *error = NULL;
    gchar *file_content = NULL;

    if (!g_file_get_contents(VARIABLES_FILE_PATH, &file_content, NULL, &error)) {
        LOG_ERROR("No se pudo leer variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        show_error_dialog(data, "Error", "No se pudo leer el archivo de configuración.");
        g_free(mirror_escaped);
        return;
    }

    GString *content = g_string_new(file_content);
    g_free(file_content);

    vars_upsert_after(content, "REPOS_MIRROR_CUSTOM", mirror_escaped, "REPOS_MIRROR_MODE");
    g_free(mirror_escaped);

    if (!g_file_set_contents(VARIABLES_FILE_PATH, content->str, -1, &error)) {
        LOG_ERROR("Error guardando variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        show_error_dialog(data, "Error", "No se pudo guardar el archivo de configuración.");
        g_string_free(content, TRUE);
        return;
    }

    g_string_free(content, TRUE);
    LOG_INFO("Mirrorlist guardada en variables.sh");

    gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
}

void window_repos_update_language(WindowReposData *data)
{
    if (!data) return;

    if (data->close_button)
        gtk_button_set_label(data->close_button,
            i18n_t("Cerrar"));
    if (data->save_button)
        gtk_button_set_label(data->save_button,
            i18n_t("Guardar"));
    if (data->window_title)
        adw_window_title_set_title(data->window_title,
            i18n_t("Repositorios"));
    if (data->chaotic_aur_switch)
        adw_action_row_set_subtitle(ADW_ACTION_ROW(data->chaotic_aur_switch),
            i18n_t("AUR precompilado, uso general"));
    if (data->archlinuxcn_switch)
        adw_action_row_set_subtitle(ADW_ACTION_ROW(data->archlinuxcn_switch),
            i18n_t("Paquetes populares precompilados"));
    if (data->cachyos_switch)
        adw_action_row_set_subtitle(ADW_ACTION_ROW(data->cachyos_switch),
            i18n_t("Repositorios separados optimizados para diferentes CPU."));
    if (data->mirrorlist_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->mirrorlist_expander),
            i18n_t("Mirrorlist Oficiales"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(data->mirrorlist_expander),
            i18n_t("Lista de espejos oficiales de Arch Linux"));
    }
    if (data->auto_button)
        gtk_button_set_label(GTK_BUTTON(data->auto_button),
            i18n_t("Automático"));
    if (data->manual_button)
        gtk_button_set_label(GTK_BUTTON(data->manual_button),
            i18n_t("Manual"));
    if (data->mirrorlist_link)
        gtk_button_set_label(GTK_BUTTON(data->mirrorlist_link),
            i18n_t("Generar mirrorlist en archlinux.org/mirrorlist/"));
}
