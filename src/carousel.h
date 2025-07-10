#ifndef CAROUSEL_H
#define CAROUSEL_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include "config.h"

// Forward declarations for page modules
typedef struct _Page1Data Page1Data;
typedef struct _Page2Data Page2Data;
typedef struct _Page3Data Page3Data;
typedef struct _Page4Data Page4Data;
typedef struct _Page5Data Page5Data;



// Estructura principal del manager del carousel
typedef struct {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    GtkButton *back_button;
    GtkButton *next_button;
    
    // Referencias a los datos de cada página
    Page1Data *page1_data;
    Page2Data *page2_data;
    Page3Data *page3_data;
    Page4Data *page4_data;
    Page5Data *page5_data;


    
    // Estado del carousel
    guint current_page;
    guint total_pages;
    gboolean is_initialized;
    ArcrisState current_state;
    
} CarouselManager;

// Funciones principales del manager del carousel
CarouselManager* carousel_manager_new(void);
void carousel_manager_init(CarouselManager *manager, GtkBuilder *builder);
void carousel_manager_cleanup(CarouselManager *manager);

// Funciones de navegación del carousel
void carousel_navigate_to_page(CarouselManager *manager, guint page_index);
void carousel_navigate_to_page_type(CarouselManager *manager, CarouselPageType page_type);
void carousel_navigate_next(CarouselManager *manager);
void carousel_navigate_previous(CarouselManager *manager);
gboolean carousel_can_navigate_next(CarouselManager *manager);
gboolean carousel_can_navigate_previous(CarouselManager *manager);

// Funciones de control de la interfaz
void carousel_update_navigation_controls(CarouselManager *manager);
void carousel_set_navigation_visible(CarouselManager *manager, gboolean visible);
void carousel_set_state(CarouselManager *manager, ArcrisState new_state);
ArcrisState carousel_get_state(CarouselManager *manager);

// Callbacks de navegación (mantenidos para compatibilidad)
void on_back_button_clicked(GtkButton *button, gpointer user_data);
void on_next_button_clicked(GtkButton *button, gpointer user_data);
void on_carousel_page_changed(AdwCarousel *carousel, guint page, gpointer user_data);

// Funciones de utilidad
guint carousel_get_current_page(CarouselManager *manager);
guint carousel_get_total_pages(CarouselManager *manager);
const char* carousel_get_page_name(guint page_index);
CarouselPageType carousel_get_current_page_type(CarouselManager *manager);
gboolean carousel_is_on_first_page(CarouselManager *manager);
gboolean carousel_is_on_last_page(CarouselManager *manager);

// Funciones de inicialización de páginas
void carousel_init_all_pages(CarouselManager *manager, GtkBuilder *builder);
void carousel_setup_page_navigation(CarouselManager *manager);

#endif

