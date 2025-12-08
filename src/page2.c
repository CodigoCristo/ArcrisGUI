#include "page2.h"
#include "config.h"
#include "internet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libsoup/soup.h>
#include <gio/gio.h>

// Variable global para datos de la página 2
static Page2Data *g_page2_data = NULL;

// Función para obtener el idioma desde la API
static gchar* page2_get_language_from_api(void)
{
    SoupSession *session;
    SoupMessage *msg;
    GBytes *response_body;
    gchar *language_code = NULL;

    session = soup_session_new();
    msg = soup_message_new("GET", "https://ipapi.co/languages");

    if (msg) {
        response_body = soup_session_send_and_read(session, msg, NULL, NULL);

        if (response_body) {
            const char *body_data = g_bytes_get_data(response_body, NULL);
            if (body_data) {
                // Extraer el primer código de idioma completo (ej: "es-PE" de "es-PE,qu,ay")
                gchar **languages = g_strsplit(body_data, ",", -1);
                if (languages && languages[0]) {
                    // Limpiar espacios en blanco y saltos de línea
                    language_code = g_strstrip(g_strdup(languages[0]));
                    g_print("🌍 Idioma detectado: %s\n", language_code);
                }
                g_strfreev(languages);
            }
            g_bytes_unref(response_body);
        }
        g_object_unref(msg);
    }

    g_object_unref(session);
    return language_code;
}

// Función para obtener la zona horaria desde la API
static gchar* page2_get_timezone_from_api(void)
{
    SoupSession *session;
    SoupMessage *msg;
    GBytes *response_body;
    gchar *timezone_code = NULL;

    session = soup_session_new();
    msg = soup_message_new("GET", "https://ipapi.co/timezone");

    if (msg) {
        response_body = soup_session_send_and_read(session, msg, NULL, NULL);

        if (response_body) {
            const char *body_data = g_bytes_get_data(response_body, NULL);
            if (body_data) {
                // Limpiar el resultado (remover espacios y saltos de línea)
                gchar *clean_timezone = g_strstrip(g_strdup(body_data));
                if (clean_timezone && strlen(clean_timezone) > 0) {
                    timezone_code = g_strdup(clean_timezone);
                    g_print("🕐 Zona horaria detectada: %s\n", timezone_code);
                }
                g_free(clean_timezone);
            }
            g_bytes_unref(response_body);
        }
        g_object_unref(msg);
    }

    g_object_unref(session);
    return timezone_code;
}

// Función para encontrar y seleccionar automáticamente un elemento en ComboRow
static void auto_select_in_combo_row(AdwComboRow *combo_row, const gchar *search_text)
{
    if (!combo_row || !search_text) return;

    GListModel *model = adw_combo_row_get_model(combo_row);
    if (!model) return;

    guint n_items = g_list_model_get_n_items(model);

    for (guint i = 0; i < n_items; i++) {
        GtkStringObject *item = GTK_STRING_OBJECT(g_list_model_get_item(model, i));
        if (item) {
            const gchar *item_text = gtk_string_object_get_string(item);
            if (item_text && g_str_has_prefix(item_text, search_text)) {
                adw_combo_row_set_selected(combo_row, i);
                g_print("✅ Auto-seleccionado en ComboRow: %s\n", item_text);
                g_object_unref(item);
                return;
            }
            g_object_unref(item);
        }
    }
    g_print("⚠ No se encontró '%s' en ComboRow\n", search_text);
}

// Estructura para pasar datos al hilo de configuración automática
typedef struct {
    gchar *detected_language;
    gchar *detected_timezone;
} AutoConfigData;

// Función que se ejecuta en el hilo principal para actualizar la UI
static gboolean apply_auto_config_to_ui(gpointer user_data)
{
    AutoConfigData *config_data = (AutoConfigData *)user_data;

    if (!g_page2_data) {
        g_free(config_data->detected_language);
        g_free(config_data->detected_timezone);
        g_free(config_data);
        return FALSE;
    }

    // Configurar basándose en el idioma detectado
    if (config_data->detected_language) {
        g_print("🔧 Aplicando configuración automática para idioma: %s\n", config_data->detected_language);

        // Extraer solo el código de idioma para teclados (ej: "es" de "es-PE")
        gchar **lang_parts = g_strsplit(config_data->detected_language, "-", 2);
        gchar *keyboard_lang = NULL;
        if (lang_parts && lang_parts[0]) {
            keyboard_lang = g_strdup(lang_parts[0]);
        }
        g_strfreev(lang_parts);

        if (keyboard_lang) {
            // Configurar teclado X11 basándose en el idioma
            auto_select_in_combo_row(g_page2_data->combo_keyboard, keyboard_lang);

            // Configurar keymap de consola basándose en el idioma
            auto_select_in_combo_row(g_page2_data->combo_keymap, keyboard_lang);

            g_free(keyboard_lang);
        }

        // Configurar locale basándose en el idioma detectado, convirtiendo es-PE a es_PE
        gchar *locale_search = g_strdup(config_data->detected_language);
        for (gchar *p = locale_search; *p; p++) {
            if (*p == '-') *p = '_';
        }
        auto_select_in_combo_row(g_page2_data->combo_locale, locale_search);
        g_free(locale_search);
    }

    // Configurar basándose en la zona horaria detectada
    if (config_data->detected_timezone) {
        g_print("🔧 Aplicando configuración automática zona horaria: %s\n", config_data->detected_timezone);

        // Configurar zona horaria
        auto_select_in_combo_row(g_page2_data->combo_timezone, config_data->detected_timezone);

        // Aplicar inmediatamente la zona horaria detectada
        setenv("TZ", config_data->detected_timezone, 1);
        tzset();

        // Forzar actualización inmediata del tiempo
        if (g_page2_data->time_label) {
            update_time_display(NULL);
        }
    }

    g_print("✅ Configuración automática aplicada a la UI\n");

    // Guardar automáticamente las variables configuradas
    save_combo_selections_to_file();

    // Limpiar memoria
    g_free(config_data->detected_language);
    g_free(config_data->detected_timezone);
    g_free(config_data);

    return FALSE; // No repetir
}

// Función que se ejecuta en un hilo separado para obtener configuración automática
static gpointer auto_config_worker_thread(gpointer user_data)
{
    g_print("🚀 Iniciando configuración automática en hilo separado...\n");

    AutoConfigData *config_data = g_malloc0(sizeof(AutoConfigData));

    // Obtener idioma detectado (sin bloquear UI)
    config_data->detected_language = page2_get_language_from_api();
    if (!config_data->detected_language) {
        g_print("⚠ No se pudo detectar el idioma desde API\n");
    }

    // Obtener zona horaria detectada (sin bloquear UI)
    config_data->detected_timezone = page2_get_timezone_from_api();
    if (!config_data->detected_timezone) {
        g_print("⚠ No se pudo detectar la zona horaria desde API\n");
    }

    // Programar la aplicación de configuración en el hilo principal
    g_idle_add(apply_auto_config_to_ui, config_data);

    g_print("✅ Datos de configuración automática obtenidos\n");
    return NULL;
}

// Función para configurar automáticamente los ComboRows basándose en el idioma y zona horaria detectados
void auto_configure_combo_rows(void)
{
    if (!g_page2_data) return;

    g_print("🌐 Iniciando configuración automática (modo asíncrono)...\n");

    // Ejecutar la configuración automática en un hilo separado
    GThread *config_thread = g_thread_new("auto-config-thread", auto_config_worker_thread, NULL);

    // Liberar la referencia al hilo (se limpiará automáticamente cuando termine)
    g_thread_unref(config_thread);
}

// Función helper para ejecutar comandos del sistema y llenar listas
gboolean execute_system_command_to_list(const char *command, GtkStringList *list)
{
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        g_warning("Failed to execute command: %s", command);
        return FALSE;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Eliminar el salto de línea al final
        buffer[strcspn(buffer, "\n")] = '\0';
        gtk_string_list_append(list, buffer);
    }

    pclose(fp);
    return TRUE;
}

// Funciones de carga de datos
void page2_load_keyboards(GtkStringList *keyboard_list)
{
    execute_system_command_to_list("localectl list-x11-keymap-layouts", keyboard_list);
}

void page2_load_keymaps(GtkStringList *keymap_list)
{
    execute_system_command_to_list("localectl list-keymaps", keymap_list);
}

void page2_load_timezones(GtkStringList *timezone_list)
{
    execute_system_command_to_list("timedatectl --no-pager list-timezones", timezone_list);
}

void page2_load_locales(GtkStringList *locale_list)
{
    GBytes *resource_data;
    const gchar *resource_content;
    gsize content_length;

    // Cargar el recurso embebido locale.gen
    resource_data = g_resources_lookup_data("/org/gtk/arcris/locale.gen",
                                          G_RESOURCE_LOOKUP_FLAGS_NONE,
                                          NULL);

    if (!resource_data) {
        g_print("❌ Error: No se pudo cargar el recurso locale.gen\n");
        return;
    }

    resource_content = g_bytes_get_data(resource_data, &content_length);

    if (!resource_content) {
        g_bytes_unref(resource_data);
        return;
    }

    // Procesar el contenido línea por línea
    gchar **lines = g_strsplit(resource_content, "\n", -1);

    for (gint i = 0; lines[i] != NULL; i++) {
        gchar *line = g_strstrip(lines[i]);

        // Filtrar líneas que empiecen con '#  ' (comentarios con doble espacio)
        if (g_str_has_prefix(line, "#  ")) {
            continue;
        }

        // Quitar el '#' del inicio si existe
        if (g_str_has_prefix(line, "#")) {
            line = line + 1;
        }

        // Buscar líneas que contengan '.UTF-8 UTF-8'
        if (g_strstr_len(line, -1, ".UTF-8 UTF-8")) {
            // Extraer la primera parte (antes del espacio) - equivalente a awk '{print $1}'
            gchar **parts = g_strsplit(line, " ", 2);
            if (parts && parts[0] && strlen(parts[0]) > 0) {
                gtk_string_list_append(locale_list, parts[0]);
            }
            g_strfreev(parts);
        }
    }

    g_strfreev(lines);
    g_bytes_unref(resource_data);

    g_print("✅ Locales cargados desde recurso embebido\n");
}

// Función helper para configurar ComboRows
void page2_setup_combo_row(AdwComboRow *combo_row, GtkStringList *model,
                           GCallback callback, gpointer user_data)
{
    // Crear expresión para mostrar las cadenas
    GtkExpression *exp = gtk_property_expression_new(
        GTK_TYPE_STRING_OBJECT,
        NULL,
        "string"
    );

    // Configurar el ComboRow
    adw_combo_row_set_model(combo_row, G_LIST_MODEL(model));
    adw_combo_row_set_expression(combo_row, exp);
    adw_combo_row_set_search_match_mode(combo_row, GTK_STRING_FILTER_MATCH_MODE_SUBSTRING);

    // Conectar callback si se proporciona
    if (callback) {
        g_signal_connect(combo_row, "notify::selected-item", callback, user_data);
    }
}

// Callbacks para los ComboRows
void on_keyboard_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *keyboard = gtk_string_object_get_string(selected_item);
        g_print("Teclado seleccionado: %s\n", keyboard);

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

void on_keymap_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *keymap = gtk_string_object_get_string(selected_item);
        g_print("Keymap TTY seleccionado: %s\n", keymap);

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

void on_timezone_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *timezone = gtk_string_object_get_string(selected_item);
        g_print("Zona horaria seleccionada: %s\n", timezone);

        // Aplicar inmediatamente la zona horaria
        setenv("TZ", timezone, 1);
        tzset();

        // Forzar actualización inmediata del tiempo
        if (g_page2_data && g_page2_data->time_label) {
            update_time_display(NULL);
        }

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

void on_locale_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *locale = gtk_string_object_get_string(selected_item);
        g_print("Locale seleccionado: %s\n", locale);

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

// Función para actualizar la hora con zona horaria actual
// Función para guardar las selecciones de ComboRow en archivo bash
void save_combo_selections_to_file(void)
{
    if (!g_page2_data) return;

    LOG_INFO("=== save_combo_selections_to_file INICIADO ===");

    gchar *bash_file_path = g_build_filename(".", "data", "bash", "variables.sh", NULL);

    // Leer el archivo existente para preservar SELECTED_DISK, PARTITION_MODE, INSTALLATION_TYPE, DESKTOP_ENVIRONMENT, WINDOW_MANAGER y variables de usuario
    gchar *selected_disk_value = NULL;
    gchar *partition_mode_value = NULL;
    gchar *installation_type_value = NULL;
    gchar *selected_kernel_value = NULL;
    gchar *desktop_environment_value = NULL;
    gchar *window_manager_value = NULL;
    gchar *user_value = NULL;
    gchar *password_user_value = NULL;
    gchar *hostname_value = NULL;
    gchar *password_root_value = NULL;
    gchar *driver_video_value = NULL;
    gchar *driver_audio_value = NULL;
    gchar *driver_wifi_value = NULL;
    gchar *driver_bluetooth_value = NULL;
    gchar *essential_apps_enabled_value = NULL;
    gchar *utilities_enabled_value = NULL;
    gchar *program_extra_value = NULL;
    FILE *read_file = fopen(bash_file_path, "r");
    if (read_file) {
        char line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Buscar la variable SELECTED_DISK
            if (g_str_has_prefix(line, "SELECTED_DISK=")) {
                char *value = line + 14; // Saltar "SELECTED_DISK="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 14;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                selected_disk_value = g_strdup(value);
            }
            // Buscar la variable PARTITION_MODE
            else if (g_str_has_prefix(line, "PARTITION_MODE=")) {
                char *value = line + 15; // Saltar "PARTITION_MODE="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 15;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                partition_mode_value = g_strdup(value);
                LOG_INFO("PARTITION_MODE preservado desde variables.sh: %s", partition_mode_value);
            }
            // Buscar la variable INSTALLATION_TYPE
            else if (g_str_has_prefix(line, "INSTALLATION_TYPE=")) {
                char *value = line + 18; // Saltar "INSTALLATION_TYPE="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 18;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                installation_type_value = g_strdup(value);
                LOG_INFO("INSTALLATION_TYPE preservado desde variables.sh: %s", installation_type_value);
            }
            // Buscar la variable SELECTED_KERNEL
            else if (g_str_has_prefix(line, "SELECTED_KERNEL=")) {
                char *value = line + 16; // Saltar "SELECTED_KERNEL="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 16;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                selected_kernel_value = g_strdup(value);
                LOG_INFO("SELECTED_KERNEL preservado desde variables.sh: %s", selected_kernel_value);
            }
            // Buscar la variable DRIVER_VIDEO
            else if (g_str_has_prefix(line, "DRIVER_VIDEO=")) {
                char *value = line + 13; // Saltar "DRIVER_VIDEO="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 13;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                driver_video_value = g_strdup(value);
                LOG_INFO("DRIVER_VIDEO preservado desde variables.sh: %s", driver_video_value);
            }
            // Buscar la variable DRIVER_AUDIO
            else if (g_str_has_prefix(line, "DRIVER_AUDIO=")) {
                char *value = line + 13; // Saltar "DRIVER_AUDIO="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 13;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                driver_audio_value = g_strdup(value);
                LOG_INFO("DRIVER_AUDIO preservado desde variables.sh: %s", driver_audio_value);
            }
            // Buscar la variable DRIVER_WIFI
            else if (g_str_has_prefix(line, "DRIVER_WIFI=")) {
                char *value = line + 12; // Saltar "DRIVER_WIFI="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 12;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                driver_wifi_value = g_strdup(value);
                LOG_INFO("DRIVER_WIFI preservado desde variables.sh: %s", driver_wifi_value);
            }
            // Buscar la variable DRIVER_BLUETOOTH
            else if (g_str_has_prefix(line, "DRIVER_BLUETOOTH=")) {
                char *value = line + 17; // Saltar "DRIVER_BLUETOOTH="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 17;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                driver_bluetooth_value = g_strdup(value);
                LOG_INFO("DRIVER_BLUETOOTH preservado desde variables.sh: %s", driver_bluetooth_value);
            }
            // Buscar la variable USER
            else if (g_str_has_prefix(line, "export USER=") || g_str_has_prefix(line, "USER=")) {
                char *value = strstr(line, "USER=") + 5; // Saltar "USER="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "USER=") + 5;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                user_value = g_strdup(value);
            }
            // Buscar la variable PASSWORD_USER
            else if (g_str_has_prefix(line, "export PASSWORD_USER=") || g_str_has_prefix(line, "PASSWORD_USER=")) {
                char *value = strstr(line, "PASSWORD_USER=") + 14; // Saltar "PASSWORD_USER="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "PASSWORD_USER=") + 14;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                password_user_value = g_strdup(value);
            }
            // Buscar la variable HOSTNAME
            else if (g_str_has_prefix(line, "export HOSTNAME=") || g_str_has_prefix(line, "HOSTNAME=")) {
                char *value = strstr(line, "HOSTNAME=") + 9; // Saltar "HOSTNAME="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "HOSTNAME=") + 9;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                hostname_value = g_strdup(value);
            }
            // Buscar la variable PASSWORD_ROOT
            else if (g_str_has_prefix(line, "export PASSWORD_ROOT=") || g_str_has_prefix(line, "PASSWORD_ROOT=")) {
                char *value = strstr(line, "PASSWORD_ROOT=") + 14; // Saltar "PASSWORD_ROOT="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "PASSWORD_ROOT=") + 14;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                password_root_value = g_strdup(value);
            }
            // Buscar la variable DESKTOP_ENVIRONMENT
            else if (g_str_has_prefix(line, "DESKTOP_ENVIRONMENT=")) {
                char *value = line + 20; // Saltar "DESKTOP_ENVIRONMENT="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 20;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                desktop_environment_value = g_strdup(value);
                LOG_INFO("DESKTOP_ENVIRONMENT preservado desde variables.sh: %s", desktop_environment_value);
            }
            // Buscar la variable WINDOW_MANAGER
            else if (g_str_has_prefix(line, "WINDOW_MANAGER=")) {
                char *value = line + 15; // Saltar "WINDOW_MANAGER="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 15;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                window_manager_value = g_strdup(value);
                LOG_INFO("WINDOW_MANAGER preservado desde variables.sh: %s", window_manager_value);
            }
            // Buscar la variable ESSENTIAL_APPS_ENABLED (solo preservar)
            else if (g_str_has_prefix(line, "ESSENTIAL_APPS_ENABLED=")) {
                char *value = line + 23; // Saltar "ESSENTIAL_APPS_ENABLED="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 23;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                essential_apps_enabled_value = g_strdup(value);
                LOG_INFO("ESSENTIAL_APPS_ENABLED preservado desde variables.sh: %s", essential_apps_enabled_value);
            }
            // Buscar la variable UTILITIES_ENABLED (solo preservar)
            else if (g_str_has_prefix(line, "UTILITIES_ENABLED=")) {
                char *value = line + 18; // Saltar "UTILITIES_ENABLED="

                // Remover salto de línea
                line[strcspn(line, "\n")] = 0;
                value = line + 18;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                utilities_enabled_value = g_strdup(value);
                LOG_INFO("UTILITIES_ENABLED preservado desde page2: %s", utilities_enabled_value);
            }
            else if (g_str_has_prefix(line, "PROGRAM_EXTRA=")) {
                char *value = line + 14;
                line[strcspn(line, "\n")] = 0;
                value = line + 14;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                program_extra_value = g_strdup(value);
                LOG_INFO("PROGRAM_EXTRA preservado desde page2: %s", program_extra_value);
            }
        }
        fclose(read_file);
    }

    // Escribir el archivo actualizado
    FILE *file = fopen(bash_file_path, "w");
    if (!file) {
        g_print("❌ Error: No se pudo crear el archivo %s\n", bash_file_path);
        g_free(bash_file_path);
        g_free(selected_disk_value);
        g_free(partition_mode_value);
        g_free(installation_type_value);
        g_free(selected_kernel_value);
        g_free(desktop_environment_value);
        g_free(window_manager_value);
        g_free(driver_video_value);
        g_free(driver_audio_value);
        g_free(driver_wifi_value);
        g_free(driver_bluetooth_value);
        g_free(user_value);
        g_free(password_user_value);
        g_free(hostname_value);
        g_free(password_root_value);
        return;
    }

    // Escribir header del archivo
    fprintf(file, "#!/bin/bash\n");
    fprintf(file, "# Variables de configuración generadas por Arcris\n");
    fprintf(file, "# Archivo generado automáticamente - No editar manualmente\n\n");

    // Obtener y guardar selección de teclado
    GtkStringObject *keyboard_item = adw_combo_row_get_selected_item(g_page2_data->combo_keyboard);
    if (keyboard_item) {
        const gchar *keyboard = gtk_string_object_get_string(keyboard_item);
        fprintf(file, "KEYBOARD_LAYOUT=\"%s\"\n", keyboard);
    }

    // Obtener y guardar selección de keymap
    GtkStringObject *keymap_item = adw_combo_row_get_selected_item(g_page2_data->combo_keymap);
    if (keymap_item) {
        const gchar *keymap = gtk_string_object_get_string(keymap_item);
        fprintf(file, "KEYMAP_TTY=\"%s\"\n", keymap);
    }

    // Obtener y guardar selección de zona horaria
    GtkStringObject *timezone_item = adw_combo_row_get_selected_item(g_page2_data->combo_timezone);
    if (timezone_item) {
        const gchar *timezone = gtk_string_object_get_string(timezone_item);
        fprintf(file, "TIMEZONE=\"%s\"\n", timezone);
    }

    // Obtener y guardar selección de locale
    GtkStringObject *locale_item = adw_combo_row_get_selected_item(g_page2_data->combo_locale);
    if (locale_item) {
        const gchar *locale = gtk_string_object_get_string(locale_item);
        fprintf(file, "LOCALE=\"%s\"\n", locale);
    }

    // Preservar la variable SELECTED_DISK
    if (selected_disk_value) {
        fprintf(file, "SELECTED_DISK=\"%s\"\n", selected_disk_value);
    } else {
        fprintf(file, "SELECTED_DISK=\"\"\n");
    }

    // Preservar la variable PARTITION_MODE
    if (partition_mode_value) {
        fprintf(file, "PARTITION_MODE=\"%s\"\n", partition_mode_value);
        LOG_INFO("PARTITION_MODE reescrito en variables.sh: %s", partition_mode_value);
    } else {
        LOG_INFO("No se encontró PARTITION_MODE para preservar en page2");
    }

    // Preservar variables de usuario si existen
    if (user_value || password_user_value || hostname_value || password_root_value) {
        fprintf(file, "\n# Variables de configuración del usuario\n");

        if (user_value) {
            fprintf(file, "export USER=\"%s\"\n", user_value);
        }
        if (password_user_value) {
            fprintf(file, "export PASSWORD_USER=\"%s\"\n", password_user_value);
        }
        if (hostname_value) {
            fprintf(file, "export HOSTNAME=\"%s\"\n", hostname_value);
        }
        if (password_root_value) {
            fprintf(file, "# La contraseña del usuario también será la contraseña de root\n");
            fprintf(file, "export PASSWORD_ROOT=\"%s\"\n", password_root_value);
        }
    }

    // Preservar la variable INSTALLATION_TYPE
    if (installation_type_value) {
        fprintf(file, "\n# Tipo de instalación seleccionado\n");
        fprintf(file, "INSTALLATION_TYPE=\"%s\"\n", installation_type_value);
        LOG_INFO("INSTALLATION_TYPE reescrito en variables.sh: %s", installation_type_value);
    }

    // Preservar la variable SELECTED_KERNEL
    if (selected_kernel_value) {
        fprintf(file, "\n# Kernel seleccionado\n");
        fprintf(file, "SELECTED_KERNEL=\"%s\"\n", selected_kernel_value);
        LOG_INFO("SELECTED_KERNEL reescrito en variables.sh: %s", selected_kernel_value);
    }

    // Preservar las variables de drivers de hardware
    if (driver_video_value) {
        fprintf(file, "\n# Driver de Video\n");
        fprintf(file, "DRIVER_VIDEO=\"%s\"\n", driver_video_value);
        LOG_INFO("DRIVER_VIDEO reescrito en variables.sh: %s", driver_video_value);
    }

    if (driver_audio_value) {
        fprintf(file, "\n# Driver de Audio\n");
        fprintf(file, "DRIVER_AUDIO=\"%s\"\n", driver_audio_value);
        LOG_INFO("DRIVER_AUDIO reescrito en variables.sh: %s", driver_audio_value);
    }

    if (driver_wifi_value) {
        fprintf(file, "\n# Driver de WiFi\n");
        fprintf(file, "DRIVER_WIFI=\"%s\"\n", driver_wifi_value);
        LOG_INFO("DRIVER_WIFI reescrito en variables.sh: %s", driver_wifi_value);
    }

    if (driver_bluetooth_value) {
        fprintf(file, "\n# Driver de Bluetooth\n");
        fprintf(file, "DRIVER_BLUETOOTH=\"%s\"\n", driver_bluetooth_value);
        LOG_INFO("DRIVER_BLUETOOTH reescrito en variables.sh: %s", driver_bluetooth_value);
    }

    // Preservar la variable DESKTOP_ENVIRONMENT
    if (desktop_environment_value) {
        fprintf(file, "# Variable DE seleccionada\n");
        fprintf(file, "DESKTOP_ENVIRONMENT=\"%s\"\n", desktop_environment_value);
        LOG_INFO("DESKTOP_ENVIRONMENT reescrito en variables.sh: %s", desktop_environment_value);
    }

    // Preservar la variable WINDOW_MANAGER
    if (window_manager_value) {
        fprintf(file, "\n# Variable WM seleccionada\n");
        fprintf(file, "WINDOW_MANAGER=\"%s\"\n", window_manager_value);
        LOG_INFO("WINDOW_MANAGER reescrito en variables.sh: %s", window_manager_value);
    }

    // Escribir variables de página 6 - siempre escribir
    fprintf(file, "\n# Configuración de aplicaciones - Página 6\n");

    if (essential_apps_enabled_value) {
        fprintf(file, "ESSENTIAL_APPS_ENABLED=\"%s\"\n", essential_apps_enabled_value);
        LOG_INFO("ESSENTIAL_APPS_ENABLED preservado en variables.sh: %s", essential_apps_enabled_value);
    } else {
        fprintf(file, "ESSENTIAL_APPS_ENABLED=\"true\"\n");
        LOG_INFO("ESSENTIAL_APPS_ENABLED inicializado por defecto desde page2: true");
    }

    if (utilities_enabled_value) {
        fprintf(file, "UTILITIES_ENABLED=\"%s\"\n", utilities_enabled_value);
        LOG_INFO("UTILITIES_ENABLED preservado en variables.sh: %s", utilities_enabled_value);
    } else {
        fprintf(file, "UTILITIES_ENABLED=\"false\"\n");
        LOG_INFO("UTILITIES_ENABLED inicializado por defecto desde page2: false");
    }

    if (program_extra_value) {
        fprintf(file, "PROGRAM_EXTRA=\"%s\"\n", program_extra_value);
        LOG_INFO("PROGRAM_EXTRA preservado desde page2: %s", program_extra_value);
    } else {
        fprintf(file, "PROGRAM_EXTRA=\"false\"\n");
        LOG_INFO("PROGRAM_EXTRA inicializado por defecto desde page2: false");
    }



    fclose(file);

    g_print("✅ Variables guardadas en: %s\n", bash_file_path);
    LOG_INFO("=== save_combo_selections_to_file FINALIZADO ===");
    g_free(bash_file_path);
    g_free(selected_disk_value);
    g_free(partition_mode_value);
    g_free(installation_type_value);
    g_free(selected_kernel_value);
    g_free(desktop_environment_value);
    g_free(window_manager_value);
    g_free(essential_apps_enabled_value);
    g_free(utilities_enabled_value);
    g_free(program_extra_value);
    g_free(driver_video_value);
    g_free(driver_audio_value);
    g_free(driver_wifi_value);
    g_free(driver_bluetooth_value);
    g_free(user_value);
    g_free(password_user_value);
    g_free(hostname_value);
    g_free(password_root_value);
}

gboolean update_time_display(gpointer user_data)
{
    if (!g_page2_data || !g_page2_data->time_label || !g_page2_data->combo_timezone) {
        return FALSE; // Detener el timer si no hay datos
    }

    // Obtener la zona horaria seleccionada del ComboRow
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(g_page2_data->combo_timezone);
    const gchar *timezone = NULL;

    if (selected_item) {
        timezone = gtk_string_object_get_string(selected_item);
    }

    // Configurar la zona horaria si está disponible
    if (timezone && strlen(timezone) > 0) {
        setenv("TZ", timezone, 1);
        tzset();
    }

    // Obtener la hora actual
    time_t raw_time;
    struct tm *time_info;
    char time_string[64];

    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_string, sizeof(time_string), "%H:%M:%S - %d/%m/%Y", time_info);

    // Actualizar el label
    gtk_label_set_text(g_page2_data->time_label, time_string);

    return TRUE; // Continuar actualizando
}

// Función para abrir configuración de teclado
void open_keyboard_settings(GtkButton *button, gpointer user_data)
{
    // Abrir la aplicación de configuración de teclado del sistema
    const char* cmd = arcris_get_keyboard_settings_command();
    gchar *full_command = g_strdup_printf("%s &", cmd);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    system(full_command);
#pragma GCC diagnostic pop
    g_free(full_command);
}

// Función auxiliar para abrir la aplicación de visualización de teclado
static gpointer open_tecla_task(gpointer data) {
    if (!g_page2_data || !g_page2_data->combo_keyboard || !g_page2_data->combo_keymap) {
        return NULL;
    }

    // Obtener el String del Row1 (teclado X11 seleccionado)
    GtkStringObject *keyboard_item = adw_combo_row_get_selected_item(g_page2_data->combo_keyboard);
    const gchar *keyboard_layout = NULL;

    // Obtener el String del Row2 (keymap TTY seleccionado)
    GtkStringObject *keymap_item = adw_combo_row_get_selected_item(g_page2_data->combo_keymap);
    const gchar *keymap_tty = NULL;

    if (keyboard_item) {
        keyboard_layout = gtk_string_object_get_string(keyboard_item);
    }

    if (keymap_item) {
        keymap_tty = gtk_string_object_get_string(keymap_item);
    }

    if (keyboard_layout && keymap_tty) {
        // Abrir kbd-layout-viewer5 con el layout seleccionado
        gchar *viewer_command = g_strdup_printf("kbd-layout-viewer5 -l %s &", keyboard_layout);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        system(viewer_command);
#pragma GCC diagnostic pop
        g_free(viewer_command);
        g_print("Abriendo visualización de teclado para: %s\n", keyboard_layout);

        // Ejecutar comandos para cambiar el idioma en el sistema
        gchar *x11_command = g_strdup_printf("sudo setxkbmap %s 2>/dev/null || true", keyboard_layout);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        system(x11_command);
#pragma GCC diagnostic pop
        g_free(x11_command);
        g_print("Configurando teclado X11: %s\n", keyboard_layout);
    }

    return NULL;
}

// Función para abrir la aplicación de visualización de teclado
void open_tecla(GtkButton *button, gpointer user_data)
{
    // Ejecutar en un hilo separado para no bloquear la interfaz
    GThread *thread = g_thread_new("open-tecla-thread", open_tecla_task, NULL);
    g_thread_unref(thread);
}

// Función principal de inicialización de la página 2
void page2_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page2_data = g_malloc0(sizeof(Page2Data));

    // Guardar referencias importantes
    g_page2_data->carousel = carousel;
    g_page2_data->revealer = revealer;

    // Cargar la página 2 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page2.ui");
    GtkWidget *page2 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page2"));

    // Obtener widgets específicos de la página
    g_page2_data->combo_keyboard = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row1"));
    g_page2_data->combo_keymap = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row2"));
    g_page2_data->combo_timezone = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row3"));
    g_page2_data->combo_locale = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row4"));
    g_page2_data->tecla_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "tecla"));
    g_page2_data->time_label = GTK_LABEL(gtk_builder_get_object(page_builder, "locale_time_label"));

    // Obtener modelos de datos
    g_page2_data->keyboard_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "main_keyboard"));
    g_page2_data->keymap_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "tty_keyboard"));
    g_page2_data->timezone_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "string_timezones"));
    g_page2_data->locale_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "locale_list"));

    // Cargar datos en las listas
    page2_load_keyboards(g_page2_data->keyboard_list);
    page2_load_keymaps(g_page2_data->keymap_list);
    page2_load_timezones(g_page2_data->timezone_list);
    page2_load_locales(g_page2_data->locale_list);

    // Configurar ComboRows
    page2_setup_combo_row(g_page2_data->combo_keyboard, g_page2_data->keyboard_list,
                          G_CALLBACK(on_keyboard_selection_changed), g_page2_data);

    page2_setup_combo_row(g_page2_data->combo_keymap, g_page2_data->keymap_list,
                          G_CALLBACK(on_keymap_selection_changed), g_page2_data);

    page2_setup_combo_row(g_page2_data->combo_timezone, g_page2_data->timezone_list,
                          G_CALLBACK(on_timezone_selection_changed), g_page2_data);

    page2_setup_combo_row(g_page2_data->combo_locale, g_page2_data->locale_list,
                          G_CALLBACK(on_locale_selection_changed), g_page2_data);

    // Configuración especial para timezone (seleccionar el segundo elemento por defecto temporalmente)
    adw_combo_row_set_selected(g_page2_data->combo_timezone, 1);

    // Configurar automáticamente los ComboRows basándose en el idioma detectado (asíncrono)
    auto_configure_combo_rows();

    // Conectar señales adicionales
    g_signal_connect(g_page2_data->tecla_button, "clicked", G_CALLBACK(open_tecla), NULL);

    // Iniciar actualización de tiempo cada segundo
    g_timeout_add_seconds(1, update_time_display, NULL);

    // Añadir la página al carousel
    adw_carousel_append(carousel, page2);

    // Liberar el builder de la página
    g_object_unref(page_builder);
}

// Función de limpieza
void page2_cleanup(Page2Data *data)
{
    if (g_page2_data) {
        g_free(g_page2_data);
        g_page2_data = NULL;
    }
}
