#include "window_program_extra.h"
#include "page7.h"
#include "config.h"
#include <glib/gstdio.h>
#include <string.h>

// Instancia global
static WindowProgramExtraData *global_program_extra_data = NULL;

// Constantes
#define VARIABLES_FILE_PATH "./data/variables.sh"

WindowProgramExtraData* window_program_extra_new(void)
{
    if (global_program_extra_data) {
        return global_program_extra_data;
    }
    
    WindowProgramExtraData *data = g_new0(WindowProgramExtraData, 1);
    data->is_initialized = FALSE;
    data->programs_text = NULL;
    
    global_program_extra_data = data;
    return data;
}

void window_program_extra_init(WindowProgramExtraData *data)
{
    if (!data || data->is_initialized) return;
    
    LOG_INFO("Inicializando ventana de programas extra");
    
    // Crear builder y cargar UI
    data->builder = gtk_builder_new();
    GError *error = NULL;
    
    if (!gtk_builder_add_from_resource(data->builder, "/org/gtk/arcris/window_program_extra.ui", &error)) {
        LOG_ERROR("Error cargando UI de programas extra: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    }
    
    // Cargar widgets
    window_program_extra_load_widgets_from_builder(data);
    
    // Configurar widgets
    window_program_extra_setup_widgets(data);
    
    // Configurar TextView
    window_program_extra_setup_textview(data);
    
    // Conectar señales
    window_program_extra_connect_signals(data);
    
    // Cargar programas guardados
    window_program_extra_load_programs_from_file(data);
    
    data->is_initialized = TRUE;
    LOG_INFO("Ventana de programas extra inicializada correctamente");
}

void window_program_extra_cleanup(WindowProgramExtraData *data)
{
    if (!data) return;
    
    LOG_INFO("Limpiando ventana de programas extra");
    
    if (data->programs_text) {
        g_free(data->programs_text);
        data->programs_text = NULL;
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

void window_program_extra_show(WindowProgramExtraData *data, GtkWindow *parent)
{
    if (!data) return;
    
    if (!data->is_initialized) {
        window_program_extra_init(data);
    }
    
    if (!data->window) {
        LOG_ERROR("No se pudo mostrar la ventana de programas extra: ventana no inicializada");
        return;
    }
    
    // Configurar ventana padre
    if (parent) {
        gtk_window_set_transient_for(data->window, parent);
        gtk_window_set_modal(data->window, TRUE);
    }
    
    // Cargar texto actual
    window_program_extra_load_programs_from_file(data);
    
    // Mostrar la ventana
    gtk_window_present(data->window);
    
    LOG_INFO("Ventana de programas extra mostrada");
}

void window_program_extra_hide(WindowProgramExtraData *data)
{
    if (!data || !data->window) return;
    
    gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
    LOG_INFO("Ventana de programas extra ocultada");
}

void window_program_extra_load_widgets_from_builder(WindowProgramExtraData *data)
{
    if (!data || !data->builder) return;
    
    // Cargar ventana principal
    data->window = GTK_WINDOW(gtk_builder_get_object(data->builder, "ProgramExtraWindow"));
    if (!data->window) {
        LOG_ERROR("No se pudo cargar la ventana principal de programas extra");
        return;
    }
    
    // Cargar botones
    data->close_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));
    
    // Cargar TextView
    data->hardware_textview = GTK_TEXT_VIEW(gtk_builder_get_object(data->builder, "programextra_textview"));
    
    if (!data->close_button) LOG_WARNING("No se pudo cargar close_button");
    if (!data->save_button) LOG_WARNING("No se pudo cargar save_button");
    if (!data->hardware_textview) LOG_WARNING("No se pudo cargar programextra_textview");
    
    LOG_INFO("Widgets de ventana de programas extra cargados desde builder");
}

void window_program_extra_setup_widgets(WindowProgramExtraData *data)
{
    if (!data) return;
    
    // Configurar ventana
    if (data->window) {
        gtk_window_set_title(data->window, "Programas Extras");
        gtk_window_set_default_size(data->window, 550, 500);
        gtk_window_set_resizable(data->window, FALSE);
    }
    
    LOG_INFO("Widgets de ventana de programas extra configurados");
}

void window_program_extra_setup_textview(WindowProgramExtraData *data)
{
    if (!data || !data->hardware_textview) return;
    
    // Obtener el buffer del TextView
    data->text_buffer = gtk_text_view_get_buffer(data->hardware_textview);
    
    // Configurar propiedades del TextView principal
    gtk_text_view_set_wrap_mode(data->hardware_textview, GTK_WRAP_CHAR);
    gtk_text_view_set_editable(data->hardware_textview, TRUE);
    gtk_text_view_set_cursor_visible(data->hardware_textview, TRUE);
    
    LOG_INFO("TextView principal de programas extra configurado");
}

void window_program_extra_connect_signals(WindowProgramExtraData *data)
{
    if (!data) return;
    
    // Conectar señales de botones
    if (data->close_button) {
        g_signal_connect(data->close_button, "clicked", 
                        G_CALLBACK(on_program_extra_close_button_clicked), data);
    }
    
    if (data->save_button) {
        g_signal_connect(data->save_button, "clicked", 
                        G_CALLBACK(on_program_extra_save_button_clicked), data);
    }
    
    // Conectar señales del buffer
    if (data->text_buffer) {
        g_signal_connect(data->text_buffer, "changed", 
                        G_CALLBACK(on_program_extra_textbuffer_changed), data);
    }
    

    LOG_INFO("Señales de ventana de programas extra conectadas");
}



gboolean window_program_extra_load_programs_from_file(WindowProgramExtraData *data)
{
    if (!data || !data->text_buffer) return FALSE;
    
    GError *error = NULL;
    gchar *content = NULL;
    gsize length;
    
    if (g_file_get_contents(VARIABLES_FILE_PATH, &content, &length, &error)) {
        // Buscar la línea EXTRA_PROGRAMS=
        gchar **lines = g_strsplit(content, "\n", -1);
        gchar *programs_text = NULL;
        
        for (int i = 0; lines[i] != NULL; i++) {
            gchar *line = g_strstrip(lines[i]);
            if (g_str_has_prefix(line, "EXTRA_PROGRAMS=(")) {
                // Extraer contenido del array
                gchar *start = strchr(line, '(');
                gchar *end = strrchr(line, ')');
                if (start && end && end > start) {
                    start++; // Saltar el '('
                    *end = '\0'; // Terminar en ')'
                    
                    // Convertir array bash a texto simple
                    gchar **programs = g_strsplit(start, " ", -1);
                    GString *text = g_string_new("");
                    
                    for (int j = 0; programs[j] != NULL; j++) {
                        gchar *program = g_strstrip(programs[j]);
                        // Remover comillas si las hay
                        if (g_str_has_prefix(program, "\"") && g_str_has_suffix(program, "\"")) {
                            program[strlen(program)-1] = '\0';
                            program++;
                        }
                        if (strlen(program) > 0) {
                            if (text->len > 0) g_string_append(text, " ");
                            g_string_append(text, program);
                        }
                    }
                    
                    programs_text = g_string_free(text, FALSE);
                    g_strfreev(programs);
                }
                break;
            }
        }
        
        if (programs_text && strlen(g_strstrip(programs_text)) > 0) {
            gtk_text_buffer_set_text(data->text_buffer, programs_text, -1);
            
            if (data->programs_text) g_free(data->programs_text);
            data->programs_text = g_strdup(programs_text);
            
            // Actualizar subtitle en page7 al cargar texto existente
            page7_update_programas_extras_subtitle(programs_text);
        } else {
            // Si no hay texto, actualizar con subtitle por defecto
            page7_update_programas_extras_subtitle(NULL);
        }
        
        g_strfreev(lines);
        g_free(content);
        if (programs_text) g_free(programs_text);
        LOG_INFO("Programas cargados desde variables.sh");
        return TRUE;
    } else {
        if (error) {
            LOG_INFO("No se pudo cargar archivo variables.sh: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }
}

gboolean window_program_extra_save_programs_to_file(WindowProgramExtraData *data)
{
    if (!data || !data->text_buffer) return FALSE;
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(data->text_buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(data->text_buffer, &start, &end, FALSE);
    
    // Leer archivo variables.sh actual
    GError *error = NULL;
    gchar *content = NULL;
    gsize length;
    
    if (!g_file_get_contents(VARIABLES_FILE_PATH, &content, &length, &error)) {
        LOG_ERROR("No se pudo leer variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        if (text) g_free(text);
        return FALSE;
    }
    
    // Determinar si hay texto para PROGRAM_EXTRA
    gboolean has_program_text = (text && strlen(g_strstrip(text)) > 0);
    
    // Procesar texto para extraer palabras
    GString *array_content = g_string_new("EXTRA_PROGRAMS=(");
    
    if (has_program_text) {
        // Dividir texto en palabras (separados por espacios, tabs, saltos de línea)
        gchar **words = g_regex_split_simple("\\s+", g_strstrip(text), 0, 0);
        
        for (int i = 0; words[i] != NULL; i++) {
            gchar *word = g_strstrip(words[i]);
            if (strlen(word) > 0) {
                if (i > 0) g_string_append(array_content, " ");
                g_string_append_printf(array_content, "\"%s\"", word);
            }
        }
        
        g_strfreev(words);
    }
    
    g_string_append(array_content, ")");
    
    // Buscar y reemplazar líneas EXTRA_PROGRAMS y PROGRAM_EXTRA o agregarlas
    gchar **lines = g_strsplit(content, "\n", -1);
    GString *new_content = g_string_new("");
    gboolean found_extra_programs = FALSE;
    gboolean found_program_extra = FALSE;
    
    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(g_strstrip(lines[i]), "EXTRA_PROGRAMS=")) {
            g_string_append_printf(new_content, "%s\n", array_content->str);
            found_extra_programs = TRUE;
        } else if (g_str_has_prefix(g_strstrip(lines[i]), "PROGRAM_EXTRA=")) {
            g_string_append_printf(new_content, "PROGRAM_EXTRA=\"%s\"\n", has_program_text ? "true" : "false");
            found_program_extra = TRUE;
        } else {
            g_string_append_printf(new_content, "%s\n", lines[i]);
        }
    }
    
    // Si no se encontraron, agregar al final
    if (!found_extra_programs) {
        g_string_append_printf(new_content, "\n# Programas extra agregados por el usuario\n%s\n", array_content->str);
    }
    
    if (!found_program_extra) {
        g_string_append_printf(new_content, "PROGRAM_EXTRA=\"%s\"\n", has_program_text ? "true" : "false");
    }
    
    // Guardar archivo actualizado
    gboolean success = g_file_set_contents(VARIABLES_FILE_PATH, new_content->str, -1, &error);
    
    if (success) {
        if (data->programs_text) g_free(data->programs_text);
        data->programs_text = g_strdup(text ? text : "");
        LOG_INFO("Programas guardados como array en variables.sh");
        LOG_INFO("PROGRAM_EXTRA establecido a: %s", has_program_text ? "true" : "false");
    } else {
        LOG_ERROR("Error guardando en variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    }
    
    // Limpiar memoria
    g_string_free(array_content, TRUE);
    g_string_free(new_content, TRUE);
    g_strfreev(lines);
    g_free(content);
    if (text) g_free(text);
    
    return success;
}

gchar* window_program_extra_get_programs_text(WindowProgramExtraData *data)
{
    if (!data || !data->text_buffer) return NULL;
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(data->text_buffer, &start, &end);
    return gtk_text_buffer_get_text(data->text_buffer, &start, &end, FALSE);
}

void window_program_extra_set_programs_text(WindowProgramExtraData *data, const gchar *text)
{
    if (!data || !data->text_buffer) return;
    
    if (text) {
        gtk_text_buffer_set_text(data->text_buffer, text, -1);
    }
}

// Callbacks

void on_program_extra_close_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowProgramExtraData *data = (WindowProgramExtraData*)user_data;
    if (!data) return;
    
    LOG_INFO("Cerrando ventana de programas extra");
    window_program_extra_hide(data);
}

void on_program_extra_save_button_clicked(GtkButton *button, gpointer user_data)
{
    static gboolean is_saving = FALSE;
    
    WindowProgramExtraData *data = (WindowProgramExtraData*)user_data;
    if (!data) return;
    
    // Protección contra múltiples llamadas
    if (is_saving) {
        LOG_INFO("Save button ya se está procesando, ignorando llamada duplicada");
        return;
    }
    
    is_saving = TRUE;
    LOG_INFO("Guardando programas extra");
    
    if (window_program_extra_save_programs_to_file(data)) {
        LOG_INFO("Programas guardados exitosamente");
        
        // Obtener el texto actual del textview y actualizar subtitle
        gchar *programs_text = window_program_extra_get_programs_text(data);
        page7_update_programas_extras_subtitle(programs_text);
        
        if (programs_text) g_free(programs_text);
        LOG_INFO("Subtitle de page7 actualizado");
        
        // Cerrar ventana después de guardar
        window_program_extra_hide(data);
    } else {
        LOG_ERROR("Error al guardar programas");
    }
    
    is_saving = FALSE;
}



void on_program_extra_textbuffer_changed(GtkTextBuffer *buffer, gpointer user_data)
{
    WindowProgramExtraData *data = (WindowProgramExtraData*)user_data;
    if (!data || !buffer) return;
    
    // Obtener el texto actual del buffer
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    // Actualizar el subtitle de programas extras en page7 en tiempo real
    page7_update_programas_extras_subtitle(text);
    
    if (text) g_free(text);
}

// Funciones de utilidad

void window_program_extra_reset_to_defaults(WindowProgramExtraData *data)
{
    if (!data) return;
    
    if (data->programs_text) {
        g_free(data->programs_text);
        data->programs_text = NULL;
    }
    
    LOG_INFO("Ventana de programas extra reiniciada a valores por defecto");
}

gboolean window_program_extra_validate_programs_text(const gchar *text)
{
    if (!text) return FALSE;
    
    // Validación básica: no debe estar vacío
    gchar *stripped = g_strstrip(g_strdup(text));
    gboolean valid = (strlen(stripped) > 0);
    
    g_free(stripped);
    return valid;
}

WindowProgramExtraData* window_program_extra_get_instance(void)
{
    if (!global_program_extra_data) {
        global_program_extra_data = window_program_extra_new();
    }
    return global_program_extra_data;
}

