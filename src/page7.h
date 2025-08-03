#ifndef PAGE7_H
#define PAGE7_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Estructura para datos de la página 7
typedef struct _Page7Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Widgets de Sistema Local
    AdwActionRow *teclado_row;
    AdwActionRow *zona_horaria_row;
    AdwActionRow *ubicacion_row;
    
    // Widgets de Selección de Disco
    AdwActionRow *disco_seleccionado_row;
    AdwActionRow *firmware_row;
    AdwActionRow *particionado_row;
    
    // Widgets de Usuario
    AdwActionRow *nombre_usuario_row;
    AdwActionRow *hostname_row;
    
    // Widgets de Personalización
    AdwActionRow *entorno_escritorio_row;
    
    // Widgets de Sistema
    AdwActionRow *kernel_row;
    AdwExpanderRow *drivers_expander;
    
    // Widgets de drivers (expandibles)
    AdwActionRow *driver_video_row;
    AdwActionRow *driver_audio_row;
    AdwActionRow *driver_wifi_row;
    AdwActionRow *driver_bluetooth_row;
    
    AdwActionRow *aplicaciones_base_row;
    AdwActionRow *utilidades_row;
    AdwActionRow *programas_extras_row;
    
    // Botones de editar - Sistema Local
    GtkButton *edit_teclado_button;
    GtkButton *edit_zona_horaria_button;
    GtkButton *edit_ubicacion_button;
    
    // Botones de editar - Selección de Disco
    GtkButton *edit_disco_seleccionado_button;
    GtkButton *edit_firmware_button;
    GtkButton *edit_particionado_button;
    
    // Botones de editar - Usuario
    GtkButton *edit_nombre_usuario_button;
    GtkButton *edit_hostname_button;
    
    // Botones de editar - Personalización
    GtkButton *edit_entorno_escritorio_button;
    
    // Botones de editar - Sistema
    GtkButton *edit_kernel_button;
    GtkButton *edit_drivers_button;
    GtkButton *edit_aplicaciones_base_button;
    GtkButton *edit_utilidades_button;
    GtkButton *edit_programas_extras_button;
    
    // Botón de instalación
    GtkButton *install_button;
    
} Page7Data;

// Funciones principales de la página 7
void page7_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page7_cleanup(Page7Data *data);

// Funciones de configuración
void page7_setup_widgets(Page7Data *data);
void page7_load_data(Page7Data *data);
void page7_update_summary(Page7Data *data);

// Funciones de carga de datos específicos
void page7_load_sistema_local_data(Page7Data *data);
void page7_load_disco_data(Page7Data *data);
void page7_load_usuario_data(Page7Data *data);
void page7_load_personalizacion_data(Page7Data *data);
void page7_load_sistema_data(Page7Data *data);

// Funciones de navegación a páginas específicas
gboolean page7_navigate_to_page2(Page7Data *data);
gboolean page7_navigate_to_page3(Page7Data *data);
gboolean page7_navigate_to_page4(Page7Data *data);
gboolean page7_navigate_to_page5(Page7Data *data);
gboolean page7_navigate_to_page6(Page7Data *data);

// Funciones de navegación general
gboolean page7_go_to_previous_page(Page7Data *data);
gboolean page7_is_final_page(void);

// Funciones de estado
void page7_on_page_shown(void);
GtkWidget* page7_get_widget(void);

// Callbacks de los botones de editar - Sistema Local
void on_edit_teclado_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_zona_horaria_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_ubicacion_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de los botones de editar - Selección de Disco
void on_edit_disco_seleccionado_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_firmware_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_particionado_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de los botones de editar - Usuario
void on_edit_nombre_usuario_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_hostname_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de los botones de editar - Personalización
void on_edit_entorno_escritorio_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de los botones de editar - Sistema
void on_edit_kernel_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_drivers_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_aplicaciones_base_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_utilidades_button_clicked(GtkButton *button, gpointer user_data);
void on_edit_programas_extras_button_clicked(GtkButton *button, gpointer user_data);

// Callback del botón de instalación
void on_install_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de navegación
void on_page7_back_button_clicked(GtkButton *button, gpointer user_data);

// Funciones auxiliares para leer variables.sh
gchar* page7_read_variable_from_file(const gchar* variable_name);
gchar* page7_get_disk_size(const gchar* disk_path);
gchar* page7_format_disk_info(const gchar* disk_path, const gchar* partition_mode);
gchar* page7_format_user_info(const gchar* username, const gchar* hostname);
gchar* page7_format_personalization_info(const gchar* installation_type);
gchar* page7_format_system_info(const gchar* kernel, const gchar* drivers, gboolean essential_apps, gboolean utilities);

// Funciones auxiliares para formatear información específica
gchar* page7_format_keyboard_info(const gchar* keyboard_layout, const gchar* keymap_tty);
gchar* page7_format_timezone_info(const gchar* timezone);
gchar* page7_format_locale_info(const gchar* locale);
gchar* page7_format_partition_mode_info(const gchar* partition_mode);
gchar* page7_format_drivers_info(void);
gchar* page7_format_disk_complete_info(const gchar* disk_path, const gchar* firmware_type, const gchar* partition_mode);
gchar* page7_get_firmware_type(const gchar* disk_path);

// Funciones para manejo de drivers expandibles
void page7_load_driver_details(Page7Data *data);

// Funciones para manejo de programas extras
void page7_load_programas_extras_data(Page7Data *data);
void page7_remove_existing_programs_row(Page7Data *data);
void page7_add_new_programs_row(Page7Data *data, const gchar *content);
void page7_update_programas_extras_subtitle(const gchar *programs_text);

// Función para obtener datos globales de page7
Page7Data* page7_get_data(void);

#endif /* PAGE7_H */