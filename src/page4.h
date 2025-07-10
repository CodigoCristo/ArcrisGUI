#ifndef PAGE4_H
#define PAGE4_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Estructura para datos de la página 4
typedef struct _Page4Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Widgets específicos de la página 4 (registro de usuario)
    AdwEntryRow *username_entry;
    AdwPasswordEntryRow *password_entry;
    AdwPasswordEntryRow *password_confirm_entry;
    
    // Widgets para hostname
    AdwEntryRow *hostname_entry;
    
    // Widget para mensaje de error
    GtkLabel *password_error_label;
    
    // Estado de validación
    gboolean passwords_match;
    gboolean username_valid;
    gboolean hostname_valid;
    gboolean password_length_valid;
    
    // Lista de nombres reservados
    gchar **reserved_usernames;
    
} Page4Data;

// Funciones principales de la página 4
void page4_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page4_cleanup(Page4Data *data);

// Funciones de configuración y carga de datos
void page4_setup_widgets(Page4Data *data);
void page4_load_data(Page4Data *data);

// Funciones de navegación y validación
gboolean page4_go_to_next_page(Page4Data *data);
gboolean page4_go_to_previous_page(Page4Data *data);
gboolean page4_is_installation_complete(void);
void page4_on_enter(void);
gboolean page4_delayed_validation(Page4Data *data);



// Navigation button functions
void page4_create_navigation_buttons(Page4Data *data);

// Funciones de validación de contraseñas
void page4_check_password_match(Page4Data *data);
gboolean page4_is_password_valid(Page4Data *data);
gboolean page4_is_form_valid(Page4Data *data);

// Funciones de validación de usuario y hostname
gboolean page4_load_reserved_usernames(Page4Data *data);
gboolean page4_is_username_valid(const gchar *username, Page4Data *data);
gboolean page4_is_hostname_valid(const gchar *hostname, Page4Data *data);
gboolean page4_is_password_length_valid(const gchar *password);
void page4_validate_username(Page4Data *data);
void page4_validate_hostname(Page4Data *data);
void page4_validate_password_length(Page4Data *data);

// Funciones de actualización de estado de UI
void page4_update_next_button_state(Page4Data *data);
GtkWidget* page4_find_next_button_recursive(GtkWidget *widget);
gboolean page4_initial_validation(Page4Data *data);

// Callbacks para señales de widgets
void on_page4_password_changed(AdwPasswordEntryRow *entry, gpointer user_data);
void on_page4_password_confirm_changed(AdwPasswordEntryRow *entry, gpointer user_data);
void on_page4_username_changed(AdwEntryRow *entry, gpointer user_data);
void on_page4_hostname_changed(AdwEntryRow *entry, gpointer user_data);

// Navigation callbacks
void on_page4_next_button_clicked(GtkButton *button, gpointer user_data);
void on_page4_back_button_clicked(GtkButton *button, gpointer user_data);

// Funciones de utilidad
void page4_reset_form(Page4Data *data);
const gchar* page4_get_username(Page4Data *data);
const gchar* page4_get_password(Page4Data *data);
const gchar* page4_get_hostname(Page4Data *data);
gboolean page4_save_user_data(Page4Data *data);

#endif /* PAGE4_H */