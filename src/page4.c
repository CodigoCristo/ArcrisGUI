#include "page4.h"
#include "page3.h"
#include "disk_manager.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <udisks/udisks.h>

// Variable global para datos de la página 4
static Page4Data *g_page4_data = NULL;

// Funciones privadas

static void page4_connect_signals(Page4Data *data);
static gchar* page4_format_disk_size(guint64 size_bytes);
static gboolean page4_validate_selection(Page4Data *data);

// Función principal de inicialización de la página 4
void page4_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page4_data = g_malloc0(sizeof(Page4Data));
    
    // Guardar referencias importantes
    g_page4_data->carousel = carousel;
    g_page4_data->revealer = revealer;
    
    // Cargar la página 4 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page4.ui");
    GtkWidget *page4 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page4"));
    
    if (!page4) {
        LOG_ERROR("No se pudo cargar la página 4 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }
    
    // Obtener widgets específicos de la página
    g_page4_data->main_content = page4;
    g_page4_data->disk_label_page4 = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_label_page4"));
    g_page4_data->disk_size_label_page4 = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_size_label_page4"));
    g_page4_data->gparted_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "gparted_button"));
    g_page4_data->refresh_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "refresh_button"));
    
    // Verificar que todos los widgets se obtuvieron correctamente
    if (!g_page4_data->disk_label_page4 || !g_page4_data->disk_size_label_page4 || 
        !g_page4_data->gparted_button || !g_page4_data->refresh_button) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de la página 4");
        g_object_unref(page_builder);
        return;
    }
    
    // Realizar configuraciones iniciales específicas de la página 4
    page4_setup_widgets(g_page4_data);
    page4_load_data(g_page4_data);
    
    // Conectar señales
    page4_connect_signals(g_page4_data);
    
    // Crear botones de navegación
    page4_create_navigation_buttons(g_page4_data);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page4);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
    
    LOG_INFO("Página 4 (Información del Disco) inicializada correctamente");
}

// Función de limpieza
void page4_cleanup(Page4Data *data)
{
    if (g_page4_data) {
        g_free(g_page4_data->current_disk_path);
        g_free(g_page4_data->current_disk_size);
        g_free(g_page4_data);
        g_page4_data = NULL;
        LOG_INFO("Página 4 limpiada correctamente");
    }
}

// Función para configurar widgets
void page4_setup_widgets(Page4Data *data)
{
    if (!data) return;
    
    // Configurar estados iniciales de los widgets
    gtk_label_set_text(data->disk_label_page4, "Disco no seleccionado");
    gtk_label_set_text(data->disk_size_label_page4, "0 GB");
    
    LOG_INFO("Widgets de la página 4 configurados");
}

// Función para cargar datos
void page4_load_data(Page4Data *data)
{
    if (!data) return;
    
    // No cargar información al inicio, esperar a que se navegue a la página
    LOG_INFO("Datos de la página 4 configurados para carga posterior");
}

// Función para actualizar información del disco
void page4_update_disk_info(Page4Data *data)
{
    if (!data) {
        LOG_ERROR("page4_update_disk_info: data es NULL");
        return;
    }
    
    gchar *selected_disk_temp = NULL;
    const char *selected_disk = NULL;
    
    LOG_INFO("=== INICIANDO ACTUALIZACIÓN DE DISCO EN PAGE4 ===");
    
    // Método 1: Obtener el disco seleccionado desde page3
    selected_disk = page3_get_selected_disk();
    LOG_INFO("Método 1 - page3_get_selected_disk(): '%s'", selected_disk ? selected_disk : "NULL");
    
    // Método 2: Leer directamente desde variables.sh como fallback
    if (!selected_disk || strlen(selected_disk) == 0) {
        LOG_INFO("Método 1 falló, intentando Método 2 - leyendo variables.sh");
        
        gchar *variables_path = g_build_filename("./data", "variables.sh", NULL);
        gchar *content = NULL;
        
        LOG_INFO("Intentando leer archivo: %s", variables_path);
        
        if (g_file_get_contents(variables_path, &content, NULL, NULL)) {
            LOG_INFO("Archivo variables.sh leído exitosamente");
            gchar **lines = g_strsplit(content, "\n", -1);
            
            for (int i = 0; lines[i] != NULL; i++) {
                if (g_str_has_prefix(lines[i], "SELECTED_DISK=")) {
                    gchar *value = lines[i] + 14; // Saltar "SELECTED_DISK="
                    LOG_INFO("Línea encontrada: %s", lines[i]);
                    
                    // Remover comillas
                    if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                        value[strlen(value)-1] = 0;
                        value++;
                    }
                    
                    if (strlen(value) > 0) {
                        selected_disk_temp = g_strdup(value);
                        selected_disk = selected_disk_temp;
                        LOG_INFO("SELECTED_DISK encontrado: '%s'", selected_disk);
                        break;
                    }
                }
            }
            
            g_strfreev(lines);
            g_free(content);
        } else {
            LOG_ERROR("No se pudo leer el archivo variables.sh");
        }
        
        g_free(variables_path);
    }
    
    // Verificar si finalmente tenemos un disco seleccionado
    if (!selected_disk || strlen(selected_disk) == 0) {
        LOG_WARNING("No se pudo obtener disco seleccionado por ningún método");
        
        // Limpiar información anterior y mostrar estado vacío
        gtk_label_set_text(data->disk_label_page4, "Disco no seleccionado");
        gtk_label_set_text(data->disk_size_label_page4, "0 GB");
        
        g_free(data->current_disk_path);
        g_free(data->current_disk_size);
        data->current_disk_path = NULL;
        data->current_disk_size = NULL;
        
        g_free(selected_disk_temp);
        LOG_INFO("=== FIN ACTUALIZACIÓN (SIN DISCO) ===");
        return;
    }
    
    LOG_INFO("Disco final seleccionado: '%s'", selected_disk);
    
    // Actualizar información del disco en la estructura
    g_free(data->current_disk_path);
    data->current_disk_path = g_strdup(selected_disk);
    
    // Actualizar label del disco con formato mejorado
    gchar *disk_label_text = g_strdup_printf("Disco %s", selected_disk);
    gtk_label_set_text(data->disk_label_page4, disk_label_text);
    LOG_INFO("Label del disco actualizado a: '%s'", disk_label_text);
    g_free(disk_label_text);
    
    // Obtener y actualizar tamaño del disco
    LOG_INFO("Obteniendo tamaño del disco...");
    gchar *disk_size = page4_get_disk_size(selected_disk);
    if (disk_size && strlen(disk_size) > 0) {
        g_free(data->current_disk_size);
        data->current_disk_size = g_strdup(disk_size);
        gtk_label_set_text(data->disk_size_label_page4, disk_size);
        LOG_INFO("Tamaño del disco actualizado a: '%s'", disk_size);
        g_free(disk_size);
    } else {
        // Fallback si no se puede obtener el tamaño
        gtk_label_set_text(data->disk_size_label_page4, "Calculando...");
        LOG_WARNING("No se pudo obtener el tamaño del disco");
    }
    
    // Forzar actualización visual de los widgets
    gtk_widget_queue_draw(GTK_WIDGET(data->disk_label_page4));
    gtk_widget_queue_draw(GTK_WIDGET(data->disk_size_label_page4));
    
    // Cleanup
    g_free(selected_disk_temp);
    
    LOG_INFO("=== ACTUALIZACIÓN DE DISCO COMPLETADA ===");
    LOG_INFO("Disco almacenado: '%s'", data->current_disk_path ? data->current_disk_path : "NULL");
    LOG_INFO("Tamaño almacenado: '%s'", data->current_disk_size ? data->current_disk_size : "NULL");
}

// Función para obtener el tamaño del disco
gchar* page4_get_disk_size(const gchar *disk_path)
{
    if (!disk_path) return NULL;
    
    UDisksClient *client = udisks_client_new_sync(NULL, NULL);
    if (!client) {
        LOG_ERROR("No se pudo crear el cliente UDisks2");
        return g_strdup("Error");
    }
    
    // Intentar obtener información del disco usando una aproximación más simple
    // Ya que udisks_client_get_objects no está disponible, usamos un enfoque básico
    
    gchar *size_str = NULL;
    
    // Intentar leer el tamaño desde /sys/block si es posible
    if (g_str_has_prefix(disk_path, "/dev/")) {
        gchar *device_name = g_path_get_basename(disk_path);
        gchar *size_file = g_strdup_printf("/sys/block/%s/size", device_name);
        
        gchar *size_content = NULL;
        if (g_file_get_contents(size_file, &size_content, NULL, NULL)) {
            guint64 sectors = g_ascii_strtoull(size_content, NULL, 10);
            // Cada sector son 512 bytes
            guint64 size_bytes = sectors * 512;
            size_str = page4_format_disk_size(size_bytes);
            g_free(size_content);
        }
        
        g_free(device_name);
        g_free(size_file);
    }
    
    g_object_unref(client);
    
    if (!size_str) {
        size_str = g_strdup("Tamaño desconocido");
    }
    
    return size_str;
}

// Función para formatear el tamaño del disco (usando mismo método que DiskManager)
static gchar* page4_format_disk_size(guint64 size_bytes)
{
    // Usar el mismo método que DiskManager: size / 1000000000.0 para GB
    if (size_bytes >= 1000000000000ULL) {
        return g_strdup_printf("%.0f TB", size_bytes / 1000000000000.0);
    } else if (size_bytes >= 1000000000ULL) {
        return g_strdup_printf("%.0f GB", size_bytes / 1000000000.0);
    } else if (size_bytes >= 1000000ULL) {
        return g_strdup_printf("%.0f MB", size_bytes / 1000000.0);
    } else if (size_bytes >= 1000ULL) {
        return g_strdup_printf("%.0f KB", size_bytes / 1000.0);
    } else {
        return g_strdup_printf("%lu bytes", size_bytes);
    }
}

// Función para conectar señales
static void page4_connect_signals(Page4Data *data)
{
    if (!data) return;
    
    // Conectar señal del botón de Gparted
    g_signal_connect(data->gparted_button, "clicked", 
                     G_CALLBACK(on_page4_gparted_button_clicked), data);
    
    // Conectar señal del botón de refresh
    g_signal_connect(data->refresh_button, "clicked", 
                     G_CALLBACK(on_page4_refresh_clicked), data);
    
    LOG_INFO("Señales conectadas para página 4");
}

// Función para abrir Gparted
void page4_open_gparted(void)
{
    LOG_INFO("Abriendo Gparted...");
    
    GError *error = NULL;
    gboolean success = FALSE;
    
    // Intentar diferentes comandos para abrir Gparted
    const gchar *gparted_commands[] = {
        "pkexec gparted", // Usar pkexec para permisos de root
        "sudo gparted",   // Fallback con sudo
        "gparted",        // Fallback directo
        NULL
    };
    
    for (int i = 0; gparted_commands[i] != NULL; i++) {
        error = NULL;
        success = g_spawn_command_line_async(gparted_commands[i], &error);
        
        if (success) {
            LOG_INFO("Gparted ejecutado exitosamente con comando: %s", gparted_commands[i]);
            break;
        } else {
            LOG_WARNING("No se pudo ejecutar '%s': %s", gparted_commands[i], error->message);
            g_error_free(error);
        }
    }
    
    if (!success) {
        LOG_ERROR("No se pudo abrir Gparted con ningún método");
        
        // Mostrar mensaje de error al usuario
        AdwDialog *dialog = adw_alert_dialog_new(
            "Error al abrir Gparted",
            "No se pudo ejecutar Gparted. Asegúrate de que esté instalado y tengas permisos de administrador."
        );
        
        adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "ok", "Aceptar");
        adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "ok");
        adw_dialog_present(dialog, NULL);
    }
}

// Validar selección antes de continuar
static gboolean page4_validate_selection(Page4Data *data)
{
    if (!data) return FALSE;
    
    // Verificar que hay información del disco
    if (!data->current_disk_path) {
        LOG_WARNING("No hay información del disco en página 4");
        return FALSE;
    }
    
    return TRUE;
}

// Función para avanzar a la siguiente página
gboolean page4_go_to_next_page(Page4Data *data)
{
    if (!data) return FALSE;
    
    // Validar que hay información del disco
    if (!page4_validate_selection(data)) {
        return FALSE;
    }
    
    LOG_INFO("Avanzando desde página 4 con disco: %s", data->current_disk_path);
    
    // Ir a la siguiente página (asumiendo que es page5 o la que corresponda)
    // Por ahora, como no hay más páginas, mantenemos en la página actual
    LOG_INFO("Función de avance desde página 4 implementada");
    
    return TRUE;
}

// Función para ir a la página anterior
gboolean page4_go_to_previous_page(Page4Data *data)
{
    if (!data) return FALSE;
    
    LOG_INFO("Regresando a página anterior desde página 4");
    
    // Ir a la página anterior en el carousel (page3, índice 2)
    if (data->carousel) {
        GtkWidget *page3 = adw_carousel_get_nth_page(data->carousel, 2);
        if (page3) {
            adw_carousel_scroll_to(data->carousel, page3, TRUE);
        } else {
            LOG_ERROR("No se pudo encontrar page3 en el carousel");
        }
    }
    
    return TRUE;
}

// Función para crear botones de navegación programáticamente
void page4_create_navigation_buttons(Page4Data *data)
{
    if (!data || !data->main_content) return;
    
    // Crear el contenedor de navegación
    GtkWidget *navigation_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(navigation_box, 24);
    gtk_widget_set_halign(navigation_box, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(navigation_box, TRUE);
    
    // Crear botón atrás
    GtkButton *back_button = GTK_BUTTON(gtk_button_new_with_label("Atrás"));
    gtk_widget_set_halign(GTK_WIDGET(back_button), GTK_ALIGN_START);
    gtk_widget_add_css_class(GTK_WIDGET(back_button), "pill");
    
    // Crear espaciador
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    
    // Crear botón siguiente
    GtkButton *next_button = GTK_BUTTON(gtk_button_new_with_label("Siguiente"));
    gtk_widget_set_halign(GTK_WIDGET(next_button), GTK_ALIGN_END);
    gtk_widget_add_css_class(GTK_WIDGET(next_button), "pill");
    gtk_widget_add_css_class(GTK_WIDGET(next_button), "suggested-action");
    
    // Agregar widgets al contenedor
    gtk_box_append(GTK_BOX(navigation_box), GTK_WIDGET(back_button));
    gtk_box_append(GTK_BOX(navigation_box), spacer);
    gtk_box_append(GTK_BOX(navigation_box), GTK_WIDGET(next_button));
    
    // Conectar señales
    g_signal_connect(back_button, "clicked", 
                     G_CALLBACK(on_page4_back_button_clicked), data);
    g_signal_connect(next_button, "clicked", 
                     G_CALLBACK(on_page4_next_button_clicked), data);
    
    // Buscar el contenedor principal de page4 y agregar navegación
    GtkWidget *status_page = gtk_widget_get_first_child(data->main_content);
    if (status_page) {
        GtkWidget *status_child = gtk_widget_get_first_child(status_page);
        if (status_child && GTK_IS_BOX(status_child)) {
            gtk_box_append(GTK_BOX(status_child), navigation_box);
        }
    }
    
    LOG_INFO("Botones de navegación creados para página 4");
}

// Función para refrescar información del disco (para uso externo)
void page4_refresh_disk_info(void)
{
    if (!g_page4_data) return;
    page4_update_disk_info(g_page4_data);
}

// Función para verificar si la configuración es válida
gboolean page4_is_configuration_valid(void)
{
    if (!g_page4_data) return FALSE;
    return page4_validate_selection(g_page4_data);
}

// Callbacks

// Callback para el botón de Gparted
void on_page4_gparted_button_clicked(GtkButton *button, gpointer user_data)
{
    LOG_INFO("Botón Gparted presionado en página 4");
    page4_open_gparted();
}

// Callback para el botón de refresh
void on_page4_refresh_clicked(GtkButton *button, gpointer user_data)
{
    LOG_INFO("Actualizando información del disco desde página 4");
    if (g_page4_data) {
        page4_update_disk_info(g_page4_data);
    }
}

// Callback para el botón de siguiente
void on_page4_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;
    
    if (page4_go_to_next_page(data)) {
        LOG_INFO("Navegación exitosa desde página 4");
    } else {
        LOG_WARNING("No se pudo navegar desde página 4");
    }
}

// Callback para el botón atrás
void on_page4_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;
    
    if (page4_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 4");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 4");
    }
}

// Función pública para actualizar la página cuando se navega a ella
void page4_on_page_shown(void)
{
    if (!g_page4_data) return;
    
    LOG_INFO("Página 4 mostrada, actualizando información del disco");
    page4_update_disk_info(g_page4_data);
}

// Función de prueba para verificar actualización de información del disco
void page4_test_update(void)
{
    if (!g_page4_data) {
        LOG_ERROR("page4_test_update: g_page4_data es NULL");
        return;
    }
    
    LOG_INFO("=== INICIANDO TEST DE ACTUALIZACIÓN PAGE4 ===");
    
    // Forzar actualización de información del disco
    page4_update_disk_info(g_page4_data);
    
    // Verificar contenido actual de los labels
    const gchar *disk_label_text = gtk_label_get_text(g_page4_data->disk_label_page4);
    const gchar *disk_size_text = gtk_label_get_text(g_page4_data->disk_size_label_page4);
    
    LOG_INFO("Contenido actual del disk_label_page4: '%s'", disk_label_text);
    LOG_INFO("Contenido actual del disk_size_label_page4: '%s'", disk_size_text);
    
    // Verificar disco seleccionado actual
    const char *selected_disk = page3_get_selected_disk();
    LOG_INFO("Disco seleccionado según page3: '%s'", selected_disk ? selected_disk : "NULL");
    
    // Verificar información almacenada en page4
    LOG_INFO("Disco almacenado en page4: '%s'", g_page4_data->current_disk_path ? g_page4_data->current_disk_path : "NULL");
    LOG_INFO("Tamaño almacenado en page4: '%s'", g_page4_data->current_disk_size ? g_page4_data->current_disk_size : "NULL");
    
    LOG_INFO("=== FIN TEST DE ACTUALIZACIÓN PAGE4 ===");
}