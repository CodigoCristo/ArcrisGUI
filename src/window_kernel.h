#ifndef WINDOW_KERNEL_H
#define WINDOW_KERNEL_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Enumeración para tipos de kernel
typedef enum {
    KERNEL_LINUX = 0,
    KERNEL_HARDENED,
    KERNEL_LTS,
    KERNEL_RT_LTS,
    KERNEL_ZEN
} KernelType;

// Estructura para datos de la ventana de kernel
typedef struct _WindowKernelData {
    GtkWindow *window;
    GtkBuilder *builder;
    
    // Botones de la ventana
    GtkButton *close_button;
    GtkButton *save_button;
    
    // Radio buttons para selección de kernel
    GtkCheckButton *kernel_linux_radio;
    GtkCheckButton *hardened_radio;
    GtkCheckButton *lts_radio;
    GtkCheckButton *rt_lts_radio;
    GtkCheckButton *zen_radio;
    
    // Estado actual
    KernelType current_kernel;
    gboolean is_initialized;
    
} WindowKernelData;

// Funciones principales de la ventana
WindowKernelData* window_kernel_new(void);
void window_kernel_init(WindowKernelData *data);
void window_kernel_cleanup(WindowKernelData *data);
void window_kernel_show(WindowKernelData *data, GtkWindow *parent);
void window_kernel_hide(WindowKernelData *data);

// Funciones de configuración
void window_kernel_setup_widgets(WindowKernelData *data);
void window_kernel_connect_signals(WindowKernelData *data);
void window_kernel_load_widgets_from_builder(WindowKernelData *data);

// Funciones de gestión de kernel
KernelType window_kernel_get_selected_kernel(WindowKernelData *data);
void window_kernel_set_selected_kernel(WindowKernelData *data, KernelType kernel);
const char* window_kernel_get_kernel_name(KernelType kernel);
KernelType window_kernel_get_kernel_from_name(const char* name);

// Funciones de persistencia (variables.sh)
gboolean window_kernel_load_from_variables(WindowKernelData *data);
gboolean window_kernel_save_to_variables(WindowKernelData *data);
gboolean window_kernel_save_kernel_variable(KernelType kernel);

// Callbacks de botones
void on_kernel_close_button_clicked(GtkButton *button, gpointer user_data);
void on_kernel_save_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de radio buttons
void on_kernel_linux_radio_toggled(GtkCheckButton *radio, gpointer user_data);
void on_kernel_hardened_radio_toggled(GtkCheckButton *radio, gpointer user_data);
void on_kernel_lts_radio_toggled(GtkCheckButton *radio, gpointer user_data);
void on_kernel_rt_lts_radio_toggled(GtkCheckButton *radio, gpointer user_data);
void on_kernel_zen_radio_toggled(GtkCheckButton *radio, gpointer user_data);

// Funciones de utilidad
gboolean window_kernel_is_valid_kernel_type(KernelType kernel);
void window_kernel_update_ui_selection(WindowKernelData *data);
void window_kernel_reset_to_default(WindowKernelData *data);

// Funciones de logging específicas
void window_kernel_log_selection_change(KernelType old_kernel, KernelType new_kernel);

// Función para obtener la instancia global
WindowKernelData* window_kernel_get_instance(void);

#endif /* WINDOW_KERNEL_H */