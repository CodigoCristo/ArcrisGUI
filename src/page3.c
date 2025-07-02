#include "page3.h"
#include "page4.h"
#include "disk_manager.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

// Variable global para datos de la página 3
static Page3Data *g_page3_data = NULL;
static DiskManager *g_disk_manager = NULL;

// Funciones privadas
static void page3_setup_disk_manager(Page3Data *data, GtkBuilder *page_builder);
static gboolean page3_validate_selection(Page3Data *data);
static void page3_save_configuration(Page3Data *data);


// Función principal de inicialización de la página 3
void page3_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page3_data = g_malloc0(sizeof(Page3Data));
    
    // Guardar referencias importantes
    g_page3_data->carousel = carousel;
    g_page3_data->revealer = revealer;
    
    // Cargar la página 3 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page3.ui");
    GtkWidget *page3 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page3"));
    
    if (!page3) {
        LOG_ERROR("No se pudo cargar la página 3 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }
    
    // Obtener widgets específicos de la página
    g_page3_data->main_content = page3;
    g_page3_data->disk_combo = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "disk_combo"));
    g_page3_data->auto_partition_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "auto_partition_radio"));
    g_page3_data->auto_btrfs_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "auto_btrfs_radio"));
    g_page3_data->manual_partition_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "manual_partition_radio"));
    g_page3_data->refresh_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "refresh_button"));
    
    // Verificar que todos los widgets se obtuvieron correctamente
    if (!g_page3_data->disk_combo || !g_page3_data->auto_partition_radio || 
        !g_page3_data->auto_btrfs_radio || !g_page3_data->manual_partition_radio ||
        !g_page3_data->refresh_button) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de la página 3");
        g_object_unref(page_builder);
        return;
    }
    
    // Configurar el administrador de discos
    page3_setup_disk_manager(g_page3_data, page_builder);
    
    // Realizar configuraciones iniciales específicas de la página 3
    page3_setup_widgets(g_page3_data);
    page3_load_data(g_page3_data);
    
    // Crear botones de navegación
    page3_create_navigation_buttons(g_page3_data);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page3);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
    
    LOG_INFO("Página 3 (Selección de Disco) inicializada correctamente");
}

// Función de limpieza
void page3_cleanup(Page3Data *data)
{
    if (g_disk_manager) {
        disk_manager_free(g_disk_manager);
        g_disk_manager = NULL;
    }
    
    if (g_page3_data) {
        g_free(g_page3_data);
        g_page3_data = NULL;
        LOG_INFO("Página 3 limpiada correctamente");
    }
}

// Configurar el administrador de discos
static void page3_setup_disk_manager(Page3Data *data, GtkBuilder *page_builder)
{
    if (!data) return;
    
    // Crear el administrador de discos
    g_disk_manager = disk_manager_new();
    if (!g_disk_manager) {
        LOG_ERROR("No se pudo crear el DiskManager");
        return;
    }
    
    // Inicializar widgets del administrador de discos
    if (!disk_manager_init(g_disk_manager, page_builder)) {
        LOG_ERROR("No se pudieron inicializar los widgets del DiskManager");
        disk_manager_free(g_disk_manager);
        g_disk_manager = NULL;
        return;
    }
    
    LOG_INFO("DiskManager configurado correctamente para la página 3");
}

// Función para configurar widgets
void page3_setup_widgets(Page3Data *data)
{
    if (!data) return;
    
    // Configurar estado inicial de los radio buttons
    gtk_check_button_set_active(data->auto_partition_radio, TRUE);
    
    // Conectar señales adicionales si es necesario
    // Los callbacks principales ya están conectados en disk_manager_init_widgets
    
    LOG_INFO("Widgets de la página 3 configurados");
}

// Función para cargar datos
void page3_load_data(Page3Data *data)
{
    if (!data || !g_disk_manager) return;
    
    // Cargar configuración guardada desde variables.sh
    disk_manager_load_from_variables(g_disk_manager);
    
    // Actualizar UI según la configuración cargada
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (selected_disk) {
        LOG_INFO("Disco previamente seleccionado: %s", selected_disk);
    }
    
    // Configurar modo de particionado por defecto
    if (data->auto_partition_radio) {
        gtk_check_button_set_active(data->auto_partition_radio, TRUE);
    }
    
    LOG_INFO("Datos de la página 3 cargados");
}

// Validar selección antes de continuar
static gboolean page3_validate_selection(Page3Data *data)
{
    if (!data || !g_disk_manager) return FALSE;
    
    // Verificar que se haya seleccionado un disco
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (!selected_disk) {
        LOG_WARNING("No se ha seleccionado ningún disco");
        
        // Mostrar mensaje de error al usuario
        AdwDialog *dialog = adw_alert_dialog_new(
            "Disco no seleccionado",
            "Por favor selecciona un disco para continuar"
        );
        
        adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "ok", "Aceptar");
        adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "ok");
        adw_dialog_present(dialog, NULL);
        
        return FALSE;
    }
    
    // Verificar que se haya seleccionado un modo de particionado
    if (data->auto_partition_radio && gtk_check_button_get_active(data->auto_partition_radio)) {
        LOG_INFO("Modo de particionado seleccionado: Auto Partition");
    } else if (data->auto_btrfs_radio && gtk_check_button_get_active(data->auto_btrfs_radio)) {
        LOG_INFO("Modo de particionado seleccionado: Auto Btrfs");
    } else if (data->manual_partition_radio && gtk_check_button_get_active(data->manual_partition_radio)) {
        LOG_INFO("Modo de particionado seleccionado: Manual Partition");
    }
    
    return TRUE;
}

// Guardar configuración actual
static void page3_save_configuration(Page3Data *data)
{
    if (!data || !g_disk_manager) return;
    
    // Guardar configuración en variables.sh
    if (disk_manager_save_to_variables(g_disk_manager)) {
        const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
        if (selected_disk) {
            LOG_INFO("Configuración de disco guardada: %s", selected_disk);
        }
    } else {
        LOG_WARNING("No se pudo guardar la configuración de disco");
    }
}

// Función para avanzar a la siguiente página
gboolean page3_go_to_next_page(Page3Data *data)
{
    if (!data || !g_disk_manager) return FALSE;
    
    // Validar selección
    if (!page3_validate_selection(data)) {
        return FALSE;
    }
    
    // Guardar configuración
    page3_save_configuration(data);
    
    // Ir siempre a page4 (la siguiente página en secuencia)
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    LOG_INFO("Dirigiendo a página 4 para disco: %s", selected_disk);
    
    // Ir a page4 (índice 3 en el carousel)
    if (data->carousel) {
        GtkWidget *page4 = adw_carousel_get_nth_page(data->carousel, 3);
        if (page4) {
            adw_carousel_scroll_to(data->carousel, page4, TRUE);
            // Actualizar información del disco en page4
            page4_on_page_shown();
        } else {
            LOG_ERROR("No se pudo encontrar page4 en el carousel");
        }
    }
    
    return TRUE;
}

// Función para obtener el disco seleccionado (para uso externo)
const char* page3_get_selected_disk(void)
{
    if (!g_disk_manager) return NULL;
    return disk_manager_get_selected_disk(g_disk_manager);
}

// Función para crear botones de navegación programáticamente
void page3_create_navigation_buttons(Page3Data *data)
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
                     G_CALLBACK(on_page3_back_button_clicked), data);
    g_signal_connect(next_button, "clicked", 
                     G_CALLBACK(on_page3_next_button_clicked), data);
    
    // Buscar el contenedor principal de page3 y agregar navegación
    GtkWidget *status_page = gtk_widget_get_first_child(data->main_content);
    if (status_page) {
        GtkWidget *status_child = gtk_widget_get_first_child(status_page);
        if (status_child && GTK_IS_BOX(status_child)) {
            gtk_box_append(GTK_BOX(status_child), navigation_box);
        }
    }
    
    LOG_INFO("Botones de navegación creados para página 3");
}

// Función para ir a la página anterior
gboolean page3_go_to_previous_page(Page3Data *data)
{
    if (!data) return FALSE;
    
    LOG_INFO("Regresando a página anterior desde página 3");
    
    // Ir a la página anterior en el carousel (page2, índice 1)
    if (data->carousel) {
        GtkWidget *page2 = adw_carousel_get_nth_page(data->carousel, 1);
        if (page2) {
            adw_carousel_scroll_to(data->carousel, page2, TRUE);
        } else {
            LOG_ERROR("No se pudo encontrar page2 en el carousel");
        }
    }
    
    return TRUE;
}

// Función para obtener el modo de particionado (para uso externo)
DiskMode page3_get_partition_mode(void)
{
    if (!g_page3_data) return DISK_MODE_AUTO_PARTITION;
    
    if (g_page3_data->auto_btrfs_radio && gtk_check_button_get_active(g_page3_data->auto_btrfs_radio)) {
        return DISK_MODE_AUTO_BTRFS;
    } else if (g_page3_data->manual_partition_radio && gtk_check_button_get_active(g_page3_data->manual_partition_radio)) {
        return DISK_MODE_MANUAL_PARTITION;
    }
    
    return DISK_MODE_AUTO_PARTITION;
}

// Función para refrescar la lista de discos (para uso externo)
void page3_refresh_disk_list(void)
{
    if (!g_disk_manager) return;
    disk_manager_refresh(g_disk_manager);
}

// Función para verificar si la configuración es válida
gboolean page3_is_configuration_valid(void)
{
    if (!g_page3_data) return FALSE;
    return page3_validate_selection(g_page3_data);
}

// Callback para el botón de siguiente (si se necesita integración externa)
// static void on_page3_next_button_clicked(GtkButton *button, gpointer user_data)
// {
//     Page3Data *data = (Page3Data *)user_data;
//     
//     if (page3_go_to_next_page(data)) {
//         LOG_INFO("Avanzando desde página 3");
//         
//         // Aquí se podría emitir una señal o llamar a una función del carousel
//         // para avanzar a la siguiente página
//         
//         // Por ejemplo:
//         // carousel_manager_go_to_next_page();
//     }
// }

// Funciones de callback que pueden ser conectadas externamente

void on_page3_disk_selection_changed(AdwComboRow *combo, GParamSpec *param, gpointer user_data)
{
    // Este callback ya se maneja en disk_manager.c
    // Se incluye aquí por compatibilidad si se necesita lógica adicional
    
    Page3Data *data = (Page3Data *)user_data;
    if (!data || !g_disk_manager) return;
    
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (selected_disk) {
        LOG_INFO("Disco seleccionado en página 3: %s", selected_disk);
    }
}

void on_page3_partition_mode_changed(GtkCheckButton *button, gpointer user_data)
{
    // Este callback ya se maneja en disk_manager.c
    // Se incluye aquí por compatibilidad si se necesita lógica adicional
    
    Page3Data *data = (Page3Data *)user_data;
    if (!data || !g_disk_manager) return;
    
    LOG_INFO("Modo de particionado cambiado en página 3");
}

void on_page3_refresh_clicked(GtkButton *button, gpointer user_data)
{
    LOG_INFO("Actualizando lista de discos desde página 3");
    page3_refresh_disk_list();
}

// Callback para el botón de siguiente
void on_page3_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    
    if (page3_go_to_next_page(data)) {
        LOG_INFO("Navegación exitosa desde página 3");
    } else {
        LOG_WARNING("No se pudo navegar desde página 3");
    }
}

// Callback para el botón atrás
void on_page3_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    
    if (page3_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 3");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 3");
    }
}