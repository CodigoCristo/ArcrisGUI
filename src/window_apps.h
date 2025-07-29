#ifndef WINDOW_APPS_H
#define WINDOW_APPS_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Estructura para datos de la ventana de utilities apps
typedef struct _WindowAppsData {
    GtkWindow *window;
    GtkBuilder *builder;
    
    // Botones de la ventana
    GtkButton *close_button;
    GtkButton *save_button;
    
    // Elementos de búsqueda
    GtkSearchEntry *search_entry;
    
    // Expanderes para categorías
    AdwExpanderRow *browsers_expander;
    AdwExpanderRow *graphics_expander;
    AdwExpanderRow *video_expander;
    
    // Estado de inicialización
    gboolean is_initialized;
    
    // Datos de aplicaciones seleccionadas
    GHashTable *selected_apps;
    
} WindowAppsData;

// Funciones principales de la ventana
WindowAppsData* window_apps_new(void);
void window_apps_init(WindowAppsData *data);
void window_apps_cleanup(WindowAppsData *data);
void window_apps_show(WindowAppsData *data, GtkWindow *parent);
void window_apps_hide(WindowAppsData *data);

// Funciones de configuración
void window_apps_setup_widgets(WindowAppsData *data);
void window_apps_connect_signals(WindowAppsData *data);
void window_apps_load_widgets_from_builder(WindowAppsData *data);

// Funciones de búsqueda
void window_apps_setup_search(WindowAppsData *data);
void window_apps_filter_apps(WindowAppsData *data, const gchar *search_text);

// Funciones de persistencia
gboolean window_apps_load_selected_apps_from_file(WindowAppsData *data);
gboolean window_apps_save_selected_apps_to_file(WindowAppsData *data);

// Callbacks de botones
void on_apps_close_button_clicked(GtkButton *button, gpointer user_data);
void on_apps_save_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de búsqueda
void on_apps_search_changed(GtkSearchEntry *entry, gpointer user_data);

// Funciones de utilidad
void window_apps_reset_to_defaults(WindowAppsData *data);
GHashTable* window_apps_get_selected_apps(WindowAppsData *data);
void window_apps_set_selected_apps(WindowAppsData *data, GHashTable *apps);

// Función para obtener la instancia global
WindowAppsData* window_apps_get_instance(void);

#endif /* WINDOW_APPS_H */