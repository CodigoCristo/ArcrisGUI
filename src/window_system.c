#include "window_system.h"
#include "variables_utils.h"
#include "config.h"
#include "i18n.h"
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
    window_system_update_language(data);
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
    data->window_title = ADW_WINDOW_TITLE(gtk_builder_get_object(data->builder, "system_window_title"));

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

    gchar *bash_file_path = g_build_filename(".", "data", "bash", "variables.sh", NULL);
    
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

static void apply_system_vars(GString *content, gpointer user_data)
{
    WindowSystemData *data = (WindowSystemData *)user_data;

    if (data->shell_combo) {
        guint selected = adw_combo_row_get_selected(data->shell_combo);
        const char *shell = window_system_shell_to_string((SystemShell)selected);
        vars_upsert_after(content, "SYSTEM_SHELL", shell, "ESSENTIAL_APPS_ENABLED");
    }

    if (data->filesystems_switch)
        vars_upsert_after(content, "FILESYSTEMS_ENABLED",
                          adw_switch_row_get_active(data->filesystems_switch) ? "true" : "false",
                          "SYSTEM_SHELL");

    if (data->compression_switch)
        vars_upsert_after(content, "COMPRESSION_ENABLED",
                          adw_switch_row_get_active(data->compression_switch) ? "true" : "false",
                          "FILESYSTEMS_ENABLED");

    if (data->video_codecs_switch)
        vars_upsert_after(content, "VIDEO_CODECS_ENABLED",
                          adw_switch_row_get_active(data->video_codecs_switch) ? "true" : "false",
                          "COMPRESSION_ENABLED");
}

void save_system_variables_to_file(void)
{
    WindowSystemData *data = window_system_get_instance();
    if (!data) {
        LOG_ERROR("No se pudo obtener la instancia de WindowSystemData para guardar variables");
        return;
    }
    LOG_INFO("=== save_system_variables_to_file INICIADO ===");
    if (!vars_update(apply_system_vars, data))
        LOG_WARNING("No se pudo guardar variables del sistema");
    else
        LOG_INFO("=== save_system_variables_to_file FINALIZADO ===");
}

void window_system_update_language(WindowSystemData *data)
{
    if (!data) return;

    if (data->close_button)
        gtk_button_set_label(data->close_button,
            i18n_t("Cerrar", "Close", "Закрыть"));
    if (data->save_button)
        gtk_button_set_label(data->save_button,
            i18n_t("Guardar", "Save", "Сохранить"));
    if (data->window_title)
        adw_window_title_set_title(data->window_title,
            i18n_t("Aplicaciones Base", "Base Applications", "Базовые приложения"));
    if (data->shell_combo)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->shell_combo),
            i18n_t("Shell del sistema", "System Shell", "Системная оболочка"));
    if (data->filesystems_switch) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->filesystems_switch),
            i18n_t("Sistemas de archivos", "File Systems", "Файловые системы"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(data->filesystems_switch),
            i18n_t("Lectura y formateo de todo tipo de discos                Android, Btrfs, VFAT, ReiserFS, exFat, etc",
                   "Read and format all types of disks — Android, Btrfs, VFAT, ReiserFS, exFat, etc",
                   "Чтение и форматирование всех типов дисков — Android, Btrfs, VFAT, ReiserFS, exFat и др."));
    }
    if (data->compression_switch) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->compression_switch),
            i18n_t("Compresión y Descompresión de archivos",
                   "File Compression and Decompression",
                   "Сжатие и распаковка файлов"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(data->compression_switch),
            i18n_t("7zip, tar, unrar, zip, unarchiver",
                   "7zip, tar, unrar, zip, unarchiver",
                   "7zip, tar, unrar, zip, unarchiver"));
    }
    if (data->video_codecs_switch) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->video_codecs_switch),
            i18n_t("Códecs de video", "Video Codecs", "Видеокодеки"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(data->video_codecs_switch),
            i18n_t("Lectura de todos los formatos de vídeo",
                   "Read all video formats",
                   "Воспроизведение всех видеоформатов"));
    }
}
