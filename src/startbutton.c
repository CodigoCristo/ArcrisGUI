#include "carousel.h"
#include "internet.h"
#include <adwaita.h>





void on_start_button_clicked(GtkButton *button, gpointer user_data)
{
    AdwCarousel *carousel = ADW_CAROUSEL(g_object_get_data(G_OBJECT(button), "carousel"));
    GtkRevealer *revealer = GTK_REVEALER(g_object_get_data(G_OBJECT(button), "revealer"));

        gtk_revealer_set_reveal_child(revealer, TRUE);
        // Mover a la siguiente página del carousel
        guint current_page = adw_carousel_get_position(carousel);
        guint total_pages = adw_carousel_get_n_pages(carousel);

        if (current_page + 1 < total_pages) {
            GtkWidget *next_page = adw_carousel_get_nth_page(carousel, current_page + 1);
            adw_carousel_scroll_to(carousel, next_page, 300); // 300 ms de duración para la animación
        }
     
}
