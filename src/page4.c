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
    g_page4_data->partitions_group = ADW_PREFERENCES_GROUP(gtk_builder_get_object(page_builder, "partitions_group"));
    
    // Verificar que todos los widgets se obtuvieron correctamente
    if (!g_page4_data->disk_label_page4 || !g_page4_data->disk_size_label_page4 || 
        !g_page4_data->gparted_button || !g_page4_data->refresh_button || !g_page4_data->partitions_group) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de la página 4");
        g_object_unref(page_builder);
        return;
    }
    
    // Inicializar cliente UDisks2 para obtener información de particiones
    GError *error = NULL;
    g_page4_data->udisks_client = udisks_client_new_sync(NULL, &error);
    if (!g_page4_data->udisks_client) {
        LOG_WARNING("No se pudo inicializar cliente UDisks2: %s", error ? error->message : "Error desconocido");
        if (error) g_error_free(error);
    } else {
        LOG_INFO("Cliente UDisks2 inicializado para page4");
    }
    
    // Inicializar lista de particiones
    g_page4_data->partitions = NULL;
    
    // Inicializar lista de filas de particiones
    g_page4_data->partition_rows = NULL;
    
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
        // Limpiar lista de particiones
        page4_clear_partitions(g_page4_data);
        
        // Limpiar lista de filas de particiones
        if (g_page4_data->partition_rows) {
            g_list_free(g_page4_data->partition_rows);
            g_page4_data->partition_rows = NULL;
        }
        
        // Limpiar cliente UDisks2
        if (g_page4_data->udisks_client) {
            g_object_unref(g_page4_data->udisks_client);
            g_page4_data->udisks_client = NULL;
        }
        
        g_free(g_page4_data->current_disk_path);
        g_free(g_page4_data->current_disk_size);
        g_free(g_page4_data);
        g_page4_data = NULL;
    }
}

// Función para configurar widgets
void page4_setup_widgets(Page4Data *data)
{
    if (!data) return;
    
    // Configurar estados iniciales de los widgets
    gtk_label_set_text(data->disk_label_page4, "Disco - 0 GB");
    gtk_label_set_text(data->disk_size_label_page4, "Disco no seleccionado");
    
    // Obtener el disco seleccionado automáticamente al inicio
    page4_update_disk_info(data);
    
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
        gtk_label_set_text(data->disk_label_page4, "Disco - 0 GB");
        gtk_label_set_text(data->disk_size_label_page4, "Disco no seleccionado");
        
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
    
    // Actualizar label del disco con la ruta
    gtk_label_set_text(data->disk_size_label_page4, selected_disk);
    LOG_INFO("Ruta del disco actualizada a: '%s'", selected_disk);
    
    // Obtener y actualizar tamaño del disco
    LOG_INFO("Obteniendo tamaño del disco...");
    gchar *disk_size = page4_get_disk_size(selected_disk);
    if (disk_size && strlen(disk_size) > 0) {
        g_free(data->current_disk_size);
        data->current_disk_size = g_strdup(disk_size);
        
        // Actualizar label del disco con formato "Disco - Tamaño"
        gchar *disk_label_text = g_strdup_printf("Disco - %s", disk_size);
        gtk_label_set_text(data->disk_label_page4, disk_label_text);
        LOG_INFO("Label del disco actualizado a: '%s'", disk_label_text);
        g_free(disk_label_text);
        
        LOG_INFO("Tamaño del disco actualizado a: '%s'", disk_size);
        g_free(disk_size);
    } else {
        // Fallback si no se puede obtener el tamaño
        gtk_label_set_text(data->disk_label_page4, "Disco - Calculando...");
        LOG_WARNING("No se pudo obtener el tamaño del disco");
    }
    
    // Forzar actualización visual de los widgets
    gtk_widget_queue_draw(GTK_WIDGET(data->disk_label_page4));
    gtk_widget_queue_draw(GTK_WIDGET(data->disk_size_label_page4));
    
    // Actualizar particiones del disco seleccionado
    page4_populate_partitions(data, selected_disk);
    
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

// =====================================================
// FUNCIONES PARA MANEJO DE PARTICIONES
// =====================================================

// Función de comparación para ordenar particiones por número
static gint partition_compare_func(gconstpointer a, gconstpointer b)
{
    const PartitionInfo *info_a = (const PartitionInfo*)a;
    const PartitionInfo *info_b = (const PartitionInfo*)b;
    
    if (!info_a || !info_b || !info_a->device_path || !info_b->device_path) {
        return 0;
    }
    
    // Extraer el nombre base del dispositivo (ej: sda1 -> sda1, nvme0n1p1 -> nvme0n1p1)
    gchar *basename_a = g_path_get_basename(info_a->device_path);
    gchar *basename_b = g_path_get_basename(info_b->device_path);
    
    gchar *num_start_a = NULL;
    gchar *num_start_b = NULL;
    
    // Para dispositivos nvme (formato: nvme0n1p1, nvme0n1p2, etc.)
    if (g_str_has_prefix(basename_a, "nvme") && strstr(basename_a, "p")) {
        num_start_a = strrchr(basename_a, 'p');
        if (num_start_a) num_start_a++; // Saltar la 'p'
    } else {
        // Para dispositivos tradicionales (formato: sda1, sdb2, etc.)
        // Encontrar donde comienzan los números
        num_start_a = basename_a;
        while (*num_start_a && !g_ascii_isdigit(*num_start_a)) {
            num_start_a++;
        }
    }
    
    // Mismo proceso para el segundo dispositivo
    if (g_str_has_prefix(basename_b, "nvme") && strstr(basename_b, "p")) {
        num_start_b = strrchr(basename_b, 'p');
        if (num_start_b) num_start_b++; // Saltar la 'p'
    } else {
        // Para dispositivos tradicionales
        num_start_b = basename_b;
        while (*num_start_b && !g_ascii_isdigit(*num_start_b)) {
            num_start_b++;
        }
    }
    
    // Si ambos tienen números, compararlos numéricamente
    gint result = 0;
    if (num_start_a && num_start_b && g_ascii_isdigit(*num_start_a) && g_ascii_isdigit(*num_start_b)) {
        gint num_a = atoi(num_start_a);
        gint num_b = atoi(num_start_b);
        result = num_a - num_b;
    } else {
        // Si no tienen números, comparar alfabéticamente
        result = g_strcmp0(basename_a, basename_b);
    }
    
    g_free(basename_a);
    g_free(basename_b);
    
    return result;
}

// Función para limpiar todas las particiones
void page4_clear_partitions(Page4Data *data)
{
    if (!data) return;
    
    LOG_INFO("Iniciando limpieza de particiones...");
    
    // Primero limpiar las filas de particiones usando las referencias almacenadas
    if (data->partition_rows) {
        LOG_INFO("Eliminando %d filas de particiones", g_list_length(data->partition_rows));
        
        GList *current_row = data->partition_rows;
        while (current_row) {
            AdwActionRow *row = ADW_ACTION_ROW(current_row->data);
            if (row && GTK_IS_WIDGET(row)) {
                // Remover la fila del grupo de particiones
                adw_preferences_group_remove(data->partitions_group, GTK_WIDGET(row));
                LOG_INFO("Fila de partición removida");
            }
            current_row = current_row->next;
        }
        
        // Limpiar la lista de referencias a filas
        g_list_free(data->partition_rows);
        data->partition_rows = NULL;
    }
    
    // Limpiar lista de información de particiones
    if (data->partitions) {
        GList *current = data->partitions;
        while (current) {
            PartitionInfo *info = (PartitionInfo*)current->data;
            page4_free_partition_info(info);
            current = current->next;
        }
        g_list_free(data->partitions);
        data->partitions = NULL;
    }
    
    LOG_INFO("Particiones limpiadas correctamente");
}

// Función para obtener particiones de un disco
void page4_populate_partitions(Page4Data *data, const gchar *disk_path)
{
    if (!data || !disk_path || !data->udisks_client) {
        LOG_WARNING("page4_populate_partitions: parámetros inválidos");
        return;
    }
    
    LOG_INFO("Obteniendo particiones del disco: %s", disk_path);
    
    // Limpiar particiones anteriores
    page4_clear_partitions(data);
    
    LOG_INFO("Particiones anteriores limpiadas, iniciando búsqueda...");
    
    // Obtener todos los objetos de UDisks2
    GList *objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(data->udisks_client));
    
    int partition_count = 0;
    for (GList *l = objects; l != NULL; l = l->next) {
        UDisksObject *object = UDISKS_OBJECT(l->data);
        UDisksPartition *partition = udisks_object_peek_partition(object);
        UDisksBlock *block = udisks_object_peek_block(object);
        
        if (partition && block) {
            const gchar *partition_device = udisks_block_get_device(block);
            
            // Verificar si esta partición pertenece al disco seleccionado
            if (partition_device && page4_is_partition_of_disk(partition_device, disk_path)) {
                LOG_INFO("Partición encontrada: %s", partition_device);
                
                // Crear información de la partición
                PartitionInfo *info = page4_create_partition_info(partition_device, partition, block, object);
                if (info) {
                    // Añadir a la lista (sin añadir al grupo todavía)
                    data->partitions = g_list_append(data->partitions, info);
                    
                    partition_count++;
                }
            }
        }
    }
    
    g_list_free_full(objects, g_object_unref);
    
    // Ordenar la lista de particiones por número
    if (data->partitions) {
        data->partitions = g_list_sort(data->partitions, partition_compare_func);
        LOG_INFO("Particiones ordenadas por número");
        
        // Limpiar el grupo y volver a añadir las particiones ordenadas
        GList *current = data->partitions;
        while (current) {
            PartitionInfo *info = (PartitionInfo*)current->data;
            page4_add_partition_row(data, info);
            current = current->next;
        }
    }
    
    if (partition_count == 0) {
        // Si no hay particiones, mostrar mensaje
        AdwActionRow *empty_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(empty_row), "No hay particiones");
        adw_action_row_set_subtitle(empty_row, "El disco no tiene particiones o no se pudieron detectar");
        
        GtkImage *icon = GTK_IMAGE(gtk_image_new_from_icon_name("dialog-information-symbolic"));
        adw_action_row_add_prefix(empty_row, GTK_WIDGET(icon));
        
        adw_preferences_group_add(data->partitions_group, GTK_WIDGET(empty_row));
        
        // Guardar referencia a la fila vacía también
        data->partition_rows = g_list_append(data->partition_rows, empty_row);
    }
    
    LOG_INFO("Particiones encontradas: %d", partition_count);
}

// Función para crear información de partición
PartitionInfo* page4_create_partition_info(const gchar *device_path, UDisksPartition *partition, UDisksBlock *block, UDisksObject *object)
{
    if (!device_path || !block) return NULL;
    
    PartitionInfo *info = g_malloc0(sizeof(PartitionInfo));
    
    // Información básica
    info->device_path = g_strdup(device_path);
    info->size = udisks_block_get_size(block);
    info->size_formatted = page4_format_partition_size(info->size);
    
    // Obtener filesystem
    UDisksFilesystem *filesystem = udisks_object_peek_filesystem(object);
    if (filesystem) {
        info->filesystem = g_strdup(udisks_block_get_id_type(block));
        
        // Obtener puntos de montaje
        const gchar *const *mount_points = udisks_filesystem_get_mount_points(filesystem);
        if (mount_points && mount_points[0]) {
            info->mount_point = g_strdup(mount_points[0]);
            info->is_mounted = TRUE;
        }
    } else {
        info->filesystem = g_strdup("Sin formato");
        info->is_mounted = FALSE;
    }
    
    // Obtener label y UUID
    info->label = g_strdup(udisks_block_get_id_label(block));
    info->uuid = g_strdup(udisks_block_get_id_uuid(block));
    
    // Si no hay label, usar un nombre genérico
    if (!info->label || strlen(info->label) == 0) {
        g_free(info->label);
        info->label = g_strdup(g_path_get_basename(device_path));
    }
    
    return info;
}

// Función para liberar información de partición
void page4_free_partition_info(PartitionInfo *info)
{
    if (!info) return;
    
    g_free(info->device_path);
    g_free(info->filesystem);
    g_free(info->mount_point);
    g_free(info->size_formatted);
    g_free(info->label);
    g_free(info->uuid);
    g_free(info);
}

// Función para añadir una fila de partición
void page4_add_partition_row(Page4Data *data, PartitionInfo *partition)
{
    if (!data || !partition) return;
    
    // Crear AdwActionRow
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    
    // Configurar título y subtítulo
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), partition->device_path);
    
    gchar *subtitle = g_strdup_printf("%s • %s%s%s", 
                                      partition->size_formatted,
                                      partition->filesystem,
                                      partition->mount_point ? " • " : "",
                                      partition->mount_point ? partition->mount_point : "");
    adw_action_row_set_subtitle(row, subtitle);
    g_free(subtitle);
    
    // Añadir icono según el tipo de filesystem
    GtkImage *icon = GTK_IMAGE(gtk_image_new_from_icon_name(page4_get_filesystem_icon(partition->filesystem)));
    adw_action_row_add_prefix(row, GTK_WIDGET(icon));
    
    // Añadir botón de configuración
    GtkButton *config_button = GTK_BUTTON(gtk_button_new_from_icon_name("list-add-symbolic"));
    gtk_widget_set_valign(GTK_WIDGET(config_button), GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(GTK_WIDGET(config_button), "Configurar partición");
    
    // Añadir clases CSS
    gtk_widget_add_css_class(GTK_WIDGET(config_button), "flat");
    gtk_widget_add_css_class(GTK_WIDGET(config_button), "circular");
    
    // Conectar señal
    g_signal_connect(config_button, "clicked", G_CALLBACK(on_page4_partition_configure_clicked), partition);
    
    adw_action_row_add_suffix(row, GTK_WIDGET(config_button));
    
    // Añadir al grupo
    adw_preferences_group_add(data->partitions_group, GTK_WIDGET(row));
    
    // Guardar referencia a la fila para poder eliminarla después
    data->partition_rows = g_list_append(data->partition_rows, row);
    
    LOG_INFO("Fila de partición añadida: %s", partition->device_path);
}

// Función para verificar si una partición pertenece a un disco
gboolean page4_is_partition_of_disk(const gchar *partition_path, const gchar *disk_path)
{
    if (!partition_path || !disk_path) return FALSE;
    
    // Extraer el nombre base del disco (ej: /dev/sda -> sda)
    gchar *disk_name = g_path_get_basename(disk_path);
    
    // Verificar si la partición comienza con el nombre del disco
    gchar *partition_name = g_path_get_basename(partition_path);
    gboolean is_partition = g_str_has_prefix(partition_name, disk_name);
    
    g_free(disk_name);
    g_free(partition_name);
    
    return is_partition;
}

// Función para formatear tamaño de partición
gchar* page4_format_partition_size(guint64 size_bytes)
{
    if (size_bytes == 0) {
        return g_strdup("0 B");
    }
    
    const gchar *units[] = {"B", "KB", "MB", "GB", "TB"};
    gint unit_index = 0;
    gdouble size = (gdouble)size_bytes;
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    if (unit_index == 0) {
        return g_strdup_printf("%.0f %s", size, units[unit_index]);
    } else {
        return g_strdup_printf("%.1f %s", size, units[unit_index]);
    }
}

// Función para obtener icono según el filesystem
const gchar* page4_get_filesystem_icon(const gchar *filesystem)
{
    if (!filesystem) return "drive-harddisk-symbolic";
    
    if (g_str_has_prefix(filesystem, "ext")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "ntfs")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "fat32") || g_str_equal(filesystem, "vfat")) {
        return "drive-removable-media-symbolic";
    } else if (g_str_equal(filesystem, "swap")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "btrfs")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "xfs")) {
        return "drive-harddisk-symbolic";
    } else {
        return "drive-harddisk-symbolic";
    }
}

// Callback para configurar partición
void on_page4_partition_configure_clicked(GtkButton *button, gpointer user_data)
{
    PartitionInfo *partition = (PartitionInfo*)user_data;
    
    if (!partition) return;
    
    LOG_INFO("Configurando partición: %s", partition->device_path);
    
    // Aquí se puede implementar un diálogo de configuración
    // Por ahora, solo mostrar información en el log
    LOG_INFO("Partición: %s", partition->device_path);
    LOG_INFO("Filesystem: %s", partition->filesystem);
    LOG_INFO("Tamaño: %s", partition->size_formatted);
    LOG_INFO("Montado en: %s", partition->mount_point ? partition->mount_point : "No montado");
    
    // TODO: Implementar diálogo de configuración de punto de montaje
}