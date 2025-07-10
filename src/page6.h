#ifndef PAGE6_H
#define PAGE6_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Estructura para datos de la página 6
typedef struct _Page6Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Widget del logo
    GtkImage *logo_image;
    
} Page6Data;

// Funciones principales de la página 6
void page6_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page6_cleanup(Page6Data *data);

// Funciones de configuración
void page6_setup_widgets(Page6Data *data);
void page6_load_data(Page6Data *data);

// Funciones de navegación
gboolean page6_go_to_previous_page(Page6Data *data);
gboolean page6_is_final_page(void);

// Funciones de estado
void page6_on_page_shown(void);
GtkWidget* page6_get_widget(void);

// Funciones de configuración del logo
void page6_setup_logo(Page6Data *data);
void page6_set_logo_size(Page6Data *data, gint pixel_size);

// Callbacks de navegación
void on_page6_back_button_clicked(GtkButton *button, gpointer user_data);

#endif /* PAGE6_H */