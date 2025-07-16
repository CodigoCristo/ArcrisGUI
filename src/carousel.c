#include "carousel.h"
#include "page1.h"
#include "page2.h"
#include "page3.h"
#include "page4.h"
#include "page5.h"
#include "page6.h"
#include "page7.h"


#include "config.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declaration for page change callback
static void on_carousel_page_changed_internal(AdwCarousel *carousel, guint page, gpointer user_data);

// Variable global del manager del carousel
static CarouselManager *g_carousel_manager = NULL;

// Usar los nombres de páginas compartidos desde config.h

// Implementación de funciones principales del manager

CarouselManager* carousel_manager_new(void)
{
    CarouselManager *manager = g_malloc0(sizeof(CarouselManager));
    
    manager->current_page = CAROUSEL_FIRST_PAGE;
    manager->total_pages = 0;
    manager->is_initialized = FALSE;
    manager->current_state = ARCRIS_STATE_INITIALIZING;
    
    // Inicializar punteros de datos de páginas
    manager->page1_data = NULL;
    manager->page2_data = NULL;
    manager->page3_data = NULL;
    manager->page4_data = NULL;
    manager->page5_data = NULL;

    
    LOG_INFO("CarouselManager creado");
    return manager;
}

void carousel_manager_init(CarouselManager *manager, GtkBuilder *builder)
{
    if (!manager || !builder) {
        LOG_ERROR("CarouselManager o GtkBuilder son NULL");
        return;
    }
    
    // Obtener widgets del carousel desde el builder principal
    manager->carousel = ADW_CAROUSEL(gtk_builder_get_object(builder, "carousel"));
    manager->revealer = GTK_REVEALER(gtk_builder_get_object(builder, "revealer"));
    manager->back_button = GTK_BUTTON(gtk_builder_get_object(builder, "back_button"));
    manager->next_button = GTK_BUTTON(gtk_builder_get_object(builder, "next_button"));
    
    if (!manager->carousel || !manager->revealer || !manager->back_button || !manager->next_button) {
        LOG_ERROR("No se pudieron obtener todos los widgets del carousel");
        return;
    }
    
    // Configurar navegación
    carousel_setup_page_navigation(manager);
    
    // Inicializar todas las páginas
    carousel_init_all_pages(manager, builder);
    
    // Actualizar estado
    manager->total_pages = adw_carousel_get_n_pages(manager->carousel);
    manager->current_page = CAROUSEL_FIRST_PAGE;
    manager->is_initialized = TRUE;
    manager->current_state = ARCRIS_STATE_READY;
    
    // Configurar controles de navegación iniciales
    carousel_update_navigation_controls(manager);
    
    // Guardar referencia global
    g_carousel_manager = manager;
    
    LOG_INFO("CarouselManager inicializado con %u páginas", manager->total_pages);
}

void carousel_init_all_pages(CarouselManager *manager, GtkBuilder *builder)
{
    if (!manager || !builder) return;
    
    LOG_INFO("Inicializando todas las páginas del carousel...");
    
    // Inicializar página 1 (Verificación de Internet)
    page1_init(builder, manager->carousel, manager->revealer);
    
    // Inicializar página 2 (Configuración del Sistema)
    page2_init(builder, manager->carousel, manager->revealer);
    
    // Inicializar página 3 (Configuración Adicional)
    page3_init(builder, manager->carousel, manager->revealer);
    
    // Inicializar página 4 (Registro de Usuario)
    page4_init(builder, manager->carousel, manager->revealer);
    
    // Inicializar página 5 (Personalización)
    page5_init(builder, manager->carousel, manager->revealer);
    
    // Inicializar página 6 (Sistema)
    page6_init(builder, manager->carousel, manager->revealer);
    
    // Inicializar página 7 (Logo Final)
    page7_init(builder, manager->carousel, manager->revealer);

    
    LOG_INFO("Todas las páginas han sido inicializadas");
}

void carousel_setup_page_navigation(CarouselManager *manager)
{
    if (!manager) return;
    
    // Asociar datos del manager a los botones para los callbacks
    g_object_set_data(G_OBJECT(manager->back_button), "carousel_manager", manager);
    g_object_set_data(G_OBJECT(manager->next_button), "carousel_manager", manager);
    
    // Conectar señales de navegación
    g_signal_connect(manager->back_button, "clicked", G_CALLBACK(on_back_button_clicked), manager);
    g_signal_connect(manager->next_button, "clicked", G_CALLBACK(on_next_button_clicked), manager);
    g_signal_connect(manager->carousel, "page-changed", G_CALLBACK(on_carousel_page_changed_internal), manager);
    
    LOG_INFO("Navegación del carousel configurada");
}

// Funciones de navegación

void carousel_navigate_to_page(CarouselManager *manager, guint page_index)
{
    if (!manager || !manager->is_initialized) return;
    
    if (page_index >= manager->total_pages) {
        LOG_WARNING("Índice de página inválido: %u (total: %u)", page_index, manager->total_pages);
        return;
    }
    
    GtkWidget *target_page = adw_carousel_get_nth_page(manager->carousel, page_index);
    if (target_page) {
        guint old_page = manager->current_page;
        adw_carousel_scroll_to(manager->carousel, target_page, CAROUSEL_ANIMATION_DURATION);
        manager->current_page = page_index;
        
        arcris_log_page_transition(old_page, page_index);
        
        // Actualizar controles de navegación
        carousel_update_navigation_controls(manager);
    }
}

void carousel_navigate_to_page_type(CarouselManager *manager, CarouselPageType page_type)
{
    if (!manager || !manager->is_initialized) return;
    
    carousel_navigate_to_page(manager, (guint)page_type);
}

void carousel_navigate_next(CarouselManager *manager)
{
    if (!manager || !carousel_can_navigate_next(manager)) return;
    
    carousel_navigate_to_page(manager, arcris_get_next_page(manager->current_page));
}

void carousel_navigate_previous(CarouselManager *manager)
{
    if (!manager || !carousel_can_navigate_previous(manager)) return;
    
    carousel_navigate_to_page(manager, arcris_get_previous_page(manager->current_page));
}

gboolean carousel_can_navigate_next(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return FALSE;
    
    return !arcris_is_last_page(manager->current_page);
}

gboolean carousel_can_navigate_previous(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return FALSE;
    
    return !arcris_is_first_page(manager->current_page);
}

// Funciones de control de la interfaz

void carousel_update_navigation_controls(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return;
    
    gboolean show_controls = FALSE;
    gboolean can_go_back = carousel_can_navigate_previous(manager);
    gboolean can_go_next = carousel_can_navigate_next(manager);
    
    // Mostrar controles en todas las páginas excepto la primera
    if (!arcris_is_first_page(manager->current_page)) {
        show_controls = TRUE;
    }
    
    // Actualizar visibilidad del revealer
    carousel_set_navigation_visible(manager, show_controls);
    
    // Actualizar sensibilidad de los botones
    gtk_widget_set_sensitive(GTK_WIDGET(manager->back_button), can_go_back);
    gtk_widget_set_sensitive(GTK_WIDGET(manager->next_button), can_go_next);
    
    DEBUG_PRINT("Controles de navegación actualizados - Página: %u/%u (%s), Visible: %s", 
                manager->current_page + 1, manager->total_pages, 
                arcris_get_page_name(manager->current_page),
                show_controls ? "Sí" : "No");
}

void carousel_set_navigation_visible(CarouselManager *manager, gboolean visible)
{
    if (!manager || !manager->revealer) return;
    
    gtk_revealer_set_reveal_child(manager->revealer, visible);
}

// Funciones de estado
void carousel_set_state(CarouselManager *manager, ArcrisState new_state)
{
    if (!manager) return;
    
    ArcrisState old_state = manager->current_state;
    manager->current_state = new_state;
    
    arcris_log_state_change(old_state, new_state);
}

ArcrisState carousel_get_state(CarouselManager *manager)
{
    if (!manager) return ARCRIS_STATE_ERROR;
    
    return manager->current_state;
}

// Callbacks de navegación (compatibilidad con el código existente)

void on_back_button_clicked(GtkButton *button, gpointer user_data)
{
    CarouselManager *manager = (CarouselManager *)user_data;
    
    if (!manager) {
        // Fallback para compatibilidad con código antiguo
        manager = g_carousel_manager;
    }
    
    if (manager) {
        DEBUG_PRINT("Botón 'Anterior' presionado");
        carousel_navigate_previous(manager);
    }
}

void on_next_button_clicked(GtkButton *button, gpointer user_data)
{
    CarouselManager *manager = (CarouselManager *)user_data;
    
    if (!manager) {
        // Fallback para compatibilidad con código antiguo
        manager = g_carousel_manager;
    }
    
    if (manager) {
        DEBUG_PRINT("Botón 'Siguiente' presionado");
        carousel_navigate_next(manager);
    }
}

void on_carousel_page_changed(AdwCarousel *carousel, guint page, gpointer user_data)
{
    CarouselManager *manager = (CarouselManager *)user_data;
    
    if (!manager) {
        // Fallback para compatibilidad con código antiguo
        manager = g_carousel_manager;
    }
    
    if (manager) {
        manager->current_page = page;
        DEBUG_PRINT("Página del carousel cambiada a: %u (%s)", page, carousel_get_page_name(page));
        
        // Actualizar controles de navegación
        carousel_update_navigation_controls(manager);
    }
}

// Funciones de utilidad

guint carousel_get_current_page(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return 0;
    
    return manager->current_page;
}

guint carousel_get_total_pages(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return 0;
    
    return manager->total_pages;
}

const char* carousel_get_page_name(guint page_index)
{
    return arcris_get_page_name(page_index);
}

CarouselPageType carousel_get_current_page_type(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return PAGE_INTERNET_CHECK;
    
    return (CarouselPageType)manager->current_page;
}

gboolean carousel_is_on_first_page(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return FALSE;
    
    return arcris_is_first_page(manager->current_page);
}

gboolean carousel_is_on_last_page(CarouselManager *manager)
{
    if (!manager || !manager->is_initialized) return FALSE;
    
    return arcris_is_last_page(manager->current_page);
}

// Función de limpieza

void carousel_manager_cleanup(CarouselManager *manager)
{
    if (!manager) return;
    
    LOG_INFO("Limpiando CarouselManager...");
    
    // Limpiar datos de páginas individuales
    if (manager->page1_data) {
        page1_cleanup(manager->page1_data);
    }
    
    if (manager->page2_data) {
        page2_cleanup(manager->page2_data);
    }
    
    if (manager->page3_data) {
        page3_cleanup(manager->page3_data);
    }
    
    if (manager->page4_data) {
        page4_cleanup(manager->page4_data);
    }
    
    if (manager->page5_data) {
        page5_cleanup(manager->page5_data);
    }
    
    if (manager->page6_data) {
        page6_cleanup(manager->page6_data);
    }
    
    if (manager->page7_data) {
        page7_cleanup(manager->page7_data);
    }

    
    // Limpiar el manager
    g_free(manager);
    
    // Limpiar referencia global
    if (g_carousel_manager == manager) {
        g_carousel_manager = NULL;
    }
    
    LOG_INFO("CarouselManager limpiado correctamente");
}

// Callback interno para cambio de página del carousel
static void on_carousel_page_changed_internal(AdwCarousel *carousel, guint page, gpointer user_data)
{
    CarouselManager *manager = (CarouselManager *)user_data;
    if (!manager) return;
    
    LOG_INFO("Cambio de página del carousel: página %u", page);
    
    // Actualizar página actual en el manager
    manager->current_page = page;
    
    // Llamar a la función específica de page4 cuando se entra en ella (índice 3)
    if (page == 3) {
        page4_on_enter();
    }
    
    // Llamar a la función específica de page6 cuando se entra en ella (índice 5)
    if (page == 5) {
        page6_on_page_shown();
    }
    
    // Llamar a la función específica de page7 cuando se entra en ella (índice 6)
    if (page == 6) {
        page7_on_page_shown();
    }
    
    // Las páginas ahora manejan su propia lógica de actualización
    
    // También llamar al callback original si existe
    on_carousel_page_changed(carousel, page, user_data);
}

// Función pública para obtener el manager global (para compatibilidad)
CarouselManager* carousel_get_manager(void)
{
    return g_carousel_manager;
}