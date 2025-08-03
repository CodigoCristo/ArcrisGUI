#include "page7.h"
#include "page8.h"
#include "config.h"
#include <stdio.h>

#include <string.h>

// Variable global para datos de la página 7
static Page7Data *g_page7_data = NULL;

// Función principal de inicialización de la página 7
void page7_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page7_data = g_malloc0(sizeof(Page7Data));
    
    // Guardar referencias importantes
    g_page7_data->carousel = carousel;
    g_page7_data->revealer = revealer;
    
    // Cargar la página 7 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page7.ui");
    if (!page_builder) {
        LOG_ERROR("No se pudo cargar el builder de página 7");
        return;
    }
    
    // Obtener el widget principal
    GtkWidget *page7 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page7"));
    if (!page7) {
        LOG_ERROR("No se pudo cargar la página 7 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }
    
    // Guardar referencia al widget principal
    g_page7_data->main_content = page7;
    
    // Obtener widgets de Sistema Local
    g_page7_data->teclado_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "teclado_row"));
    g_page7_data->zona_horaria_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "zona_horaria_row"));
    g_page7_data->ubicacion_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "ubicacion_row"));
    
    // Obtener widgets de Selección de Disco
    g_page7_data->disco_seleccionado_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "disco_seleccionado_row"));
    g_page7_data->firmware_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "firmware_row"));
    g_page7_data->particionado_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "particionado_row"));
    
    // Obtener widgets de Usuario
    g_page7_data->nombre_usuario_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "nombre_usuario_row"));
    g_page7_data->hostname_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "hostname_row"));
    
    // Obtener widgets de Personalización
    g_page7_data->entorno_escritorio_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "entorno_escritorio_row"));
    
    // Obtener widgets de Sistema
    g_page7_data->kernel_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "kernel_row"));
    g_page7_data->drivers_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(page_builder, "drivers_expander"));
    
    // Obtener widgets de drivers expandibles
    g_page7_data->driver_video_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "driver_video_row"));
    g_page7_data->driver_audio_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "driver_audio_row"));
    g_page7_data->driver_wifi_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "driver_wifi_row"));
    g_page7_data->driver_bluetooth_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "driver_bluetooth_row"));
    
    g_page7_data->aplicaciones_base_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "aplicaciones_base_row"));
    g_page7_data->utilidades_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "utilidades_row"));
    g_page7_data->programas_extras_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "programas_extras_row"));
    
    // Obtener botones de editar - Sistema Local
    g_page7_data->edit_teclado_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_teclado_button"));
    g_page7_data->edit_zona_horaria_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_zona_horaria_button"));
    g_page7_data->edit_ubicacion_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_ubicacion_button"));
    
    // Obtener botones de editar - Selección de Disco
    g_page7_data->edit_disco_seleccionado_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_disco_seleccionado_button"));
    g_page7_data->edit_firmware_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_firmware_button"));
    g_page7_data->edit_particionado_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_particionado_button"));
    
    // Obtener botones de editar - Usuario
    g_page7_data->edit_nombre_usuario_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_nombre_usuario_button"));
    g_page7_data->edit_hostname_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_hostname_button"));
    
    // Obtener botones de editar - Personalización
    g_page7_data->edit_entorno_escritorio_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_entorno_escritorio_button"));
    
    // Obtener botones de editar - Sistema
    g_page7_data->edit_kernel_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_kernel_button"));
    g_page7_data->edit_drivers_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_drivers_button"));
    g_page7_data->edit_aplicaciones_base_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_aplicaciones_base_button"));
    g_page7_data->edit_utilidades_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_utilidades_button"));
    g_page7_data->edit_programas_extras_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "edit_programas_extras_button"));
    
    // Obtener botón de instalación
    g_page7_data->install_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "install_button"));
    
    // Verificar que se obtuvieron todos los widgets necesarios
    if (!g_page7_data->teclado_row || !g_page7_data->zona_horaria_row || !g_page7_data->ubicacion_row ||
        !g_page7_data->disco_seleccionado_row || !g_page7_data->firmware_row || !g_page7_data->particionado_row ||
        !g_page7_data->nombre_usuario_row || !g_page7_data->hostname_row ||
        !g_page7_data->entorno_escritorio_row || !g_page7_data->kernel_row ||
        !g_page7_data->drivers_expander || !g_page7_data->driver_video_row || !g_page7_data->driver_audio_row ||
        !g_page7_data->driver_wifi_row || !g_page7_data->driver_bluetooth_row ||
        !g_page7_data->aplicaciones_base_row || !g_page7_data->utilidades_row || 
        !g_page7_data->programas_extras_row || !g_page7_data->install_button) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de la página 7");
        g_object_unref(page_builder);
        return;
    }
    
    // Realizar configuraciones iniciales
    page7_setup_widgets(g_page7_data);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page7);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
    
    LOG_INFO("Página 7 (Resumen) inicializada correctamente");
}

// Función de limpieza
void page7_cleanup(Page7Data *data)
{
    if (g_page7_data) {
        g_free(g_page7_data);
        g_page7_data = NULL;
        LOG_INFO("Página 7 limpiada correctamente");
    }
}

// Función para configurar widgets y conectar señales
void page7_setup_widgets(Page7Data *data)
{
    if (!data) return;
    
    // Conectar señales de los botones de editar - Sistema Local
    g_signal_connect(data->edit_teclado_button, "clicked", 
                     G_CALLBACK(on_edit_teclado_button_clicked), data);
    g_signal_connect(data->edit_zona_horaria_button, "clicked", 
                     G_CALLBACK(on_edit_zona_horaria_button_clicked), data);
    g_signal_connect(data->edit_ubicacion_button, "clicked", 
                     G_CALLBACK(on_edit_ubicacion_button_clicked), data);
    
    // Conectar señales de los botones de editar - Selección de Disco
    g_signal_connect(data->edit_disco_seleccionado_button, "clicked", 
                     G_CALLBACK(on_edit_disco_seleccionado_button_clicked), data);
    g_signal_connect(data->edit_firmware_button, "clicked", 
                     G_CALLBACK(on_edit_firmware_button_clicked), data);
    g_signal_connect(data->edit_particionado_button, "clicked", 
                     G_CALLBACK(on_edit_particionado_button_clicked), data);
    
    // Conectar señales de los botones de editar - Usuario
    g_signal_connect(data->edit_nombre_usuario_button, "clicked", 
                     G_CALLBACK(on_edit_nombre_usuario_button_clicked), data);
    g_signal_connect(data->edit_hostname_button, "clicked", 
                     G_CALLBACK(on_edit_hostname_button_clicked), data);
    
    // Conectar señales de los botones de editar - Personalización
    g_signal_connect(data->edit_entorno_escritorio_button, "clicked", 
                     G_CALLBACK(on_edit_entorno_escritorio_button_clicked), data);
    
    // Conectar señales de los botones de editar - Sistema
    g_signal_connect(data->edit_kernel_button, "clicked", 
                     G_CALLBACK(on_edit_kernel_button_clicked), data);
    g_signal_connect(data->edit_drivers_button, "clicked", 
                     G_CALLBACK(on_edit_drivers_button_clicked), data);
    g_signal_connect(data->edit_aplicaciones_base_button, "clicked", 
                     G_CALLBACK(on_edit_aplicaciones_base_button_clicked), data);
    g_signal_connect(data->edit_utilidades_button, "clicked", 
                     G_CALLBACK(on_edit_utilidades_button_clicked), data);
    g_signal_connect(data->edit_programas_extras_button, "clicked", 
                     G_CALLBACK(on_edit_programas_extras_button_clicked), data);
    
    // Conectar señal del botón de instalación
    g_signal_connect(data->install_button, "clicked", 
                     G_CALLBACK(on_install_button_clicked), data);
    
    LOG_INFO("Widgets de la página 7 configurados y señales conectadas");
}

// Función para cargar todos los datos del resumen
void page7_load_data(Page7Data *data)
{
    if (!data) return;
    
    page7_load_sistema_local_data(data);
    page7_load_disco_data(data);
    page7_load_usuario_data(data);
    page7_load_personalizacion_data(data);
    page7_load_sistema_data(data);
    page7_load_programas_extras_data(data);
    
    LOG_INFO("Datos del resumen cargados completamente");
}

// Función para actualizar el resumen
void page7_update_summary(Page7Data *data)
{
    if (!data) return;
    
    page7_load_data(data);
    LOG_INFO("Resumen actualizado");
}

// Función para cargar datos del sistema local
void page7_load_sistema_local_data(Page7Data *data)
{
    if (!data) return;
    
    // Cargar datos del teclado
    gchar *keyboard_layout = page7_read_variable_from_file("KEYBOARD_LAYOUT");
    gchar *keymap_tty = page7_read_variable_from_file("KEYMAP_TTY");
    gchar *keyboard_info = page7_format_keyboard_info(keyboard_layout, keymap_tty);
    adw_action_row_set_subtitle(data->teclado_row, keyboard_info);
    
    // Cargar datos de zona horaria
    gchar *timezone = page7_read_variable_from_file("TIMEZONE");
    gchar *timezone_info = page7_format_timezone_info(timezone);
    adw_action_row_set_subtitle(data->zona_horaria_row, timezone_info);
    
    // Cargar datos de ubicación/idioma
    gchar *locale = page7_read_variable_from_file("LOCALE");
    gchar *locale_info = page7_format_locale_info(locale);
    adw_action_row_set_subtitle(data->ubicacion_row, locale_info);
    
    // Limpiar memoria
    g_free(keyboard_layout);
    g_free(keymap_tty);
    g_free(keyboard_info);
    g_free(timezone);
    g_free(timezone_info);
    g_free(locale);
    g_free(locale_info);
    
    LOG_INFO("Datos del sistema local cargados en el resumen");
}

// Función para cargar datos del disco
void page7_load_disco_data(Page7Data *data)
{
    if (!data) return;
    
    // Cargar datos del disco seleccionado
    gchar *selected_disk = page7_read_variable_from_file("SELECTED_DISK");
    gchar *disk_size = NULL;
    if (selected_disk) {
        disk_size = page7_get_disk_size(selected_disk);
    }
    
    gchar *disk_info;
    if (selected_disk && disk_size) {
        disk_info = g_strdup_printf("%s (%s)", selected_disk, disk_size);
    } else if (selected_disk) {
        disk_info = g_strdup(selected_disk);
    } else {
        disk_info = g_strdup("No seleccionado");
    }
    adw_action_row_set_subtitle(data->disco_seleccionado_row, disk_info);
    
    // Cargar tipo de firmware
    gchar *firmware_type = page7_get_firmware_type(selected_disk);
    const gchar *firmware_info = firmware_type ? firmware_type : "No detectado";
    adw_action_row_set_subtitle(data->firmware_row, firmware_info);
    
    // Cargar datos del tipo de particionado
    gchar *partition_mode = page7_read_variable_from_file("PARTITION_MODE");
    gchar *partition_info = page7_format_partition_mode_info(partition_mode);
    adw_action_row_set_subtitle(data->particionado_row, partition_info);
    g_free(partition_info);
    
    // Limpiar memoria
    g_free(selected_disk);
    g_free(disk_size);
    g_free(disk_info);
    g_free(firmware_type);
    g_free(partition_mode);
    
    LOG_INFO("Datos del disco cargados en el resumen");
}

// Función para cargar datos del usuario
void page7_load_usuario_data(Page7Data *data)
{
    if (!data) return;
    
    // Cargar nombre de usuario
    gchar *username = page7_read_variable_from_file("USER");
    const gchar *user_info = username ? username : "No configurado";
    adw_action_row_set_subtitle(data->nombre_usuario_row, user_info);
    
    // Cargar hostname
    gchar *hostname = page7_read_variable_from_file("HOSTNAME");
    const gchar *hostname_info = hostname ? hostname : "No configurado";
    adw_action_row_set_subtitle(data->hostname_row, hostname_info);
    
    // Limpiar memoria
    g_free(username);
    g_free(hostname);
    
    LOG_INFO("Datos del usuario cargados en el resumen");
}

// Función para cargar datos de personalización
void page7_load_personalizacion_data(Page7Data *data)
{
    if (!data) return;
    
    // Cargar tipo de instalación y entorno
    gchar *installation_type = page7_read_variable_from_file("INSTALLATION_TYPE");
    gchar *desktop_environment = page7_read_variable_from_file("DESKTOP_ENVIRONMENT");
    gchar *window_manager = page7_read_variable_from_file("WINDOW_MANAGER");
    
    // Formatear información según el tipo
    gchar *entorno_info = NULL;
    if (installation_type) {
        if (g_strcmp0(installation_type, "TERMINAL") == 0) {
            entorno_info = g_strdup("TTY (Terminal)");
        } else if (g_strcmp0(installation_type, "DESKTOP") == 0 && desktop_environment) {
            entorno_info = g_strdup_printf("DE: %s", desktop_environment);
        } else if (g_strcmp0(installation_type, "WINDOW_MANAGER") == 0 && window_manager) {
            entorno_info = g_strdup_printf("WM: %s", window_manager);
        } else {
            entorno_info = g_strdup("No configurado");
        }
    } else {
        entorno_info = g_strdup("No configurado");
    }
    
    adw_action_row_set_subtitle(data->entorno_escritorio_row, entorno_info);
    
    // Limpiar memoria
    g_free(installation_type);
    g_free(desktop_environment);
    g_free(window_manager);
    g_free(entorno_info);
    
    LOG_INFO("Datos de personalización cargados en el resumen");
}

// Función para cargar datos del sistema
void page7_load_sistema_data(Page7Data *data)
{
    if (!data) return;
    
    // Cargar kernel
    gchar *kernel = page7_read_variable_from_file("SELECTED_KERNEL");
    const gchar *kernel_info = kernel ? kernel : "linux (por defecto)";
    adw_action_row_set_subtitle(data->kernel_row, kernel_info);
    
    // Cargar información de drivers (subtitle simplificado)
    adw_expander_row_set_subtitle(data->drivers_expander, "Video | Audio | WiFi | Bluetooth");
    
    // Cargar detalles individuales de drivers
    page7_load_driver_details(data);
    
    // Cargar aplicaciones base
    gchar *essential_apps_str = page7_read_variable_from_file("ESSENTIAL_APPS_ENABLED");
    gboolean essential_apps = essential_apps_str && g_strcmp0(essential_apps_str, "true") == 0;
    const gchar *apps_info = essential_apps ? "Habilitadas" : "Deshabilitadas";
    adw_action_row_set_subtitle(data->aplicaciones_base_row, apps_info);
    
    // Cargar utilidades
    gchar *utilities_str = page7_read_variable_from_file("UTILITIES_ENABLED");
    gboolean utilities = utilities_str && g_strcmp0(utilities_str, "true") == 0;
    const gchar *utilities_info = utilities ? "Habilitadas" : "Deshabilitadas";
    adw_action_row_set_subtitle(data->utilidades_row, utilities_info);
    
    // Limpiar memoria
    g_free(kernel);
    g_free(essential_apps_str);
    g_free(utilities_str);
    
    LOG_INFO("Datos del sistema cargados en el resumen");
}

// Funciones de navegación a páginas específicas
gboolean page7_navigate_to_page2(Page7Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    GtkWidget *page2 = adw_carousel_get_nth_page(data->carousel, 1);
    if (page2) {
        adw_carousel_scroll_to(data->carousel, page2, TRUE);
        LOG_INFO("Navegación a página 2 (Sistema Local) exitosa desde resumen");
        return TRUE;
    }
    return FALSE;
}

gboolean page7_navigate_to_page3(Page7Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    GtkWidget *page3 = adw_carousel_get_nth_page(data->carousel, 2);
    if (page3) {
        adw_carousel_scroll_to(data->carousel, page3, TRUE);
        LOG_INFO("Navegación a página 3 (Selección de Disco) exitosa desde resumen");
        return TRUE;
    }
    return FALSE;
}

gboolean page7_navigate_to_page4(Page7Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    GtkWidget *page4 = adw_carousel_get_nth_page(data->carousel, 3);
    if (page4) {
        adw_carousel_scroll_to(data->carousel, page4, TRUE);
        LOG_INFO("Navegación a página 4 (Usuario) exitosa desde resumen");
        return TRUE;
    }
    return FALSE;
}

gboolean page7_navigate_to_page5(Page7Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    GtkWidget *page5 = adw_carousel_get_nth_page(data->carousel, 4);
    if (page5) {
        adw_carousel_scroll_to(data->carousel, page5, TRUE);
        LOG_INFO("Navegación a página 5 (Personalización) exitosa desde resumen");
        return TRUE;
    }
    return FALSE;
}

gboolean page7_navigate_to_page6(Page7Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    GtkWidget *page6 = adw_carousel_get_nth_page(data->carousel, 5);
    if (page6) {
        adw_carousel_scroll_to(data->carousel, page6, TRUE);
        LOG_INFO("Navegación a página 6 (Sistema) exitosa desde resumen");
        return TRUE;
    }
    return FALSE;
}

// Función para ir a la página anterior
gboolean page7_go_to_previous_page(Page7Data *data)
{
    if (!data || !data->carousel) return FALSE;
    
    GtkWidget *page6 = adw_carousel_get_nth_page(data->carousel, 5);
    if (page6) {
        adw_carousel_scroll_to(data->carousel, page6, TRUE);
        LOG_INFO("Navegación a página anterior exitosa desde página 7");
        return TRUE;
    }
    
    return FALSE;
}

// Función para verificar si es la página final
gboolean page7_is_final_page(void)
{
    return TRUE;
}

// Callbacks de los botones de editar - Sistema Local
void on_edit_teclado_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page2(data)) {
        LOG_INFO("Editando configuración del teclado");
    } else {
        LOG_WARNING("No se pudo navegar a la página de configuración del teclado");
    }
}

void on_edit_zona_horaria_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page2(data)) {
        LOG_INFO("Editando zona horaria");
    } else {
        LOG_WARNING("No se pudo navegar a la página de configuración de zona horaria");
    }
}

void on_edit_ubicacion_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page2(data)) {
        LOG_INFO("Editando ubicación e idioma");
    } else {
        LOG_WARNING("No se pudo navegar a la página de configuración de ubicación");
    }
}

// Callbacks de los botones de editar - Selección de Disco
void on_edit_disco_seleccionado_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page3(data)) {
        LOG_INFO("Editando selección de disco");
    } else {
        LOG_WARNING("No se pudo navegar a la página de selección de disco");
    }
}

void on_edit_firmware_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page3(data)) {
        LOG_INFO("Editando configuración de firmware");
    } else {
        LOG_WARNING("No se pudo navegar a la página de configuración de firmware");
    }
}

void on_edit_particionado_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page3(data)) {
        LOG_INFO("Editando tipo de particionado");
    } else {
        LOG_WARNING("No se pudo navegar a la página de particionado");
    }
}

// Callbacks de los botones de editar - Usuario
void on_edit_nombre_usuario_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page4(data)) {
        LOG_INFO("Editando nombre de usuario");
    } else {
        LOG_WARNING("No se pudo navegar a la página de usuario");
    }
}

void on_edit_hostname_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page4(data)) {
        LOG_INFO("Editando hostname");
    } else {
        LOG_WARNING("No se pudo navegar a la página de hostname");
    }
}

// Callbacks de los botones de editar - Personalización
void on_edit_entorno_escritorio_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page5(data)) {
        LOG_INFO("Editando entorno de escritorio");
    } else {
        LOG_WARNING("No se pudo navegar a la página de personalización");
    }
}

void on_edit_programas_extras_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page6(data)) {
        LOG_INFO("Editando programas extras");
    } else {
        LOG_WARNING("No se pudo navegar a la página de programas extras");
    }
}

// Callbacks de los botones de editar - Sistema
void on_edit_kernel_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page6(data)) {
        LOG_INFO("Editando kernel");
    } else {
        LOG_WARNING("No se pudo navegar a la página de configuración del kernel");
    }
}

void on_edit_drivers_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page6(data)) {
        LOG_INFO("Editando drivers");
    } else {
        LOG_WARNING("No se pudo navegar a la página de configuración de drivers");
    }
}

void on_edit_aplicaciones_base_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page6(data)) {
        LOG_INFO("Editando aplicaciones base");
    } else {
        LOG_WARNING("No se pudo navegar a la página de aplicaciones base");
    }
}

void on_edit_utilidades_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_navigate_to_page6(data)) {
        LOG_INFO("Editando utilidades");
    } else {
        LOG_WARNING("No se pudo navegar a la página de utilidades");
    }
}

// Callback del botón de instalación
void on_install_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    LOG_INFO("=== DEBUG: on_install_button_clicked INICIADO ===");
    LOG_INFO("Iniciando instalación del sistema - navegando a página 8...");
    
    if (!data) {
        LOG_ERROR("DEBUG: data es NULL!");
        return;
    }
    LOG_INFO("DEBUG: data válido");
    
    // Deshabilitar el botón durante la instalación
    gtk_widget_set_sensitive(GTK_WIDGET(data->install_button), FALSE);
    adw_button_content_set_label(
        ADW_BUTTON_CONTENT(gtk_button_get_child(data->install_button)),
        "Instalando..."
    );
    
    // Inicializar page8 si no está inicializada
    LOG_INFO("DEBUG: Obteniendo page8_data...");
    Page8Data *page8_data = page8_get_data();
    if (!page8_data) {
        LOG_INFO("DEBUG: page8_data es NULL, inicializando página 8...");
        LOG_INFO("DEBUG: carousel = %p, revealer = %p", data->carousel, data->revealer);
        page8_init(NULL, data->carousel, data->revealer);
        LOG_INFO("DEBUG: page8_init completado");
        
        // Obtener page8_data después de la inicialización
        page8_data = page8_get_data();
        LOG_INFO("DEBUG: page8_data después de init = %p", page8_data);
    } else {
        LOG_INFO("DEBUG: page8_data ya existe");
    }
    
    // Navegar a la página 8 (instalación)
    LOG_INFO("DEBUG: Verificando carousel...");
    if (data->carousel) {
        LOG_INFO("DEBUG: carousel válido, obteniendo información...");
        guint current_page = adw_carousel_get_position(data->carousel);
        guint total_pages = adw_carousel_get_n_pages(data->carousel);
        
        LOG_INFO("DEBUG: current_page = %u, total_pages = %u", current_page, total_pages);
        LOG_INFO("Navegando de página %u a página 8 (total: %u)", current_page, total_pages);
        
        // La página 8 es la penúltima página (índice 7), page9 es la última (índice 8)
        guint page8_index = total_pages - 2;
        LOG_INFO("DEBUG: Calculado page8_index = %u", page8_index);
        
        GtkWidget *page8_widget = adw_carousel_get_nth_page(data->carousel, page8_index);
        LOG_INFO("DEBUG: page8_widget = %p", page8_widget);
        
        if (page8_widget) {
            LOG_INFO("DEBUG: page8_widget válido, ejecutando scroll_to...");
            adw_carousel_scroll_to(data->carousel, page8_widget, TRUE);
            LOG_INFO("DEBUG: adw_carousel_scroll_to ejecutado");
            LOG_INFO("Navegación a página 8 completada - mostrando animación del carousel");
            
            // Ejecutar el script de instalación después de navegar exitosamente
            if (page8_data) {
                LOG_INFO("DEBUG: Iniciando script de instalación...");
                page8_start_installation(page8_data);
                LOG_INFO("DEBUG: Script de instalación iniciado");
            } else {
                LOG_ERROR("DEBUG: No se puede ejecutar instalación - page8_data es NULL");
            }
        } else {
            LOG_ERROR("DEBUG: No se pudo encontrar la página 8 en el carousel (widget NULL)");
            LOG_ERROR("DEBUG: Verificando si page8 fue agregado al carousel...");
        }
    } else {
        LOG_ERROR("DEBUG: Carousel es NULL - no disponible para navegar a página 8");
    }
    
    LOG_INFO("DEBUG: Proceso de instalación iniciado en página 8");
    LOG_INFO("=== DEBUG: on_install_button_clicked FINALIZADO ===");
}

// Callback de navegación hacia atrás
void on_page7_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page7Data *data = (Page7Data *)user_data;
    
    if (page7_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 7");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 7");
    }
}

// Función llamada cuando se muestra la página 7
void page7_on_page_shown(void)
{
    LOG_INFO("Página 7 mostrada - Actualizando resumen");
    if (g_page7_data) {
        page7_update_summary(g_page7_data);
        
        // El revealer se oculta automáticamente mediante carousel_update_navigation_controls
    }
}

// Función para obtener el widget principal
GtkWidget* page7_get_widget(void)
{
    if (!g_page7_data) return NULL;
    return g_page7_data->main_content;
}

// Funciones auxiliares para leer variables.sh
gchar* page7_read_variable_from_file(const gchar* variable_name)
{
    if (!variable_name) return NULL;
    
    gchar *file_path = g_build_filename(".", "data", "variables.sh", NULL);
    FILE *file = fopen(file_path, "r");
    
    if (!file) {
        LOG_WARNING("No se pudo abrir el archivo variables.sh");
        g_free(file_path);
        return NULL;
    }
    
    gchar line[1024];
    gchar *result = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        // Remover comentarios y líneas vacías
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Crear patrón de búsqueda para variable con o sin export
        gchar *pattern1 = g_strdup_printf("%s=", variable_name);
        gchar *pattern2 = g_strdup_printf("export %s=", variable_name);
        
        // Buscar la variable (con o sin export)
        if (g_str_has_prefix(line, pattern1) || g_str_has_prefix(line, pattern2)) {
            gchar *equals_pos = strchr(line, '=');
            if (equals_pos) {
                equals_pos++; // Saltar el '='
                
                // Remover comillas y espacios
                gchar *value = g_strstrip(equals_pos);
                if (value[0] == '"' && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = '\0';
                    value++;
                }
                
                result = g_strdup(value);
                g_free(pattern1);
                g_free(pattern2);
                break;
            }
        }
        
        g_free(pattern1);
        g_free(pattern2);
    }
    
    fclose(file);
    g_free(file_path);
    
    return result;
}

// Función para obtener el tamaño del disco
gchar* page7_get_disk_size(const gchar* disk_path)
{
    if (!disk_path) return NULL;
    
    // Comando para obtener el tamaño del disco
    gchar *command = g_strdup_printf("lsblk -b -d -n -o SIZE %s 2>/dev/null", disk_path);
    
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        g_free(command);
        return NULL;
    }
    
    gchar buffer[256];
    gchar *result = NULL;
    
    if (fgets(buffer, sizeof(buffer), pipe)) {
        guint64 size_bytes = g_ascii_strtoull(g_strstrip(buffer), NULL, 10);
        
        // Convertir a unidades legibles
        if (size_bytes >= (1024ULL * 1024 * 1024 * 1024)) {
            result = g_strdup_printf("%.1f TB", (double)size_bytes / (1024.0 * 1024 * 1024 * 1024));
        } else if (size_bytes >= (1024ULL * 1024 * 1024)) {
            result = g_strdup_printf("%.1f GB", (double)size_bytes / (1024.0 * 1024 * 1024));
        } else if (size_bytes >= (1024ULL * 1024)) {
            result = g_strdup_printf("%.1f MB", (double)size_bytes / (1024.0 * 1024));
        } else {
            result = g_strdup_printf("%lu B", size_bytes);
        }
    }
    
    pclose(pipe);
    g_free(command);
    
    return result;
}

// Función para formatear información del disco
gchar* page7_format_disk_info(const gchar* disk_path, const gchar* partition_mode)
{
    const gchar *disk_name = disk_path ? disk_path : "No seleccionado";
    const gchar *mode_name = "Desconocido";
    
    if (partition_mode) {
        if (g_strcmp0(partition_mode, "auto") == 0) {
            mode_name = "Automático";
        } else if (g_strcmp0(partition_mode, "auto_btrfs") == 0) {
            mode_name = "Automático (Btrfs)";
        } else if (g_strcmp0(partition_mode, "manual") == 0) {
            mode_name = "Manual";
        }
    }
    
    return g_strdup_printf("Disco: %s | Particionado: %s", disk_name, mode_name);
}

// Función para formatear información del usuario
gchar* page7_format_user_info(const gchar* username, const gchar* hostname)
{
    const gchar *user = username ? username : "No configurado";
    const gchar *host = hostname ? hostname : "No configurado";
    
    return g_strdup_printf("Usuario: %s | Hostname: %s", user, host);
}

// Función para formatear información de personalización
gchar* page7_format_personalization_info(const gchar* installation_type)
{
    const gchar *type_name = "No configurado";
    
    if (installation_type) {
        if (g_strcmp0(installation_type, "TERMINAL") == 0) {
            type_name = "TTY (Terminal)";
        } else if (g_strcmp0(installation_type, "DESKTOP") == 0) {
            type_name = "Entorno de Escritorio";
        } else if (g_strcmp0(installation_type, "WINDOW_MANAGER") == 0) {
            type_name = "Gestor de Ventanas";
        }
    }
    
    return g_strdup_printf("%s", type_name);
}

// Función para formatear información del sistema
gchar* page7_format_system_info(const gchar* kernel, const gchar* drivers, gboolean essential_apps, gboolean utilities)
{
    const gchar *kernel_name = kernel ? kernel : "linux";
    
    GString *info = g_string_new("");
    g_string_append_printf(info, "Kernel: %s | Drivers: %s", kernel_name, drivers);
    
    if (essential_apps || utilities) {
        g_string_append(info, " | Apps: ");
        if (essential_apps) {
            g_string_append(info, "Esenciales");
        }
        if (essential_apps && utilities) {
            g_string_append(info, ", ");
        }
        if (utilities) {
            g_string_append(info, "Utilidades");
        }
    }
    
    gchar *result = g_string_free(info, FALSE);
    return result;
}

// Función para formatear información del teclado
gchar* page7_format_keyboard_info(const gchar* keyboard_layout, const gchar* keymap_tty)
{
    const gchar *layout = keyboard_layout ? keyboard_layout : "No configurado";
    const gchar *tty_keymap = keymap_tty ? keymap_tty : "No configurado";
    
    return g_strdup_printf("%s (TTY: %s)", layout, tty_keymap);
}

// Función para formatear información de zona horaria
gchar* page7_format_timezone_info(const gchar* timezone)
{
    if (!timezone) {
        return g_strdup("No configurado");
    }
    
    // Simplificar la zona horaria para mostrar solo la ciudad/región
    gchar **parts = g_strsplit(timezone, "/", -1);
    if (parts && parts[0] && parts[1]) {
        gchar *result = g_strdup_printf("%s/%s", parts[0], parts[1]);
        g_strfreev(parts);
        return result;
    }
    
    g_strfreev(parts);
    return g_strdup(timezone);
}

// Función para formatear información de ubicación/idioma
gchar* page7_format_locale_info(const gchar* locale)
{
    if (!locale) {
        return g_strdup("No configurado");
    }
    
    // Extraer el código de idioma y país del locale
    gchar **parts = g_strsplit(locale, "_", 2);
    if (parts && parts[0] && parts[1]) {
        gchar **country_parts = g_strsplit(parts[1], ".", 2);
        if (country_parts && country_parts[0]) {
            gchar *result = g_strdup_printf("%s_%s", parts[0], country_parts[0]);
            g_strfreev(parts);
            g_strfreev(country_parts);
            return result;
        }
        g_strfreev(country_parts);
    }
    
    g_strfreev(parts);
    return g_strdup(locale);
}

// Función para formatear información del modo de partición
gchar* page7_format_partition_mode_info(const gchar* partition_mode)
{
    if (!partition_mode) {
        return g_strdup("No configurado");
    }
    
    if (g_strcmp0(partition_mode, "auto") == 0) {
        return g_strdup("Automático (ext4)");
    } else if (g_strcmp0(partition_mode, "auto_btrfs") == 0) {
        return g_strdup("Automático (Btrfs)");
    } else if (g_strcmp0(partition_mode, "manual") == 0) {
        return g_strdup("Manual");
    }
    
    return g_strdup("Desconocido");
}

// Función para formatear información de drivers (formato simple)
gchar* page7_format_drivers_info(void)
{
    return g_strdup("Video | Audio | WiFi | Bluetooth");
}

// Función para formatear información completa del disco
gchar* page7_format_disk_complete_info(const gchar* disk_path, const gchar* firmware_type, const gchar* partition_mode)
{
    const gchar *disk_name = disk_path ? disk_path : "No seleccionado";
    const gchar *firmware = firmware_type ? firmware_type : "BIOS";
    const gchar *mode_name = "Automático";
    
    if (partition_mode) {
        if (g_strcmp0(partition_mode, "auto") == 0) {
            mode_name = "Automático";
        } else if (g_strcmp0(partition_mode, "auto_btrfs") == 0) {
            mode_name = "Automático (Btrfs)";
        } else if (g_strcmp0(partition_mode, "manual") == 0) {
            mode_name = "Manual";
        }
    }
    
    return g_strdup_printf("%s - %s - %s", disk_name, firmware, mode_name);
}

// Función para obtener el tipo de firmware (BIOS/UEFI)
gchar* page7_get_firmware_type(const gchar* disk_path)
{
    // Verificar si el sistema usa UEFI
    if (g_file_test("/sys/firmware/efi", G_FILE_TEST_EXISTS)) {
        return g_strdup("UEFI");
    } else {
        return g_strdup("LEGACY BIOS");
    }
}

// Función para cargar detalles de drivers expandibles
void page7_load_driver_details(Page7Data *data)
{
    if (!data) return;
    
    // Cargar driver de video
    gchar *video_driver = page7_read_variable_from_file("DRIVER_VIDEO");
    const gchar *video_info = video_driver ? video_driver : "No configurado";
    adw_action_row_set_subtitle(data->driver_video_row, video_info);
    
    // Cargar driver de audio
    gchar *audio_driver = page7_read_variable_from_file("DRIVER_AUDIO");
    const gchar *audio_info = audio_driver ? audio_driver : "No configurado";
    adw_action_row_set_subtitle(data->driver_audio_row, audio_info);
    
    // Cargar driver de WiFi
    gchar *wifi_driver = page7_read_variable_from_file("DRIVER_WIFI");
    const gchar *wifi_info = wifi_driver ? wifi_driver : "No configurado";
    adw_action_row_set_subtitle(data->driver_wifi_row, wifi_info);
    
    // Cargar driver de Bluetooth
    gchar *bluetooth_driver = page7_read_variable_from_file("DRIVER_BLUETOOTH");
    const gchar *bluetooth_info = bluetooth_driver ? bluetooth_driver : "No configurado";
    adw_action_row_set_subtitle(data->driver_bluetooth_row, bluetooth_info);
    
    // Limpiar memoria
    g_free(video_driver);
    g_free(audio_driver);
    g_free(wifi_driver);
    g_free(bluetooth_driver);
    
    LOG_INFO("Detalles de drivers cargados");
}

// Función para cargar datos de programas extras
// Función auxiliar para eliminar TODOS los rows existentes de programas
void page7_remove_existing_programs_row(Page7Data *data)
{
    if (!data || !data->programas_extras_row) return;
    
    // Ya no necesitamos remover sub-rows, solo limpiar para la próxima actualización
    LOG_INFO("Preparando para actualizar subtitle de programas extras");
}

// Función auxiliar para crear y agregar un nuevo row con contenido
void page7_add_new_programs_row(Page7Data *data, const gchar *content)
{
    if (!data || !data->programas_extras_row || !content) return;
    
    LOG_INFO("Actualizando subtitle de programas extras con contenido: %s", content);
    // Actualizar el subtitle del AdwActionRow principal en lugar de agregar sub-rows
    adw_action_row_set_subtitle(data->programas_extras_row, content);
}

void page7_load_programas_extras_data(Page7Data *data)
{
    if (!data || !data->programas_extras_row) return;
    
    LOG_INFO("Carga inicial de programas extras");
    
    // Cargar los programas desde el archivo variables.sh si existen
    gchar *programs_text = page7_read_variable_from_file("EXTRA_PROGRAMS");
    if (programs_text && strlen(g_strstrip(programs_text)) > 0) {
        // Usar la función de actualización para procesar el texto
        page7_update_programas_extras_subtitle(programs_text);
    } else {
        // Sin programas, mostrar mensaje de sin programas extras
        adw_action_row_set_subtitle(data->programas_extras_row, "Sin Programas Extras");
    }
    
    if (programs_text) g_free(programs_text);
    LOG_INFO("Carga inicial completada");
}

// Función para obtener los datos globales de page7
void page7_update_programas_extras_subtitle(const gchar *programs_text)
{
    Page7Data *data = page7_get_data();
    if (!data || !data->programas_extras_row) {
        LOG_WARNING("No se puede actualizar subtitle de programas extras: datos no disponibles");
        return;
    }

    LOG_INFO("Actualizando subtitle de programas extras con texto: '%s'", programs_text ? programs_text : "NULL");

    if (programs_text) {
        // Verificar si hay contenido real
        gchar *temp_check = g_strdup(programs_text);
        g_strstrip(temp_check);
        gboolean has_content = strlen(temp_check) > 0;
        g_free(temp_check);
        
        if (has_content) {
            // Crear una copia limpia del texto
            gchar *working_text = g_strdup(programs_text);
            g_strstrip(working_text);
        
        // Remover paréntesis si están presentes (formato de array bash)
        if (g_str_has_prefix(working_text, "(") && g_str_has_suffix(working_text, ")")) {
            gchar *temp = g_strndup(working_text + 1, strlen(working_text) - 2);
            g_free(working_text);
            working_text = temp;
            g_strstrip(working_text);
        }
        
        // Función simple para limpiar comillas
        GString *clean_string = g_string_new("");
        for (int i = 0; working_text[i] != '\0'; i++) {
            char c = working_text[i];
            if (c != '"' && c != '\'') {
                g_string_append_c(clean_string, c);
            } else {
                // Reemplazar comillas con espacios
                g_string_append_c(clean_string, ' ');
            }
        }
        
        // Dividir por espacios y crear lista con comas
        gchar **words = g_strsplit(clean_string->str, " ", -1);
        GString *final_list = g_string_new("");
        
        int word_count = 0;
        for (int i = 0; words[i] != NULL; i++) {
            gchar *word = g_strstrip(words[i]);
            if (strlen(word) > 0) {
                word_count++;
                if (final_list->len > 0) {
                    g_string_append(final_list, ", ");
                }
                g_string_append(final_list, word);
            }
        }
        
        if (word_count > 0) {
            LOG_INFO("Actualizando subtitle con %d programas: '%s'", word_count, final_list->str);
            adw_action_row_set_subtitle(data->programas_extras_row, final_list->str);
        } else {
            LOG_INFO("Sin programas válidos, usando mensaje de sin programas");
            adw_action_row_set_subtitle(data->programas_extras_row, "Sin Programas Extras");
        }
        
        // Liberar memoria
        g_string_free(clean_string, TRUE);
        g_string_free(final_list, TRUE);
            g_strfreev(words);
            g_free(working_text);
        } else {
            // Sin programas, mostrar mensaje de sin programas
            LOG_INFO("Texto vacío después de limpiar, mostrando mensaje de sin programas");
            adw_action_row_set_subtitle(data->programas_extras_row, "Sin Programas Extras");
        }
    } else {
        // Sin programas, mostrar mensaje de sin programas
        LOG_INFO("Texto vacío o NULL, mostrando mensaje de sin programas");
        adw_action_row_set_subtitle(data->programas_extras_row, "Sin Programas Extras");
    }
    
    LOG_INFO("Subtitle de programas extras actualizado correctamente");
}

Page7Data* page7_get_data(void)
{
    return g_page7_data;
}