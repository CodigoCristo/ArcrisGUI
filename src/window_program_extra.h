#ifndef WINDOW_PROGRAM_EXTRA_H
#define WINDOW_PROGRAM_EXTRA_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Estructura para datos de la ventana de programas extra
typedef struct _WindowProgramExtraData {
    GtkWindow *window;
    GtkBuilder *builder;
    
    // Botones de la ventana
    GtkButton *close_button;
    GtkButton *save_button;
    
    // TextView y buffer para programas extra
    GtkTextView *hardware_textview;
    GtkTextBuffer *text_buffer;
    
    // Estado de inicialización
    gboolean is_initialized;
    
    // Texto de los programas guardados
    gchar *programs_text;
    
} WindowProgramExtraData;

// Funciones principales de la ventana
WindowProgramExtraData* window_program_extra_new(void);
void window_program_extra_init(WindowProgramExtraData *data);
void window_program_extra_cleanup(WindowProgramExtraData *data);
void window_program_extra_show(WindowProgramExtraData *data, GtkWindow *parent);
void window_program_extra_hide(WindowProgramExtraData *data);

// Funciones de configuración
void window_program_extra_setup_widgets(WindowProgramExtraData *data);
void window_program_extra_connect_signals(WindowProgramExtraData *data);
void window_program_extra_load_widgets_from_builder(WindowProgramExtraData *data);

// Funciones de manejo del TextView principal de programas
void window_program_extra_setup_textview(WindowProgramExtraData *data);

// Funciones de persistencia
gboolean window_program_extra_load_programs_from_file(WindowProgramExtraData *data);
gboolean window_program_extra_save_programs_to_file(WindowProgramExtraData *data);
gchar* window_program_extra_get_programs_text(WindowProgramExtraData *data);
void window_program_extra_set_programs_text(WindowProgramExtraData *data, const gchar *text);

// Callbacks de botones
void on_program_extra_close_button_clicked(GtkButton *button, gpointer user_data);
void on_program_extra_save_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks del TextView principal
void on_program_extra_textbuffer_changed(GtkTextBuffer *buffer, gpointer user_data);

// Funciones de utilidad
void window_program_extra_reset_to_defaults(WindowProgramExtraData *data);
gboolean window_program_extra_validate_programs_text(const gchar *text);

// Función para obtener la instancia global
WindowProgramExtraData* window_program_extra_get_instance(void);

#endif /* WINDOW_PROGRAM_EXTRA_H */