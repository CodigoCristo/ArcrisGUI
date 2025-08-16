#ifndef PAGE9_H
#define PAGE9_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include "types.h"

// Estructura de datos para la página 9 (finalización)
typedef struct _Page9Data {
    // Referencias principales
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Widgets de la interfaz
    GtkImage *success_icon;
    GtkLabel *completion_message;
    GtkLabel *secondary_message;
    GtkLabel *info_label;
    
    // Botones de acción
    GtkButton *restart_button;
    GtkButton *exit_button;
    
    // Estado de la página
    gboolean is_initialized;
    gboolean show_completion_animation;
    
} Page9Data;

// Funciones principales
void page9_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page9_cleanup(void);
Page9Data* page9_get_data(void);

// Funciones de configuración
void page9_setup_widgets(Page9Data *data);
void page9_setup_styles(Page9Data *data);
void page9_load_data(Page9Data *data);

// Funciones de animación y efectos
void page9_show_completion_animation(Page9Data *data);
void page9_fade_in_elements(Page9Data *data);

// Callbacks de botones
void on_restart_button_clicked(GtkButton *button, gpointer user_data);
void on_exit_button_clicked(GtkButton *button, gpointer user_data);



// Funciones de navegación
void page9_on_page_shown(void);
void page9_on_page_hidden(void);

// Funciones de sistema
void page9_execute_restart(void);
void page9_execute_exit(void);

// Funciones de validación
gboolean page9_can_restart(void);
gboolean page9_can_exit(void);

// Funciones auxiliares para verificaciones del sistema
gboolean page9_check_critical_processes(void);
gboolean page9_check_root_permissions(void);
gboolean page9_check_system_state(void);
gboolean page9_check_installation_in_progress(void);
gboolean page9_check_installer_processes(void);

// Funciones de manejo de archivos de bloqueo
void page9_create_installation_lock(void);
void page9_remove_installation_lock(void);

#endif // PAGE9_H