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
    
    // Widgets de configuración del sistema
    AdwActionRow *kernel_row;
    AdwActionRow *drivers_row;
    AdwSwitchRow *essential_apps_switch;
    // office_switch eliminado - no existe en el UI
    AdwSwitchRow *utilities_switch;
    GtkButton *kernel_button;
    GtkButton *driver_hardware_button;
    GtkButton *essential_apps_button;
    // office_button eliminado - no existe en el UI
    GtkButton *utilities_button;
    AdwButtonRow *program_extra_button;
    
    // Estados de configuración
    gboolean essential_apps_enabled;
    gboolean office_enabled;
    gboolean utilities_enabled;
    
} Page6Data;

// Funciones principales de la página 6
void page6_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page6_cleanup(Page6Data *data);

// Funciones de configuración
void page6_setup_widgets(Page6Data *data);
void page6_load_data(Page6Data *data);
void page6_get_ui_widgets(Page6Data *data, GtkBuilder *builder);
void page6_connect_signals(Page6Data *data);

// Funciones de navegación
gboolean page6_go_to_previous_page(Page6Data *data);
gboolean page6_go_to_next_page(Page6Data *data);
gboolean page6_is_final_page(void);

// Funciones de estado
void page6_on_page_shown(void);
GtkWidget* page6_get_widget(void);
Page6Data* page6_get_data(void);

// Funciones de configuración del sistema
void page6_setup_switches(Page6Data *data);
void page6_load_system_config(Page6Data *data);
void page6_save_configuration(Page6Data *data);
gboolean page6_validate_configuration(Page6Data *data);
void load_page6_switches_from_file(void);
void save_page6_switches_to_file(void);

// Funciones de manejo de eventos
void page6_on_kernel_selection(Page6Data *data);
void page6_on_drivers_configuration(Page6Data *data);
void page6_on_essential_apps_toggled(Page6Data *data, gboolean active);
void page6_on_office_toggled(Page6Data *data, gboolean active);
void page6_on_utilities_toggled(Page6Data *data, gboolean active);

// Callbacks de navegación
void on_page6_back_button_clicked(GtkButton *button, gpointer user_data);
void on_page6_next_button_clicked(GtkButton *button, gpointer user_data);

// Funciones de acceso a configuración
gboolean page6_get_essential_apps_enabled(void);
gboolean page6_get_office_enabled(void);
gboolean page6_get_utilities_enabled(void);

// Funciones para manejo de kernels
void on_kernel_button_clicked(GtkButton *button, gpointer user_data);
void page6_open_kernel_selection_window(Page6Data *data);
void page6_display_current_kernel(void);
void page6_update_kernel_subtitle(const char* kernel_name);

// Funciones para manejo de hardware
void on_driver_hardware_button_clicked(GtkButton *button, gpointer user_data);
void page6_open_hardware_window(Page6Data *data);

// Funciones para manejo del sistema
void on_essential_apps_button_clicked(GtkButton *button, gpointer user_data);
void page6_open_system_window(Page6Data *data);

// Funciones para manejo de utilities
void on_utilities_button_clicked(GtkButton *button, gpointer user_data);
void page6_open_utilities_window(Page6Data *data);

// Funciones para manejo de programas extra
void on_program_extra_button_clicked(AdwButtonRow *button, gpointer user_data);
void page6_open_program_extra_window(Page6Data *data);

// Callbacks para switches
void on_essential_apps_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data);
// void on_office_switch_toggled eliminado - office_switch no existe en el UI
void on_utilities_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data);

// Callbacks para ventana de kernels
void on_kernel_save_clicked(GtkButton *button, gpointer user_data);
gboolean page6_update_kernel_ui_delayed(gpointer user_data);

#endif /* PAGE6_H */