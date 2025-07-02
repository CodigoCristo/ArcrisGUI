#ifndef PAGE4_H
#define PAGE4_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include "disk_manager.h"

// Estructura para datos de la página 4
typedef struct _Page4Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Widgets específicos de la página 4 (información del disco)
    GtkLabel *disk_label_page4;
    GtkLabel *disk_size_label_page4;
    GtkButton *gparted_button;
    GtkButton *refresh_button;
    
    // Información del disco actual
    gchar *current_disk_path;
    gchar *current_disk_size;
    
} Page4Data;

// Funciones principales de la página 4
void page4_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page4_cleanup(Page4Data *data);

// Funciones de configuración y carga de datos
void page4_setup_widgets(Page4Data *data);
void page4_load_data(Page4Data *data);
void page4_update_disk_info(Page4Data *data);

// Funciones de navegación y validación
gboolean page4_go_to_next_page(Page4Data *data);
gboolean page4_go_to_previous_page(Page4Data *data);
gboolean page4_is_configuration_valid(void);

// Navigation button functions
void page4_create_navigation_buttons(Page4Data *data);

// Funciones de utilidad
void page4_refresh_disk_info(void);
void page4_open_gparted(void);
gchar* page4_get_disk_size(const gchar *disk_path);
void page4_on_page_shown(void);
void page4_test_update(void);

// Callbacks para señales de widgets
void on_page4_gparted_button_clicked(GtkButton *button, gpointer user_data);
void on_page4_refresh_clicked(GtkButton *button, gpointer user_data);

// Navigation callbacks
void on_page4_next_button_clicked(GtkButton *button, gpointer user_data);
void on_page4_back_button_clicked(GtkButton *button, gpointer user_data);

#endif /* PAGE4_H */