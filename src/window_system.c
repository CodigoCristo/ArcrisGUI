#include "window_system.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

// Variable global para la instancia
static WindowSystemData *g_system_instance = NULL;

// Nombres de shells para la conversión
static const char* shell_names[] = {
    "bash",
    "dash",
    "ksh",
    "fish",
    "zsh"
};

WindowSystemData* window_system_new(void)
{
    WindowSystemData *data = g_new0(WindowSystemData, 1);
    if (!data) {
        LOG_ERROR("No se pudo asignar memoria para WindowSystemData");
        return NULL;
    }

    // Inicializar valores por defecto
    data->current_shell = SHELL_BASH;
    data->filesystems_enabled = FALSE;
    data->compression_enabled = FALSE;
    data->video_codecs_enabled = FALSE;
    data->is_initialized = FALSE;
    data->is_visible = FALSE;

    LOG_INFO("Nueva instancia de WindowSystemData creada");
    return data;
}

void window_system_init(WindowSystemData *data)
{
    if (!data) {
        LOG_ERROR("WindowSystemData es NULL en init");
        return;
    }

    // Cargar el builder desde recursos
    data->builder = gtk_builder_new_from_resource("/org/gtk/arcris/window_system.ui");
    if (!data->builder) {
        LOG_ERROR("No se pudo cargar el builder de window_system.ui");
        return;
    }

    // Cargar widgets desde el builder
    window_system_load_widgets_from_builder(data);

    // Configurar widgets
    window_system_setup_widgets(data);

    // Conectar señales
    window_system_connect_signals(data);

    // Cargar configuración
    window_system_load_configuration(data);

    data->is_initialized = TRUE;
    LOG_INFO("WindowSystemData inicializada correctamente");
}

void window_system_cleanup(WindowSystemData *data)
{
    if (!data) return;

    if (data->window && data->is_visible) {
        gtk_window_close(GTK_WINDOW(data->window));
    }

    if (data->builder) {
        g_object_unref(data->builder);
        data->builder = NULL;
    }

    g_free(data);
    LOG_INFO("WindowSystemData limpiada");
}

WindowSystemData* window_system_get_instance(void)
{
    if (!g_system_instance) {
        g_system_instance = window_system_new();
        if (g_system_instance) {
            window_system_init(g_system_instance);
        }
    }
    return g_system_instance;
}

void window_system_load_widgets_from_builder(WindowSystemData *data)
{
    if (!data || !data->builder) {
        LOG_ERROR("Datos inválidos en window_system_load_widgets_from_builder");
        return;
    }

    // Cargar ventana principal
    data->window = ADW_APPLICATION_WINDOW(gtk_builder_get_object(data->builder, "appBaseListWindow"));

    // Cargar botones del header
    data->close_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));

    // Cargar widgets de configuración
    data->shell_combo = ADW_COMBO_ROW(gtk_builder_get_object(data->builder, "shell_combo"));

    // Buscar switches (usando nombres genéricos ya que no están definidos específicamente en el UI)
    GObject *obj;
    obj = gtk_builder_get_object(data->builder, "filesystems_switch");
    if (obj && ADW_IS_SWITCH_ROW(obj)) {
        data->filesystems_switch = ADW_SWITCH_ROW(obj);
    }

    obj = gtk_builder_get_object(data->builder, "compression_switch");
    if (obj && ADW_IS_SWITCH_ROW(obj)) {
        data->compression_switch = ADW_SWITCH_ROW(obj);
    }

    obj = gtk_builder_get_object(data->builder, "video_codecs_switch");
    if (obj && ADW_IS_SWITCH_ROW(obj)) {
        data->video_codecs_switch = ADW_SWITCH_ROW(obj);
    }

    // Verificar widgets críticos
    if (!data->window) LOG_WARNING("No se pudo obtener la ventana principal");
    if (!data->close_button) LOG_WARNING("No se pudo obtener close_button");
    if (!data->save_button) LOG_WARNING("No se pudo obtener save_button");
    if (!data->shell_combo) LOG_WARNING("No se pudo obtener shell_combo");

    LOG_INFO("Widgets cargados desde el builder");
}

void window_system_setup_widgets(WindowSystemData *data)
{
    if (!data) return;

    // Configurar combo de shell
    if (data->shell_combo) {
        adw_combo_row_set_selected(data->shell_combo, data->current_shell);
    }

    // Configurar switches
    if (data->filesystems_switch) {
        adw_switch_row_set_active(data->filesystems_switch, data->filesystems_enabled);
    }

    if (data->compression_switch) {
        adw_switch_row_set_active(data->compression_switch, data->compression_enabled);
    }

    if (data->video_codecs_switch) {
        adw_switch_row_set_active(data->video_codecs_switch, data->video_codecs_enabled);
    }

    LOG_INFO("Widgets configurados");
}

void window_system_connect_signals(WindowSystemData *data)
{
    if (!data) return;

    // Conectar botones del header
    if (data->close_button) {
        g_signal_connect(data->close_button, "clicked",
                        G_CALLBACK(on_system_close_button_clicked), data);
    }

    if (data->save_button) {
        g_signal_connect(data->save_button, "clicked",
                        G_CALLBACK(on_system_save_button_clicked), data);
    }

    // Conectar combo de shell
    if (data->shell_combo) {
        g_signal_connect(data->shell_combo, "notify::selected",
                        G_CALLBACK(on_shell_combo_changed), data);
    }

    // Conectar switches
    if (data->filesystems_switch) {
        g_signal_connect(data->filesystems_switch, "notify::active",
                        G_CALLBACK(on_filesystems_switch_toggled), data);
    }

    if (data->compression_switch) {
        g_signal_connect(data->compression_switch, "notify::active",
                        G_CALLBACK(on_compression_switch_toggled), data);
    }

    if (data->video_codecs_switch) {
        g_signal_connect(data->video_codecs_switch, "notify::active",
                        G_CALLBACK(on_video_codecs_switch_toggled), data);
    }

    LOG_INFO("Señales conectadas");
}

void window_system_show(WindowSystemData *data, GtkWindow *parent)
{
    if (!data || !data->window) {
        LOG_ERROR("Datos inválidos en window_system_show");
        return;
    }

    // Configurar ventana padre
    if (parent) {
        gtk_window_set_transient_for(GTK_WINDOW(data->window), parent);
    }

    // Cargar variables desde archivo antes de mostrar
    load_system_variables_from_file();
    
    // Mostrar la ventana
    gtk_window_present(GTK_WINDOW(data->window));
    data->is_visible = TRUE;
    
    LOG_INFO("Ventana del sistema mostrada");
}

void window_system_hide(WindowSystemData *data)
{
    if (!data || !data->window) return;

    gtk_window_close(GTK_WINDOW(data->window));
    data->is_visible = FALSE;

    LOG_INFO("Ventana del sistema ocultada");
}

void window_system_load_configuration(WindowSystemData *data)
{
    if (!data) return;

    // TODO: Cargar configuración desde archivo de configuración
    // Por ahora usar valores por defecto

    LOG_INFO("Configuración del sistema cargada");
}

void window_system_save_configuration(WindowSystemData *data)
{
    if (!data) return;

    // TODO: Guardar configuración en archivo

    LOG_INFO("Configuración del sistema guardada");
}

// Callbacks
void on_system_close_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowSystemData *data = (WindowSystemData *)user_data;
    if (!data) return;

    window_system_hide(data);
    LOG_INFO("Ventana del sistema cerrada por el usuario");
}

void on_system_save_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowSystemData *data = (WindowSystemData *)user_data;
    if (!data) return;

    window_system_save_configuration(data);
    save_system_variables_to_file();
    window_system_hide(data);
    LOG_INFO("Configuración guardada y ventana cerrada");
}

void on_shell_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    WindowSystemData *data = (WindowSystemData *)user_data;
    if (!data || !combo) return;

    guint selected = adw_combo_row_get_selected(combo);
    if (selected < G_N_ELEMENTS(shell_names)) {
        data->current_shell = (SystemShell)selected;
        LOG_INFO("Shell cambiado a: %s", shell_names[selected]);
        
        // Guardar automáticamente en variables.sh
        save_system_variables_to_file();
    }
}

void on_filesystems_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    WindowSystemData *data = (WindowSystemData *)user_data;
    if (!data || !ADW_IS_SWITCH_ROW(object)) return;

    gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(object));
    data->filesystems_enabled = active;
    
    LOG_INFO("Sistemas de archivos %s", active ? "habilitados" : "deshabilitados");
    
    // Guardar automáticamente en variables.sh
    save_system_variables_to_file();
}

void on_compression_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    WindowSystemData *data = (WindowSystemData *)user_data;
    if (!data || !ADW_IS_SWITCH_ROW(object)) return;

    gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(object));
    data->compression_enabled = active;
    
    LOG_INFO("Compresión %s", active ? "habilitada" : "deshabilitada");
    
    // Guardar automáticamente en variables.sh
    save_system_variables_to_file();
}

void on_video_codecs_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    WindowSystemData *data = (WindowSystemData *)user_data;
    if (!data || !ADW_IS_SWITCH_ROW(object)) return;

    gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(object));
    data->video_codecs_enabled = active;
    
    LOG_INFO("Códecs de video %s", active ? "habilitados" : "deshabilitados");
    
    // Guardar automáticamente en variables.sh
    save_system_variables_to_file();
}

// Funciones de acceso
SystemShell window_system_get_shell(void)
{
    WindowSystemData *data = window_system_get_instance();
    return data ? data->current_shell : SHELL_BASH;
}

gboolean window_system_get_filesystems_enabled(void)
{
    WindowSystemData *data = window_system_get_instance();
    return data ? data->filesystems_enabled : FALSE;
}

gboolean window_system_get_compression_enabled(void)
{
    WindowSystemData *data = window_system_get_instance();
    return data ? data->compression_enabled : FALSE;
}

gboolean window_system_get_video_codecs_enabled(void)
{
    WindowSystemData *data = window_system_get_instance();
    return data ? data->video_codecs_enabled : FALSE;
}

// Funciones de utilidad
const char* window_system_shell_to_string(SystemShell shell)
{
    if (shell >= 0 && shell < G_N_ELEMENTS(shell_names)) {
        return shell_names[shell];
    }
    return "bash";
}

SystemShell window_system_string_to_shell(const char *shell_name)
{
    if (!shell_name) return SHELL_BASH;

    for (int i = 0; i < G_N_ELEMENTS(shell_names); i++) {
        if (g_strcmp0(shell_name, shell_names[i]) == 0) {
            return (SystemShell)i;
        }
    }
    return SHELL_BASH;
}

void load_system_variables_from_file(void)
{
    WindowSystemData *data = window_system_get_instance();
    if (!data) {
        LOG_ERROR("No se pudo obtener la instancia de WindowSystemData para cargar variables");
        return;
    }

    LOG_INFO("=== load_system_variables_from_file INICIADO ===");

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);
    
    // Variables por defecto
    SystemShell shell = SHELL_BASH;
    gboolean filesystems_enabled = FALSE;
    gboolean compression_enabled = FALSE;
    gboolean video_codecs_enabled = FALSE;

    FILE *read_file = fopen(bash_file_path, "r");
    if (read_file) {
        char line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Leer SYSTEM_SHELL
            if (g_str_has_prefix(line, "SYSTEM_SHELL=")) {
                char *value = line + 13;
                line[strcspn(line, "\n")] = 0;
                value = line + 13;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                shell = window_system_string_to_shell(value);
                LOG_INFO("SYSTEM_SHELL cargado: %s", value);
            }
            // Leer FILESYSTEMS_ENABLED
            else if (g_str_has_prefix(line, "FILESYSTEMS_ENABLED=")) {
                char *value = line + 20;
                line[strcspn(line, "\n")] = 0;
                value = line + 20;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                filesystems_enabled = (g_strcmp0(value, "true") == 0);
                LOG_INFO("FILESYSTEMS_ENABLED cargado: %s", filesystems_enabled ? "true" : "false");
            }
            // Leer COMPRESSION_ENABLED
            else if (g_str_has_prefix(line, "COMPRESSION_ENABLED=")) {
                char *value = line + 20;
                line[strcspn(line, "\n")] = 0;
                value = line + 20;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                compression_enabled = (g_strcmp0(value, "true") == 0);
                LOG_INFO("COMPRESSION_ENABLED cargado: %s", compression_enabled ? "true" : "false");
            }
            // Leer VIDEO_CODECS_ENABLED
            else if (g_str_has_prefix(line, "VIDEO_CODECS_ENABLED=")) {
                char *value = line + 21;
                line[strcspn(line, "\n")] = 0;
                value = line + 21;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                video_codecs_enabled = (g_strcmp0(value, "true") == 0);
                LOG_INFO("VIDEO_CODECS_ENABLED cargado: %s", video_codecs_enabled ? "true" : "false");
            }
        }
        fclose(read_file);
    } else {
        LOG_INFO("Archivo variables.sh no existe, usando valores por defecto");
    }

    // Actualizar datos internos
    data->current_shell = shell;
    data->filesystems_enabled = filesystems_enabled;
    data->compression_enabled = compression_enabled;
    data->video_codecs_enabled = video_codecs_enabled;

    // Actualizar widgets UI si están disponibles
    if (data->shell_combo) {
        adw_combo_row_set_selected(data->shell_combo, (guint)shell);
    }
    if (data->filesystems_switch) {
        adw_switch_row_set_active(data->filesystems_switch, filesystems_enabled);
    }
    if (data->compression_switch) {
        adw_switch_row_set_active(data->compression_switch, compression_enabled);
    }
    if (data->video_codecs_switch) {
        adw_switch_row_set_active(data->video_codecs_switch, video_codecs_enabled);
    }

    g_free(bash_file_path);
    LOG_INFO("=== load_system_variables_from_file FINALIZADO ===");
}

void save_system_variables_to_file(void)
{
    WindowSystemData *data = window_system_get_instance();
    if (!data) {
        LOG_ERROR("No se pudo obtener la instancia de WindowSystemData para guardar variables");
        return;
    }

    LOG_INFO("=== save_system_variables_to_file INICIADO ===");

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);

    // Leer el archivo existente para preservar todas las variables existentes
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
    gchar *keyboard_layout_value = NULL;
    gchar *keymap_tty_value = NULL;
    gchar *timezone_value = NULL;
    gchar *locale_value = NULL;
    gchar *essential_apps_enabled_value = NULL;
    gchar *utilities_enabled_value = NULL;
    gchar *extra_programs_value = NULL;
    gchar *utilities_apps_value = NULL;
    gchar *program_extra_value = NULL;
    gchar *encryption_enabled_value = NULL;
    gchar *encryption_password_value = NULL;

    FILE *read_file = fopen(bash_file_path, "r");
    if (read_file) {
        char line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Preservar variables existentes
            if (g_str_has_prefix(line, "SELECTED_DISK=")) {
                char *value = line + 14;
                line[strcspn(line, "\n")] = 0;
                value = line + 14;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                selected_disk_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "PARTITION_MODE=")) {
                char *value = line + 15;
                line[strcspn(line, "\n")] = 0;
                value = line + 15;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                partition_mode_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "INSTALLATION_TYPE=")) {
                char *value = line + 18;
                line[strcspn(line, "\n")] = 0;
                value = line + 18;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                installation_type_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "SELECTED_KERNEL=")) {
                char *value = line + 16;
                line[strcspn(line, "\n")] = 0;
                value = line + 16;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                selected_kernel_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "DESKTOP_ENVIRONMENT=")) {
                char *value = line + 20;
                line[strcspn(line, "\n")] = 0;
                value = line + 20;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                desktop_environment_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "WINDOW_MANAGER=")) {
                char *value = line + 15;
                line[strcspn(line, "\n")] = 0;
                value = line + 15;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                window_manager_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "DRIVER_VIDEO=")) {
                char *value = line + 13;
                line[strcspn(line, "\n")] = 0;
                value = line + 13;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                driver_video_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "DRIVER_AUDIO=")) {
                char *value = line + 13;
                line[strcspn(line, "\n")] = 0;
                value = line + 13;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                driver_audio_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "DRIVER_WIFI=")) {
                char *value = line + 12;
                line[strcspn(line, "\n")] = 0;
                value = line + 12;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                driver_wifi_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "DRIVER_BLUETOOTH=")) {
                char *value = line + 17;
                line[strcspn(line, "\n")] = 0;
                value = line + 17;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                driver_bluetooth_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "KEYBOARD_LAYOUT=")) {
                char *value = line + 16;
                line[strcspn(line, "\n")] = 0;
                value = line + 16;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                keyboard_layout_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "KEYMAP_TTY=")) {
                char *value = line + 11;
                line[strcspn(line, "\n")] = 0;
                value = line + 11;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                keymap_tty_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "TIMEZONE=")) {
                char *value = line + 9;
                line[strcspn(line, "\n")] = 0;
                value = line + 9;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                timezone_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "LOCALE=")) {
                char *value = line + 7;
                line[strcspn(line, "\n")] = 0;
                value = line + 7;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                locale_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "export USER=") || g_str_has_prefix(line, "USER=")) {
                char *value = strstr(line, "USER=") + 5;
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "USER=") + 5;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                user_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "export PASSWORD_USER=") || g_str_has_prefix(line, "PASSWORD_USER=")) {
                char *value = strstr(line, "PASSWORD_USER=") + 14;
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "PASSWORD_USER=") + 14;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                password_user_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "export HOSTNAME=") || g_str_has_prefix(line, "HOSTNAME=")) {
                char *value = strstr(line, "HOSTNAME=") + 9;
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "HOSTNAME=") + 9;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                hostname_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "export PASSWORD_ROOT=") || g_str_has_prefix(line, "PASSWORD_ROOT=")) {
                char *value = strstr(line, "PASSWORD_ROOT=") + 14;
                line[strcspn(line, "\n")] = 0;
                value = strstr(line, "PASSWORD_ROOT=") + 14;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                password_root_value = g_strdup(value);
            }
            // Buscar la variable ESSENTIAL_APPS_ENABLED (solo preservar)
            else if (g_str_has_prefix(line, "ESSENTIAL_APPS_ENABLED=")) {
                char *value = line + 23;
                line[strcspn(line, "\n")] = 0;
                value = line + 23;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                essential_apps_enabled_value = g_strdup(value);
                LOG_INFO("ESSENTIAL_APPS_ENABLED preservado desde variables.sh: %s", essential_apps_enabled_value);
            }
            // Buscar la variable UTILITIES_ENABLED (solo preservar)
            else if (g_str_has_prefix(line, "UTILITIES_ENABLED=")) {
                char *value = line + 18;
                line[strcspn(line, "\n")] = 0;
                value = line + 18;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                utilities_enabled_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "EXTRA_PROGRAMS=")) {
                line[strcspn(line, "\n")] = 0;
                extra_programs_value = g_strdup(line + 15); // Guardar todo después de "EXTRA_PROGRAMS="
            }
            else if (g_str_has_prefix(line, "UTILITIES_APPS=")) {
                line[strcspn(line, "\n")] = 0;
                utilities_apps_value = g_strdup(line + 15); // Guardar todo después de "UTILITIES_APPS="
            }
            else if (g_str_has_prefix(line, "PROGRAM_EXTRA=")) {
                line[strcspn(line, "\n")] = 0;
                program_extra_value = g_strdup(line + 14); // Guardar todo después de "PROGRAM_EXTRA="
            }
            else if (g_str_has_prefix(line, "ENCRYPTION_ENABLED=")) {
                char *value = line + 19;
                line[strcspn(line, "\n")] = 0;
                value = line + 19;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                encryption_enabled_value = g_strdup(value);
            }
            else if (g_str_has_prefix(line, "ENCRYPTION_PASSWORD=")) {
                char *value = line + 20;
                line[strcspn(line, "\n")] = 0;
                value = line + 20;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                encryption_password_value = g_strdup(value);
            }


        }
        fclose(read_file);
    }

    // Escribir el archivo completo con todas las variables
    FILE *file = fopen(bash_file_path, "w");
    if (!file) {
        LOG_ERROR("No se pudo abrir el archivo %s para escritura", bash_file_path);
        g_free(bash_file_path);
        return;
    }

    fprintf(file, "#!/bin/bash\n");
    fprintf(file, "# Archivo de variables de configuración de Arcris\n");
    fprintf(file, "# Generado automáticamente - No editar manualmente\n\n");

    // Escribir variables de configuración regional (page2)
    if (keyboard_layout_value) {
        fprintf(file, "# Configuración regional\n");
        fprintf(file, "KEYBOARD_LAYOUT=\"%s\"\n", keyboard_layout_value);
    }
    if (keymap_tty_value) {
        fprintf(file, "KEYMAP_TTY=\"%s\"\n", keymap_tty_value);
    }
    if (timezone_value) {
        fprintf(file, "TIMEZONE=\"%s\"\n", timezone_value);
    }
    if (locale_value) {
        fprintf(file, "LOCALE=\"%s\"\n", locale_value);
    }

    // Escribir variables del sistema (window_system)
    fprintf(file, "\n# Configuración del sistema\n");
    
    // Shell del sistema
    if (data->shell_combo) {
        guint selected = adw_combo_row_get_selected(data->shell_combo);
        const char *shell = window_system_shell_to_string((SystemShell)selected);
        fprintf(file, "SYSTEM_SHELL=\"%s\"\n", shell);
        LOG_INFO("SYSTEM_SHELL guardado: %s", shell);
    }

    // Sistemas de archivos
    if (data->filesystems_switch) {
        gboolean enabled = adw_switch_row_get_active(data->filesystems_switch);
        fprintf(file, "FILESYSTEMS_ENABLED=\"%s\"\n", enabled ? "true" : "false");
        LOG_INFO("FILESYSTEMS_ENABLED guardado: %s", enabled ? "true" : "false");
    }

    // Compresión de archivos
    if (data->compression_switch) {
        gboolean enabled = adw_switch_row_get_active(data->compression_switch);
        fprintf(file, "COMPRESSION_ENABLED=\"%s\"\n", enabled ? "true" : "false");
        LOG_INFO("COMPRESSION_ENABLED guardado: %s", enabled ? "true" : "false");
    }

    // Códecs de video
    if (data->video_codecs_switch) {
        gboolean enabled = adw_switch_row_get_active(data->video_codecs_switch);
        fprintf(file, "VIDEO_CODECS_ENABLED=\"%s\"\n", enabled ? "true" : "false");
        LOG_INFO("VIDEO_CODECS_ENABLED guardado: %s", enabled ? "true" : "false");
    }

    // Preservar variables de disco y partición
    if (selected_disk_value) {
        fprintf(file, "\n# Configuración de disco\n");
        fprintf(file, "SELECTED_DISK=\"%s\"\n", selected_disk_value);
    }
    if (partition_mode_value) {
        fprintf(file, "PARTITION_MODE=\"%s\"\n", partition_mode_value);
    }

    // Preservar variables de usuario
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
            fprintf(file, "export PASSWORD_ROOT=\"%s\"\n", password_root_value);
        }
    }

    // Preservar variables de instalación
    if (installation_type_value) {
        fprintf(file, "\n# Tipo de instalación\n");
        fprintf(file, "INSTALLATION_TYPE=\"%s\"\n", installation_type_value);
    }
    if (selected_kernel_value) {
        fprintf(file, "\n# Kernel seleccionado\n");
        fprintf(file, "SELECTED_KERNEL=\"%s\"\n", selected_kernel_value);
    }

    // Preservar variables de entorno de escritorio
    if (desktop_environment_value) {
        fprintf(file, "\n# Entorno de escritorio\n");
        fprintf(file, "DESKTOP_ENVIRONMENT=\"%s\"\n", desktop_environment_value);
    }
    if (window_manager_value) {
        fprintf(file, "WINDOW_MANAGER=\"%s\"\n", window_manager_value);
    }

    // Preservar variables de drivers de hardware
    if (driver_video_value || driver_audio_value || driver_wifi_value || driver_bluetooth_value) {
        fprintf(file, "\n# Drivers de hardware\n");
        if (driver_video_value) {
            fprintf(file, "DRIVER_VIDEO=\"%s\"\n", driver_video_value);
        }
        if (driver_audio_value) {
            fprintf(file, "DRIVER_AUDIO=\"%s\"\n", driver_audio_value);
        }
        if (driver_wifi_value) {
            fprintf(file, "DRIVER_WIFI=\"%s\"\n", driver_wifi_value);
        }
        if (driver_bluetooth_value) {
            fprintf(file, "DRIVER_BLUETOOTH=\"%s\"\n", driver_bluetooth_value);
        }
    }

    // Escribir variables de página 6
    fprintf(file, "\n# Configuración de aplicaciones - Página 6\n");
    
    if (essential_apps_enabled_value) {
        fprintf(file, "ESSENTIAL_APPS_ENABLED=\"%s\"\n", essential_apps_enabled_value);
        LOG_INFO("ESSENTIAL_APPS_ENABLED preservado en variables.sh: %s", essential_apps_enabled_value);
    }
    if (utilities_enabled_value) {
        fprintf(file, "UTILITIES_ENABLED=\"%s\"\n", utilities_enabled_value);
        LOG_INFO("UTILITIES_ENABLED preservado en variables.sh: %s", utilities_enabled_value);
    }
    
    // Program Extra Status - siempre escribir
    if (program_extra_value) {
        fprintf(file, "PROGRAM_EXTRA=%s\n", program_extra_value);
        LOG_INFO("PROGRAM_EXTRA preservado desde window_system: %s", program_extra_value);
    } else {
        // Inicializar por defecto si no existe
        fprintf(file, "PROGRAM_EXTRA=\"false\"\n");
        LOG_INFO("PROGRAM_EXTRA inicializado por defecto desde window_system: false");
    }

    // Preservar programas extra y utilidades
    if (extra_programs_value) {
        fprintf(file, "\n# Programas extra agregados por el usuario\n");
        fprintf(file, "EXTRA_PROGRAMS=%s\n", extra_programs_value);
        LOG_INFO("EXTRA_PROGRAMS preservado desde window_system: %s", extra_programs_value);
    }

    if (utilities_apps_value) {
        fprintf(file, "\n# Utilidades seleccionadas\n");
        fprintf(file, "UTILITIES_APPS=%s\n", utilities_apps_value);
        LOG_INFO("UTILITIES_APPS preservado desde window_system: %s", utilities_apps_value);
    }

    // Preservar variables de cifrado
    if (encryption_enabled_value || encryption_password_value) {
        if (encryption_enabled_value) {
            fprintf(file, "ENCRYPTION_ENABLED=\"%s\"\n", encryption_enabled_value);
            LOG_INFO("ENCRYPTION_ENABLED preservado desde window_system: %s", encryption_enabled_value);
        }
        if (encryption_password_value) {
            fprintf(file, "ENCRYPTION_PASSWORD=\"%s\"\n", encryption_password_value);
            LOG_INFO("ENCRYPTION_PASSWORD preservado desde window_system: %s", encryption_password_value);
        }
    }



    fclose(file);

    // Liberar memoria
    g_free(bash_file_path);
    g_free(selected_disk_value);
    g_free(partition_mode_value);
    g_free(installation_type_value);
    g_free(selected_kernel_value);
    g_free(desktop_environment_value);
    g_free(window_manager_value);
    g_free(user_value);
    g_free(password_user_value);
    g_free(hostname_value);
    g_free(password_root_value);
    g_free(driver_video_value);
    g_free(driver_audio_value);
    g_free(driver_wifi_value);
    g_free(driver_bluetooth_value);
    g_free(keyboard_layout_value);
    g_free(keymap_tty_value);
    g_free(timezone_value);
    g_free(locale_value);
    g_free(essential_apps_enabled_value);
    g_free(utilities_enabled_value);
    g_free(extra_programs_value);
    g_free(utilities_apps_value);
    g_free(program_extra_value);
    g_free(encryption_enabled_value);
    g_free(encryption_password_value);



    LOG_INFO("Variables del sistema guardadas exitosamente en %s", bash_file_path);
    LOG_INFO("=== save_system_variables_to_file FINALIZADO ===");
}
