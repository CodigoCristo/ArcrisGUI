#ifndef PAGE5_H
#define PAGE5_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include "types.h"

// Enumeración para tipos de instalación
typedef enum {
    INSTALL_TYPE_TERMINAL,
    INSTALL_TYPE_DESKTOP,
    INSTALL_TYPE_WINDOW_MANAGER
} InstallationType;

// Enumeración para entornos de escritorio
typedef enum {
    DE_TYPE_GNOME,
    DE_TYPE_KDE,
    DE_TYPE_XFCE4,
    DE_TYPE_BUDGIE,
    DE_TYPE_CINNAMON,
    DE_TYPE_MATE,
    DE_TYPE_CUTEFISH,
    DE_TYPE_LXDE,
    DE_TYPE_LXQT,
    DE_TYPE_ENLIGHTENMENT,
    DE_TYPE_UKUI,
    DE_TYPE_PANTHEON
} DesktopEnvironmentType;

// Enumeración para gestores de ventanas
typedef enum {
    WM_TYPE_HYPRLAND,
    WM_TYPE_SWAY,
    WM_TYPE_DWL,
    WM_TYPE_DWM,
    WM_TYPE_I3WM,
    WM_TYPE_BSPWM,
    WM_TYPE_QTITLE,
    WM_TYPE_AWESOME,
    WM_TYPE_XMONAD,
    WM_TYPE_OPENBOX
} WindowManagerType;

// Estructura para datos de la página 5
typedef struct _Page5Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    GtkButton *next_button;  // Botón "Siguiente" del revealer
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Páginas del stack
    AdwStatusPage *page5;
    AdwClamp *de_page;
    AdwClamp *wm_page;
    
    // Widgets de la página principal
    GtkCheckButton *terminal_check;
    GtkCheckButton *desktop_check;
    GtkCheckButton *wm_check;
    
    // Botones go-next-symbolic
    GtkButton *desktop_next_button;
    GtkButton *wm_next_button;
    
    // Widgets de la página DE
    AdwComboRow *de_combo;
    GtkPicture *de_preview_image;
    GtkButton *de_back_to_main_button;
    GtkLabel *de_title_label;
    
    // Widgets de la página WM
    AdwComboRow *wm_combo;
    GtkPicture *wm_preview_image;
    GtkButton *wm_back_to_main_button;
    GtkLabel *wm_title_label;
    
    // Estado actual
    InstallationType current_type;
    DesktopEnvironmentType current_de;
    WindowManagerType current_wm;
    
    // Stack para navegación entre páginas
    GtkStack *pages_stack;
    
} Page5Data;

// Funciones principales de la página 5
void page5_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page5_cleanup(Page5Data *data);

// Funciones de configuración y carga de datos
void page5_setup_widgets(Page5Data *data);
void page5_load_data(Page5Data *data);
void page5_setup_stack(Page5Data *data);

// Funciones de navegación
gboolean page5_go_to_next_page(Page5Data *data);
gboolean page5_go_to_previous_page(Page5Data *data);
void page5_show_main_page(Page5Data *data);
void page5_show_de_page(Page5Data *data);
void page5_show_wm_page(Page5Data *data);

// Funciones de configuración de tipo de instalación
void page5_set_installation_type(Page5Data *data, InstallationType type);
InstallationType page5_get_installation_type(Page5Data *data);
void page5_update_next_buttons_state(Page5Data *data);

// Funciones de configuración de DE
void page5_set_desktop_environment(Page5Data *data, DesktopEnvironmentType de);
DesktopEnvironmentType page5_get_desktop_environment(Page5Data *data);
void page5_update_de_preview(Page5Data *data);

// Funciones de configuración de WM
void page5_set_window_manager(Page5Data *data, WindowManagerType wm);
WindowManagerType page5_get_window_manager(Page5Data *data);
void page5_update_wm_preview(Page5Data *data);

// Funciones de validación
gboolean page5_is_configuration_valid(Page5Data *data);
gboolean page5_can_proceed_to_next_page(Page5Data *data);

// Funciones de navegación de botones
void page5_create_navigation_buttons(Page5Data *data);

// Callbacks para señales de widgets
void on_page5_terminal_check_toggled(GtkCheckButton *check, gpointer user_data);
void on_page5_desktop_check_toggled(GtkCheckButton *check, gpointer user_data);
void on_page5_wm_check_toggled(GtkCheckButton *check, gpointer user_data);

// Callbacks para combo boxes
void on_page5_de_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);
void on_page5_wm_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);

// Callbacks para filas activables
void on_page5_desktop_row_activated(AdwActionRow *row, gpointer user_data);
void on_page5_wm_row_activated(AdwActionRow *row, gpointer user_data);

// Callbacks de navegación
void on_page5_next_button_clicked(GtkButton *button, gpointer user_data);
void on_page5_back_button_clicked(GtkButton *button, gpointer user_data);



// Callbacks para botones de navegación hacia atrás
void on_page5_de_back_to_main_button_clicked(GtkButton *button, gpointer user_data);
void on_page5_wm_back_to_main_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks para botones go-next-symbolic
void on_page5_desktop_next_button_clicked(GtkButton *button, gpointer user_data);
void on_page5_wm_next_button_clicked(GtkButton *button, gpointer user_data);

// Funciones de utilidad para nombres de recursos
const char* page5_get_de_image_resource(DesktopEnvironmentType de);
const char* page5_get_wm_image_resource(WindowManagerType wm);
const char* page5_get_de_name(DesktopEnvironmentType de);
const char* page5_get_wm_name(WindowManagerType wm);

// Funciones de configuración del sistema
gboolean page5_save_configuration(Page5Data *data);
gboolean page5_apply_configuration(Page5Data *data);

// Funciones de estado
void page5_on_page_shown(void);
GtkWidget* page5_get_widget(void);

// Funciones de conversión
DesktopEnvironmentType page5_index_to_de_type(guint index);
WindowManagerType page5_index_to_wm_type(guint index);
guint page5_de_type_to_index(DesktopEnvironmentType de);
guint page5_wm_type_to_index(WindowManagerType wm);

#endif /* PAGE5_H */