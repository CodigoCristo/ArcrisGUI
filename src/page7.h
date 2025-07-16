#ifndef PAGE7_H
#define PAGE7_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Estructura para datos de la página 7
typedef struct _Page7Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Widget del logo
    GtkImage *logo_image;
    
} Page7Data;

// Funciones principales de la página 7
void page7_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page7_cleanup(Page7Data *data);

// Funciones de configuración
void page7_setup_widgets(Page7Data *data);
void page7_load_data(Page7Data *data);

// Funciones de navegación
gboolean page7_go_to_previous_page(Page7Data *data);
gboolean page7_is_final_page(void);

// Funciones de estado
void page7_on_page_shown(void);
GtkWidget* page7_get_widget(void);

// Funciones de configuración del logo
void page7_setup_logo(Page7Data *data);
void page7_set_logo_size(Page7Data *data, gint pixel_size);

// Callbacks de navegación
void on_page7_back_button_clicked(GtkButton *button, gpointer user_data);

#endif /* PAGE7_H */