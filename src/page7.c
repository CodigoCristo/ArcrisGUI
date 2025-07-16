#include "page7.h"
#include "config.h"
#include <stdio.h>

// Variable global para datos de la página 7
static Page7Data *g_page7_data = NULL;

// Función principal de inicialización de la página 7
void page7_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page7_data = g_malloc0(sizeof(Page7Data));
    
    // Guardar referencias importantes
    g_page7_data->carousel = carousel;
    g_page7_data->revealer = revealer;
    
    // Cargar la página 7 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page7.ui");
    if (!page_builder) {
        LOG_ERROR("No se pudo cargar el builder de página 7");
        return;
    }
    
    // Obtener el widget principal
    GtkWidget *page7 = GTK_WIDGET(gtk_builder_get_object(page_builder, "main_bin"));
    if (!page7) {
        LOG_ERROR("No se pudo cargar la página 7 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }
    
    // Guardar referencia al widget principal
    g_page7_data->main_content = page7;
    
    // Obtener el widget del logo
    g_page7_data->logo_image = GTK_IMAGE(gtk_builder_get_object(page_builder, "logo"));
    
    // Realizar configuraciones iniciales
    page7_setup_widgets(g_page7_data);
    page7_load_data(g_page7_data);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page7);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
    
    LOG_INFO("Página 7 (Logo Final) inicializada correctamente");
}

// Función de limpieza
void page7_cleanup(Page7Data *data)
{
    if (g_page7_data) {
        g_free(g_page7_data);
        g_page7_data = NULL;
        LOG_INFO("Página 7 limpiada correctamente");
    }
}

// Función para configurar widgets
void page7_setup_widgets(Page7Data *data)
{
    if (!data) return;
    
    // Configurar el logo
    page7_setup_logo(data);
    
    LOG_INFO("Widgets de la página 7 configurados");
}

// Función para cargar datos
void page7_load_data(Page7Data *data)
{
    if (!data) return;
    
    // No hay datos específicos que cargar para esta página
    LOG_INFO("Datos de la página 7 cargados");
}

// Función para configurar el logo
void page7_setup_logo(Page7Data *data)
{
    if (!data || !data->logo_image) return;
    
    // El logo ya está configurado en el UI con el recurso correcto
    // Solo verificamos que esté visible
    gtk_widget_set_visible(GTK_WIDGET(data->logo_image), TRUE);
    
    LOG_INFO("Logo configurado correctamente en página 7");
}

// Función para establecer el tamaño del logo
void page7_set_logo_size(Page7Data *data, gint pixel_size)
{
    if (!data || !data->logo_image) return;
    
    gtk_image_set_pixel_size(data->logo_image, pixel_size);
    LOG_INFO("Tamaño del logo establecido: %d px en página 7", pixel_size);
}

// Función para ir a la página anterior
gboolean page7_go_to_previous_page(Page7Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    // Ir a la página anterior (página 6)
    GtkWidget *page6 = adw_carousel_get_nth_page(data->carousel, 5);
    if (page6) {
        adw_carousel_scroll_to(data->carousel, page6, TRUE);
        LOG_INFO("Navegación a página anterior exitosa desde página 7");
        return TRUE;
    }
    
    return FALSE;
}

// Función para verificar si es la página final
gboolean page7_is_final_page(void)
{
    return TRUE;
}

// Callback de navegación hacia atrás
void on_page7_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 7");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 7");
    }
}

// Función llamada cuando se muestra la página 7
void page7_on_page_shown(void)
{
    LOG_INFO("Página 7 mostrada - Mostrando logo final");
}

// Función para obtener el widget principal
GtkWidget* page7_get_widget(void)
{
    if (!g_page7_data) return NULL;
    return g_page7_data->main_content;
}