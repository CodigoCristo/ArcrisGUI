#ifndef WINDOW_SYSTEM_H
#define WINDOW_SYSTEM_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Enumeraciones para opciones del sistema
typedef enum {
    SHELL_BASH = 0,
    SHELL_DASH,
    SHELL_KSH,
    SHELL_FISH,
    SHELL_ZSH
} SystemShell;

// Estructura para datos de la ventana del sistema
typedef struct _WindowSystemData {
    GtkBuilder *builder;
    AdwApplicationWindow *window;
    
    // Header bar y botones
    AdwHeaderBar *header_bar;
    GtkButton *close_button;
    GtkButton *save_button;
    
    // Widgets principales
    AdwComboRow *shell_combo;
    AdwSwitchRow *filesystems_switch;
    AdwSwitchRow *compression_switch;
    AdwSwitchRow *video_codecs_switch;
    
    // Estados de configuración
    SystemShell current_shell;
    gboolean filesystems_enabled;
    gboolean compression_enabled;
    gboolean video_codecs_enabled;
    
    // Estado de la ventana
    gboolean is_initialized;
    gboolean is_visible;
    
} WindowSystemData;

// Funciones principales
WindowSystemData* window_system_new(void);
void window_system_init(WindowSystemData *data);
void window_system_cleanup(WindowSystemData *data);
WindowSystemData* window_system_get_instance(void);

// Funciones de interfaz
void window_system_show(WindowSystemData *data, GtkWindow *parent);
void window_system_hide(WindowSystemData *data);
void window_system_load_widgets_from_builder(WindowSystemData *data);

// Funciones de configuración
void window_system_setup_widgets(WindowSystemData *data);
void window_system_connect_signals(WindowSystemData *data);
void window_system_load_configuration(WindowSystemData *data);
void window_system_save_configuration(WindowSystemData *data);
void load_system_variables_from_file(void);
void save_system_variables_to_file(void);

// Callbacks
void on_system_close_button_clicked(GtkButton *button, gpointer user_data);
void on_system_save_button_clicked(GtkButton *button, gpointer user_data);
void on_shell_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);
void on_filesystems_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data);
void on_compression_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data);
void on_video_codecs_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data);

// Funciones de acceso a configuración
SystemShell window_system_get_shell(void);
gboolean window_system_get_filesystems_enabled(void);
gboolean window_system_get_compression_enabled(void);
gboolean window_system_get_video_codecs_enabled(void);

// Funciones de utilidad
const char* window_system_shell_to_string(SystemShell shell);
SystemShell window_system_string_to_shell(const char *shell_name);

#endif /* WINDOW_SYSTEM_H */