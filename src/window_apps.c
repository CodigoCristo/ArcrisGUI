#include "window_apps.h"
#include "config.h"
#include <glib/gstdio.h>
#include <string.h>

// Instancia global
static WindowAppsData *global_apps_data = NULL;

// Constantes
#define VARIABLES_FILE_PATH "./data/variables.sh"

WindowAppsData* window_apps_new(void)
{
    if (global_apps_data) {
        return global_apps_data;
    }
    
    WindowAppsData *data = g_new0(WindowAppsData, 1);
    data->is_initialized = FALSE;
    data->selected_apps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    
    global_apps_data = data;
    return data;
}

void window_apps_init(WindowAppsData *data)
{
    if (!data || data->is_initialized) return;
    
    LOG_INFO("Inicializando ventana de utilities apps");
    
    // Crear builder y cargar UI
    data->builder = gtk_builder_new();
    GError *error = NULL;
    
    if (!gtk_builder_add_from_resource(data->builder, "/org/gtk/arcris/window_apps.ui", &error)) {
        LOG_ERROR("Error cargando UI de utilities apps: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    }
    
    // Cargar widgets
    window_apps_load_widgets_from_builder(data);
    
    // Configurar widgets
    window_apps_setup_widgets(data);
    
    // Configurar búsqueda
    window_apps_setup_search(data);
    
    // Conectar señales
    window_apps_connect_signals(data);
    
    // Cargar aplicaciones guardadas
    window_apps_load_selected_apps_from_file(data);
    
    data->is_initialized = TRUE;
    LOG_INFO("Ventana de utilities apps inicializada correctamente");
}

void window_apps_cleanup(WindowAppsData *data)
{
    if (!data) return;
    
    LOG_INFO("Limpiando ventana de utilities apps");
    
    if (data->selected_apps) {
        g_hash_table_destroy(data->selected_apps);
        data->selected_apps = NULL;
    }
    
    if (data->builder) {
        g_object_unref(data->builder);
        data->builder = NULL;
    }
    
    if (data->window) {
        gtk_window_destroy(data->window);
        data->window = NULL;
    }
    
    data->is_initialized = FALSE;
}

void window_apps_show(WindowAppsData *data, GtkWindow *parent)
{
    if (!data) return;
    
    if (!data->is_initialized) {
        window_apps_init(data);
    }
    
    if (!data->window) {
        LOG_ERROR("No se pudo mostrar la ventana de utilities apps: ventana no inicializada");
        return;
    }
    
    // Configurar ventana padre
    if (parent) {
        gtk_window_set_transient_for(data->window, parent);
        gtk_window_set_modal(data->window, TRUE);
    }
    
    // Cargar aplicaciones seleccionadas actuales
    window_apps_load_selected_apps_from_file(data);
    
    // Mostrar la ventana
    gtk_window_present(data->window);
    
    LOG_INFO("Ventana de utilities apps mostrada");
}

void window_apps_hide(WindowAppsData *data)
{
    if (!data || !data->window) return;
    
    gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
    LOG_INFO("Ventana de utilities apps ocultada");
}

void window_apps_load_widgets_from_builder(WindowAppsData *data)
{
    if (!data || !data->builder) return;
    
    // Cargar ventana principal
    data->window = GTK_WINDOW(gtk_builder_get_object(data->builder, "ProgramExtraWindow"));
    if (!data->window) {
        LOG_ERROR("No se pudo cargar la ventana principal de utilities apps");
        return;
    }
    
    // Cargar botones
    data->close_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));
    
    // Cargar entrada de búsqueda
    data->search_entry = GTK_SEARCH_ENTRY(gtk_builder_get_object(data->builder, "searchApp"));
    
    // Cargar expanderes
    data->browsers_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "browsers_expander"));
    data->graphics_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "graphics_expander"));
    data->video_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "video_expander"));
    
    if (!data->close_button) LOG_WARNING("No se pudo cargar close_button");
    if (!data->save_button) LOG_WARNING("No se pudo cargar save_button");
    if (!data->search_entry) LOG_WARNING("No se pudo cargar searchApp");
    if (!data->browsers_expander) LOG_WARNING("No se pudo cargar browsers_expander");
    if (!data->graphics_expander) LOG_WARNING("No se pudo cargar graphics_expander");
    if (!data->video_expander) LOG_WARNING("No se pudo cargar video_expander");
    
    LOG_INFO("Widgets de ventana de utilities apps cargados desde builder");
}

void window_apps_setup_widgets(WindowAppsData *data)
{
    if (!data) return;
    
    // Configurar ventana
    if (data->window) {
        gtk_window_set_title(data->window, "Utilities Apps");
        gtk_window_set_default_size(data->window, 900, 630);
        gtk_window_set_resizable(data->window, FALSE);
    }
    
    LOG_INFO("Widgets de ventana de utilities apps configurados");
}

void window_apps_setup_search(WindowAppsData *data)
{
    if (!data || !data->search_entry) return;
    
    // Configurar propiedades de la entrada de búsqueda
    gtk_search_entry_set_placeholder_text(data->search_entry, "Busca tu aplicación");
    
    LOG_INFO("Búsqueda de utilities apps configurada");
}

void window_apps_connect_signals(WindowAppsData *data)
{
    if (!data) return;
    
    // Conectar señales de botones
    if (data->close_button) {
        g_signal_connect(data->close_button, "clicked", 
                        G_CALLBACK(on_apps_close_button_clicked), data);
    }
    
    if (data->save_button) {
        g_signal_connect(data->save_button, "clicked", 
                        G_CALLBACK(on_apps_save_button_clicked), data);
    }
    
    // Conectar señales de búsqueda
    if (data->search_entry) {
        g_signal_connect(data->search_entry, "search-changed", 
                        G_CALLBACK(on_apps_search_changed), data);
    }
    
    LOG_INFO("Señales de ventana de utilities apps conectadas");
}

void window_apps_filter_apps(WindowAppsData *data, const gchar *search_text)
{
    if (!data) return;
    
    // Implementar filtrado de aplicaciones basado en el texto de búsqueda
    // Por ahora, simplemente logeamos la búsqueda
    if (search_text && strlen(search_text) > 0) {
        LOG_INFO("Filtrando apps con texto: %s", search_text);
    } else {
        LOG_INFO("Mostrando todas las apps");
    }
}

gboolean window_apps_load_selected_apps_from_file(WindowAppsData *data)
{
    if (!data || !data->selected_apps) return FALSE;
    
    GError *error = NULL;
    gchar *content = NULL;
    gsize length;
    
    if (g_file_get_contents(VARIABLES_FILE_PATH, &content, &length, &error)) {
        // Buscar la línea UTILITIES_APPS=
        gchar **lines = g_strsplit(content, "\n", -1);
        
        for (int i = 0; lines[i] != NULL; i++) {
            gchar *line = g_strstrip(lines[i]);
            if (g_str_has_prefix(line, "UTILITIES_APPS=(")) {
                // Extraer contenido del array
                gchar *start = strchr(line, '(');
                gchar *end = strrchr(line, ')');
                if (start && end && end > start) {
                    start++; // Saltar el '('
                    *end = '\0'; // Terminar en ')'
                    
                    // Convertir array bash a hash table
                    gchar **apps = g_strsplit(start, " ", -1);
                    
                    // Limpiar hash table anterior
                    g_hash_table_remove_all(data->selected_apps);
                    
                    for (int j = 0; apps[j] != NULL; j++) {
                        gchar *app = g_strstrip(apps[j]);
                        // Remover comillas si las hay
                        if (g_str_has_prefix(app, "\"") && g_str_has_suffix(app, "\"")) {
                            app[strlen(app)-1] = '\0';
                            app++;
                        }
                        if (strlen(app) > 0) {
                            g_hash_table_insert(data->selected_apps, g_strdup(app), g_strdup("selected"));
                        }
                    }
                    
                    g_strfreev(apps);
                }
                break;
            }
        }
        
        g_strfreev(lines);
        g_free(content);
        LOG_INFO("Utilities apps cargadas desde variables.sh");
        return TRUE;
    } else {
        if (error) {
            LOG_INFO("No se pudo cargar archivo variables.sh: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }
}

gboolean window_apps_save_selected_apps_to_file(WindowAppsData *data)
{
    if (!data || !data->selected_apps) return FALSE;
    
    // Leer archivo variables.sh actual
    GError *error = NULL;
    gchar *content = NULL;
    gsize length;
    
    if (!g_file_get_contents(VARIABLES_FILE_PATH, &content, &length, &error)) {
        LOG_ERROR("No se pudo leer variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }
    
    // Crear contenido del array
    GString *array_content = g_string_new("UTILITIES_APPS=(");
    
    if (g_hash_table_size(data->selected_apps) > 0) {
        GHashTableIter iter;
        gpointer key, value;
        gboolean first = TRUE;
        
        g_hash_table_iter_init(&iter, data->selected_apps);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            if (!first) g_string_append(array_content, " ");
            g_string_append_printf(array_content, "\"%s\"", (gchar*)key);
            first = FALSE;
        }
    }
    
    g_string_append(array_content, ")");
    
    // Buscar y reemplazar línea UTILITIES_APPS o agregarla
    gchar **lines = g_strsplit(content, "\n", -1);
    GString *new_content = g_string_new("");
    gboolean found = FALSE;
    
    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(g_strstrip(lines[i]), "UTILITIES_APPS=")) {
            g_string_append_printf(new_content, "%s\n", array_content->str);
            found = TRUE;
        } else {
            g_string_append_printf(new_content, "%s\n", lines[i]);
        }
    }
    
    // Si no se encontró, agregar al final
    if (!found) {
        g_string_append_printf(new_content, "\n# Utilities apps seleccionadas por el usuario\n%s\n", array_content->str);
    }
    
    // Guardar archivo actualizado
    gboolean success = g_file_set_contents(VARIABLES_FILE_PATH, new_content->str, -1, &error);
    
    if (success) {
        LOG_INFO("Utilities apps guardadas como array en variables.sh");
    } else {
        LOG_ERROR("Error guardando en variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    }
    
    // Limpiar memoria
    g_string_free(array_content, TRUE);
    g_string_free(new_content, TRUE);
    g_strfreev(lines);
    g_free(content);
    
    return success;
}

GHashTable* window_apps_get_selected_apps(WindowAppsData *data)
{
    if (!data) return NULL;
    return data->selected_apps;
}

void window_apps_set_selected_apps(WindowAppsData *data, GHashTable *apps)
{
    if (!data || !apps) return;
    
    if (data->selected_apps) {
        g_hash_table_destroy(data->selected_apps);
    }
    
    data->selected_apps = g_hash_table_ref(apps);
}

// Callbacks

void on_apps_close_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowAppsData *data = (WindowAppsData*)user_data;
    if (!data) return;
    
    LOG_INFO("Cerrando ventana de utilities apps");
    window_apps_hide(data);
}

void on_apps_save_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowAppsData *data = (WindowAppsData*)user_data;
    if (!data) return;
    
    LOG_INFO("Guardando utilities apps seleccionadas");
    
    if (window_apps_save_selected_apps_to_file(data)) {
        LOG_INFO("Utilities apps guardadas exitosamente");
        // Cerrar ventana después de guardar
        window_apps_hide(data);
    } else {
        LOG_ERROR("Error al guardar utilities apps");
    }
}

void on_apps_search_changed(GtkSearchEntry *entry, gpointer user_data)
{
    WindowAppsData *data = (WindowAppsData*)user_data;
    if (!data) return;
    
    const gchar *search_text = gtk_editable_get_text(GTK_EDITABLE(entry));
    window_apps_filter_apps(data, search_text);
}

// Funciones de utilidad

void window_apps_reset_to_defaults(WindowAppsData *data)
{
    if (!data) return;
    
    if (data->selected_apps) {
        g_hash_table_remove_all(data->selected_apps);
    }
    
    LOG_INFO("Ventana de utilities apps reiniciada a valores por defecto");
}

WindowAppsData* window_apps_get_instance(void)
{
    if (!global_apps_data) {
        global_apps_data = window_apps_new();
    }
    return global_apps_data;
}