#ifndef PAGE10_H
#define PAGE10_H

#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct _Page10Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    GtkWidget *main_content;
    GtkLabel *error_title;
    GtkLabel *error_message;
    GtkToggleButton *view_log_button;
    GtkRevealer *log_revealer;
    GtkTextView *log_text_view;
} Page10Data;

void page10_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page10_load_log(Page10Data *data);
GtkWidget* page10_get_widget(void);
Page10Data* page10_get_data(void);

void on_view_log_button_toggled(GtkToggleButton *button, gpointer user_data);

void page10_on_page_shown(void);
void page10_on_page_hidden(void);
void page10_update_language(void);

#endif /* PAGE10_H */
