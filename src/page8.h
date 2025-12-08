#ifndef PAGE8_H
#define PAGE8_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include <vte/vte.h>

// Estructura para datos de la página 8
typedef struct _Page8Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Stack principal para alternar entre carousel y terminal
    GtkStack *main_stack;
    GtkToggleButton *terminal_button;
    
    // Widgets del carousel de imágenes
    AdwCarousel *image_carousel;
    AdwCarouselIndicatorLines *carousel_indicators;
    
    // Imágenes del carousel
    GtkPicture *carousel_image1;
    GtkPicture *carousel_image2;
    GtkPicture *carousel_image3;
    GtkPicture *carousel_image4;
    
    // Labels y otros widgets
    GtkLabel *install_title;
    GtkProgressBar *progress_bar;
    
    // Widgets de la terminal
    VteTerminal *vte_terminal;
    GtkLabel *terminal_title;
    GtkLabel *terminal_info;
    
    // Timer para cambio automático de imágenes
    guint carousel_timeout_id;
    gint current_image_index;
    gint total_images;
    
    // Timer para animación del progress bar
    guint progress_bar_timeout_id;
    
    // Estado de la página
    gboolean is_installing;
    gboolean carousel_auto_advance;
    gboolean terminal_visible;
    
} Page8Data;

// Funciones principales de la página 8
void page8_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page8_cleanup(Page8Data *data);

// Funciones de configuración
void page8_setup_widgets(Page8Data *data);
void page8_load_data(Page8Data *data);

// Funciones del carousel
void page8_start_carousel_timer(Page8Data *data);
void page8_stop_carousel_timer(Page8Data *data);
gboolean page8_carousel_timeout_callback(gpointer user_data);
void page8_advance_carousel(Page8Data *data);

// Funciones del progress bar
void page8_start_progress_bar_pulse(Page8Data *data);
void page8_stop_progress_bar_pulse(Page8Data *data);

// Funciones de instalación
void page8_start_installation(Page8Data *data);
void page8_stop_installation(Page8Data *data);
void page8_execute_install_script(Page8Data *data);

// Funciones de terminal
void page8_setup_terminal(Page8Data *data);
void page8_toggle_terminal(Page8Data *data);
void page8_show_terminal(Page8Data *data);
void page8_show_carousel(Page8Data *data);
void page8_terminal_output(Page8Data *data, const gchar *text);

// Callbacks
void on_terminal_button_toggled(GtkToggleButton *button, gpointer user_data);

// Funciones de estado
void page8_on_page_shown(void);
void page8_on_page_hidden(void);
GtkWidget* page8_get_widget(void);

// Funciones de navegación
gboolean page8_is_final_page(void);

// Función para obtener datos globales de page8
Page8Data* page8_get_data(void);

#endif /* PAGE8_H */