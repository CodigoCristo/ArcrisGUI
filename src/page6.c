#include "page6.h"
#include "config.h"
#include <stdio.h>

// Variable global para datos de la página 6
static Page6Data *g_page6_data = NULL;

// Función principal de inicialización de la página 6
void page6_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page6_data = g_malloc0(sizeof(Page6Data));
    
    // Guardar referencias importantes
    g_page6_data->carousel = carousel;
    g_page6_data->revealer = revealer;
    
    // Cargar la página 6 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page6.ui");
    if (!page_builder) {
        LOG_ERROR("No se pudo cargar el builder de página 6");
        return;
    }
    
    // Obtener el widget principal
    GtkWidget *page6 = GTK_WIDGET(gtk_builder_get_object(page_builder, "main_bin"));
    if (!page6) {
        LOG_ERROR("No se pudo cargar la página 6 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }
    
    // Guardar referencia al widget principal
    g_page6_data->main_content = page6;
    
    // Obtener el widget del logo
    g_page6_data->logo_image = GTK_IMAGE(gtk_builder_get_object(page_builder, "logo"));
    
    // Realizar configuraciones iniciales
    page6_setup_widgets(g_page6_data);
    page6_load_data(g_page6_data);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page6);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
    
    LOG_INFO("Página 6 (Logo) inicializada correctamente");
}

// Función de limpieza
void page6_cleanup(Page6Data *data)
{
    if (g_page6_data) {
        g_free(g_page6_data);
        g_page6_data = NULL;
        LOG_INFO("Página 6 limpiada correctamente");
    }
}

// Función para configurar widgets
void page6_setup_widgets(Page6Data *data)
{
    if (!data) return;
    
    // Configurar el logo
    page6_setup_logo(data);
    
    LOG_INFO("Widgets de la página 6 configurados");
}

// Función para cargar datos
void page6_load_data(Page6Data *data)
{
    if (!data) return;
    
    // No hay datos específicos que cargar para esta página
    LOG_INFO("Datos de la página 6 cargados");
}

// Función para configurar el logo
void page6_setup_logo(Page6Data *data)
{
    if (!data || !data->logo_image) return;
    
    // El logo ya está configurado en el UI con el recurso correcto
    // Solo verificamos que esté visible
    gtk_widget_set_visible(GTK_WIDGET(data->logo_image), TRUE);
    
    LOG_INFO("Logo configurado correctamente");
}

// Función para establecer el tamaño del logo
void page6_set_logo_size(Page6Data *data, gint pixel_size)
{
    if (!data || !data->logo_image) return;
    
    gtk_image_set_pixel_size(data->logo_image, pixel_size);
    LOG_INFO("Tamaño del logo establecido: %d px", pixel_size);
}

// Función para ir a la página anterior
gboolean page6_go_to_previous_page(Page6Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    // Ir a la página anterior (página 5)
    GtkWidget *page5 = adw_carousel_get_nth_page(data->carousel, 4);
    if (page5) {
        adw_carousel_scroll_to(data->carousel, page5, TRUE);
        LOG_INFO("Navegación a página anterior exitosa desde página 6");
        return TRUE;
    }
    
    return FALSE;
}

// Función para verificar si es la página final
gboolean page6_is_final_page(void)
{
    return TRUE;
}

// Callback de navegación hacia atrás
void on_page6_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    
    if (page6_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 6");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 6");
    }
}

// Función llamada cuando se muestra la página 6
void page6_on_page_shown(void)
{
    LOG_INFO("Página 6 mostrada - Mostrando logo final");
}

// Función para obtener el widget principal
GtkWidget* page6_get_widget(void)
{
    if (!g_page6_data) return NULL;
    return g_page6_data->main_content;
}