#ifndef PAGE2_H
#define PAGE2_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include <libsoup/soup.h>

// Estructura para datos de la página 2
typedef struct _Page2Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // ComboRows para configuraciones
    AdwComboRow *combo_keyboard;
    AdwComboRow *combo_keymap;
    AdwComboRow *combo_timezone;
    AdwComboRow *combo_locale;
    
    // Widgets adicionales
    GtkButton *tecla_button;
    GtkLabel *time_label;
    
    // Modelos de datos
    GtkStringList *keyboard_list;
    GtkStringList *keymap_list;
    GtkStringList *timezone_list;
    GtkStringList *locale_list;
} Page2Data;

// Funciones principales de la página 2
void page2_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page2_cleanup(Page2Data *data);

// Funciones de inicialización de datos
void page2_load_keyboards(GtkStringList *keyboard_list);
void page2_load_keymaps(GtkStringList *keymap_list);
void page2_load_timezones(GtkStringList *timezone_list);
void page2_load_locales(GtkStringList *locale_list);

// Funciones de configuración de ComboRows
void page2_setup_combo_row(AdwComboRow *combo_row, GtkStringList *model, 
                           GCallback callback, gpointer user_data);

// Callbacks para los ComboRows
void on_keyboard_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data);
void on_keymap_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data);
void on_timezone_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data);
void on_locale_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data);

// Funciones utilitarias
gboolean update_time_display(gpointer user_data);
void open_keyboard_settings(GtkButton *button, gpointer user_data);
void open_tecla(GtkButton *button, gpointer user_data);
void save_combo_selections_to_file(void);

// Funciones de configuración automática (internas)
// Las siguientes funciones son privadas del módulo y se ejecutan automáticamente:
// - page2_get_language_from_api(): Obtiene idioma desde https://ipapi.co/languages
// - page2_get_timezone_from_api(): Obtiene zona horaria desde https://ipapi.co/timezone
// - auto_select_in_combo_row(): Selecciona automáticamente elementos en ComboRows
// - auto_configure_combo_rows(): Configura todos los ComboRows basándose en el idioma y zona horaria detectados

// Funciones helper para ejecutar comandos del sistema
gboolean execute_system_command_to_list(const char *command, GtkStringList *list);

#endif