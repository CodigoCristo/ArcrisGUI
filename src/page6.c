#include "page6.h"
#include "variables_utils.h"
#include "i18n.h"
#include "window_kernel.h"
#include "window_hardware.h"
#include "window_system.h"
#include "window_program_extra.h"
#include "window_apps.h"
#include "window_repos.h"
#include "config.h"
#include <stdio.h>

// Variable global para datos de la página 6
static Page6Data *g_page6_data = NULL;

// Función principal de inicialización de la página 6
void page6_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page6_data = g_malloc0(sizeof(Page6Data));

    // Guardar referencias importantes
    g_page6_data->carousel = carousel;
    g_page6_data->revealer = revealer;

    // Cargar la página 6 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page6.ui");
    if (!page_builder) {
        LOG_ERROR("No se pudo cargar el builder de página 6");
        return;
    }

    // Obtener el widget principal (AdwStatusPage como raíz)
    GtkWidget *page6 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page6_status"));
    if (!page6) {
        LOG_ERROR("No se pudo cargar la página 6 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }

    // Guardar referencia al widget principal
    g_page6_data->main_content = page6;

    // Obtener widgets específicos de la interfaz
    page6_get_ui_widgets(g_page6_data, page_builder);

    // Realizar configuraciones iniciales
    page6_setup_widgets(g_page6_data);
    page6_load_data(g_page6_data);
    page6_connect_signals(g_page6_data);

    // Añadir la página al carousel
    adw_carousel_append(carousel, page6);

    // Liberar el builder de la página
    g_object_unref(page_builder);

    // Guardar el kernel por defecto (linux) al iniciar la aplicación
    window_kernel_save_kernel_variable(KERNEL_LINUX);

    // Inicializar variables de repositorios si no existen
    window_repos_init_defaults();

    // Mostrar el kernel actualmente seleccionado
    page6_display_current_kernel();

    // Cargar estado previo de los switches desde variables.sh
    load_page6_switches_from_file();

    // Guardar estado inicial de los switches en variables.sh
    save_page6_switches_to_file();

    LOG_INFO("Página 6 (Sistema) inicializada correctamente");
}

// Función para obtener widgets de la UI
void page6_get_ui_widgets(Page6Data *data, GtkBuilder *builder)
{
    if (!data || !builder) return;

    // Obtener el botón del kernel
    data->kernel_button = GTK_BUTTON(gtk_builder_get_object(builder, "kernel_button"));
    if (!data->kernel_button) {
        LOG_WARNING("No se pudo obtener el botón del kernel");
    }

    // Obtener el AdwActionRow del kernel
    data->kernel_row = ADW_ACTION_ROW(gtk_builder_get_object(builder, "kernel_row"));
    if (!data->kernel_row) {
        LOG_WARNING("No se pudo obtener kernel_row");
    }

    // Obtener el botón de hardware
    data->driver_hardware_button = GTK_BUTTON(gtk_builder_get_object(builder, "driver_hardware_button"));
    if (!data->driver_hardware_button) {
        LOG_WARNING("No se pudo obtener el botón de hardware");
    }

    // Obtener los switches
    data->essential_apps_switch = ADW_SWITCH_ROW(gtk_builder_get_object(builder, "essential_apps_switch"));
    // office_switch no existe en el UI
    data->utilities_switch = ADW_SWITCH_ROW(gtk_builder_get_object(builder, "utilities_switch"));

    // Obtener los botones de los switches
    data->essential_apps_button = GTK_BUTTON(gtk_builder_get_object(builder, "essential_apps_button"));
    // office_button no existe en el UI
    data->utilities_button = GTK_BUTTON(gtk_builder_get_object(builder, "utilities_button"));

    // Obtener el botón de programas extra
    data->program_extra_button = ADW_BUTTON_ROW(gtk_builder_get_object(builder, "program_extra_button"));

    // Obtener el botón de repositorios
    data->repository_button = GTK_BUTTON(gtk_builder_get_object(builder, "repository_button"));
    if (!data->repository_button) LOG_WARNING("No se pudo obtener repository_button");

    data->drivers_row = ADW_ACTION_ROW(gtk_builder_get_object(builder, "drivers_row"));
    data->repos_row = ADW_ACTION_ROW(gtk_builder_get_object(builder, "repos_row"));
    data->status_page = ADW_STATUS_PAGE(gtk_builder_get_object(builder, "page6_status"));

    // Verificar que se obtuvieron correctamente
    if (!data->essential_apps_switch) LOG_WARNING("No se pudo obtener essential_apps_switch");

    if (!data->utilities_switch) LOG_WARNING("No se pudo obtener utilities_switch");
    if (!data->essential_apps_button) LOG_WARNING("No se pudo obtener essential_apps_button");

    if (!data->utilities_button) LOG_WARNING("No se pudo obtener utilities_button");
    if (!data->driver_hardware_button) LOG_WARNING("No se pudo obtener driver_hardware_button");
    if (!data->program_extra_button) LOG_WARNING("No se pudo obtener program_extra_button");

    // Inicializar estados por defecto
    data->essential_apps_enabled = TRUE;
    data->office_enabled = FALSE; // Office switch no existe en UI, mantener desactivado
    data->utilities_enabled = FALSE;

    LOG_INFO("Widgets de página 6 obtenidos");
}

// Función de limpieza
void page6_cleanup(Page6Data *data)
{
    if (g_page6_data) {
        g_free(g_page6_data);
        g_page6_data = NULL;
        LOG_INFO("Página 6 limpiada correctamente");
    }
}

// Función para configurar widgets
void page6_setup_widgets(Page6Data *data)
{
    if (!data) return;

    // Configurar switches con valores por defecto
    page6_setup_switches(data);

    LOG_INFO("Widgets de la página 6 configurados");
}

// Función para cargar datos
void page6_load_data(Page6Data *data)
{
    if (!data) return;

    // Cargar configuraciones guardadas si existen
    page6_load_system_config(data);

    LOG_INFO("Datos de la página 6 cargados");
}

// Función para conectar señales
void page6_connect_signals(Page6Data *data)
{
    if (!data) return;

    // Conectar señal del botón del kernel
    if (data->kernel_button) {
        g_signal_connect(data->kernel_button, "clicked", G_CALLBACK(on_kernel_button_clicked), data);
        LOG_INFO("Señal del botón kernel conectada");
    }

    // Conectar señal del botón de hardware
    if (data->driver_hardware_button) {
        g_signal_connect(data->driver_hardware_button, "clicked", G_CALLBACK(on_driver_hardware_button_clicked), data);
        LOG_INFO("Señal del botón de hardware conectada");
    }

    // Conectar señal del botón de aplicaciones esenciales
    if (data->essential_apps_button) {
        g_signal_connect(data->essential_apps_button, "clicked", G_CALLBACK(on_essential_apps_button_clicked), data);
        LOG_INFO("Señal del botón de aplicaciones esenciales conectada");
    }

    // Conectar señal del botón de utilities
    if (data->utilities_button) {
        g_signal_connect(data->utilities_button, "clicked", G_CALLBACK(on_utilities_button_clicked), data);
        LOG_INFO("Señal del botón de utilities conectada");
    }

    // Conectar señal del botón de programas extra
    if (data->program_extra_button) {
        g_signal_connect(data->program_extra_button, "activated", G_CALLBACK(on_program_extra_button_clicked), data);
        LOG_INFO("Señal del botón de programas extra conectada");
    }

    // Conectar señal del botón de repositorios
    if (data->repository_button) {
        g_signal_connect(data->repository_button, "clicked", G_CALLBACK(on_repository_button_clicked), data);
        LOG_INFO("Señal del botón de repositorios conectada");
    }

    // Conectar señales de los switches
    if (data->essential_apps_switch) {
        g_signal_connect(data->essential_apps_switch, "notify::active", G_CALLBACK(on_essential_apps_switch_toggled), data);
        LOG_INFO("Señal de essential_apps_switch conectada");
    }

    // office_switch no existe en el UI

    if (data->utilities_switch) {
        g_signal_connect(data->utilities_switch, "notify::active", G_CALLBACK(on_utilities_switch_toggled), data);
        LOG_INFO("Señal de utilities_switch conectada");
    }

    LOG_INFO("Señales de página 6 configuradas");
}

// Función para configurar switches
void page6_setup_switches(Page6Data *data)
{
    if (!data) return;

    // Aplicar estados por defecto
    // Aplicaciones esenciales: activado por defecto
    // Office: desactivado por defecto
    // Utilidades útiles: desactivado por defecto

    LOG_INFO("Switches configurados con valores por defecto");
}

// Función para cargar configuración del sistema
void page6_load_system_config(Page6Data *data)
{
    if (!data) return;

    // Aquí se cargarían las configuraciones guardadas
    // Por ahora usamos valores por defecto

    LOG_INFO("Configuración del sistema cargada");
}

// Función para manejar selección de kernel
void page6_on_kernel_selection(Page6Data *data)
{
    if (!data) return;

    LOG_INFO("Selección de kernel activada");
    // Aquí se abriría un diálogo o página para seleccionar el kernel
}

// Función para manejar configuración de drivers
void page6_on_drivers_configuration(Page6Data *data)
{
    if (!data) return;

    LOG_INFO("Configuración de drivers activada");
    // Aquí se abriría un diálogo o página para configurar drivers
}

// Función para manejar cambio en aplicaciones esenciales
void page6_on_essential_apps_toggled(Page6Data *data, gboolean active)
{
    if (!data) return;

    data->essential_apps_enabled = active;
    LOG_INFO("Aplicaciones esenciales %s", active ? "activadas" : "desactivadas");
}

// Función para manejar cambio en Office
// Función eliminada - office_switch no existe en el UI
// void page6_on_office_toggled(Page6Data *data, gboolean active)
// {
//     if (!data) return;
//     data->office_enabled = active;
//     LOG_INFO("Office %s", active ? "activado" : "desactivado");
// }

// Función para manejar cambio en utilidades útiles
void page6_on_utilities_toggled(Page6Data *data, gboolean active)
{
    if (!data) return;

    data->utilities_enabled = active;
    LOG_INFO("Utilidades útiles %s", active ? "activadas" : "desactivadas");
}

// Función para validar configuración
gboolean page6_validate_configuration(Page6Data *data)
{
    if (!data) return FALSE;

    // La configuración mínima requiere aplicaciones esenciales
    if (!data->essential_apps_enabled) {
        LOG_WARNING("Se recomienda mantener las aplicaciones esenciales activadas");
    }

    return TRUE;
}

// Función para guardar configuración
void page6_save_configuration(Page6Data *data)
{
    if (!data) return;

    LOG_INFO("Guardando configuración del sistema:");
    LOG_INFO("  - Aplicaciones esenciales: %s", data->essential_apps_enabled ? "Sí" : "No");
    LOG_INFO("  - Office: %s", data->office_enabled ? "Sí" : "No");
    LOG_INFO("  - Utilidades útiles: %s", data->utilities_enabled ? "Sí" : "No");

    // Aquí se guardaría la configuración en archivos o variables globales
}

// Función para ir a la página anterior
gboolean page6_go_to_previous_page(Page6Data *data)
{
    if (!data || !data->carousel) return FALSE;

    // Ir a la página anterior (página 5)
    GtkWidget *page5 = adw_carousel_get_nth_page(data->carousel, 4);
    if (page5) {
        adw_carousel_scroll_to(data->carousel, page5, TRUE);
        LOG_INFO("Navegación a página anterior exitosa desde página 6");
        return TRUE;
    }

    return FALSE;
}

// Función para ir a la página siguiente
gboolean page6_go_to_next_page(Page6Data *data)
{
    if (!data || !data->carousel) return FALSE;

    // Validar configuración antes de continuar
    if (!page6_validate_configuration(data)) {
        return FALSE;
    }

    // Guardar configuración
    page6_save_configuration(data);

    // Ir a la página siguiente (página 7)
    GtkWidget *page7 = adw_carousel_get_nth_page(data->carousel, 6);
    if (page7) {
        adw_carousel_scroll_to(data->carousel, page7, TRUE);
        LOG_INFO("Navegación a página siguiente exitosa desde página 6");
        return TRUE;
    }

    return FALSE;
}

// Función para verificar si es la página final
gboolean page6_is_final_page(void)
{
    return FALSE; // Page6 ya no es la página final, ahora es page7
}

// Callback de navegación hacia atrás
void on_page6_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;

    if (page6_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 6");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 6");
    }
}

// Callback de navegación hacia adelante
void on_page6_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;

    if (page6_go_to_next_page(data)) {
        LOG_INFO("Navegación hacia adelante exitosa desde página 6");
    } else {
        LOG_WARNING("No se pudo navegar hacia adelante desde página 6");
    }
}

// Función llamada cuando se muestra la página 6
void page6_on_page_shown(void)
{
    LOG_INFO("Página 6 mostrada - Configuración del sistema");
}

// Función para obtener el widget principal
GtkWidget* page6_get_widget(void)
{
    if (!g_page6_data) return NULL;
    return g_page6_data->main_content;
}

// Función para obtener datos de configuración
Page6Data* page6_get_data(void)
{
    return g_page6_data;
}

// Función para obtener estado de aplicaciones esenciales
gboolean page6_get_essential_apps_enabled(void)
{
    if (!g_page6_data) return TRUE; // Valor por defecto
    return g_page6_data->essential_apps_enabled;
}

// Función para obtener estado de Office
gboolean page6_get_office_enabled(void)
{
    if (!g_page6_data) return FALSE; // Valor por defecto
    return g_page6_data->office_enabled;
}

// Función para obtener estado de utilidades útiles
gboolean page6_get_utilities_enabled(void)
{
    if (!g_page6_data) return FALSE; // Valor por defecto
    return g_page6_data->utilities_enabled;
}

// Función para mostrar el kernel actualmente seleccionado
void page6_display_current_kernel(void)
{
    GError *error = NULL;
    gchar *config_content = NULL;
    const gchar *config_path = "data/bash/variables.sh";
    gchar *current_kernel = g_strdup("linux"); // Por defecto como copia
    gboolean found = FALSE;

    // Leer el archivo variables.sh
    if (g_file_get_contents(config_path, &config_content, NULL, &error)) {
        gchar **lines = g_strsplit(config_content, "\n", -1);

        for (int i = 0; lines[i] != NULL; i++) {
            if (g_str_has_prefix(lines[i], "SELECTED_KERNEL=")) {
                char *value = lines[i] + 16; // Saltar "SELECTED_KERNEL="

                // Remover salto de línea si existe
                lines[i][strcspn(lines[i], "\n")] = 0;
                value = lines[i] + 16;

                // Remover comillas si existen
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }

                // Validar que el kernel no esté vacío
                if (strlen(value) > 0) {
                    g_free(current_kernel);
                    current_kernel = g_strdup(value);
                    found = TRUE;
                }
                break;
            }
        }

        g_strfreev(lines);
        g_free(config_content);

        if (found) {
            LOG_INFO("🐧 Kernel leído desde variables.sh: %s", current_kernel);
        } else {
            LOG_INFO("🐧 SELECTED_KERNEL no encontrado, usando por defecto: %s", current_kernel);
        }
    } else {
        LOG_WARNING("No se pudo leer variables.sh: %s", error ? error->message : "Unknown error");
        LOG_INFO("🐧 Usando kernel por defecto: %s", current_kernel);
        if (error) g_error_free(error);
    }

    // Actualizar el subtítulo de la página para mostrar el kernel actual
    page6_update_kernel_subtitle(current_kernel);

    // Liberar memoria
    g_free(current_kernel);
}

// Función para actualizar el subtítulo del kernel en la UI
void page6_update_kernel_subtitle(const char* kernel_name)
{
    if (!g_page6_data || !kernel_name || !g_page6_data->kernel_row) return;

    // Crear el nuevo subtítulo con el kernel seleccionado
    gchar *new_subtitle = g_strdup_printf("%s %s",
        i18n_t("Kernel seleccionado:", "Selected kernel:", "Выбранное ядро:"), kernel_name);

    // Actualizar el subtítulo del AdwActionRow del kernel
    adw_action_row_set_subtitle(g_page6_data->kernel_row, new_subtitle);

    LOG_INFO("Subtítulo del kernel actualizado en UI: %s", kernel_name);

    // Liberar memoria
    g_free(new_subtitle);
}

// Callback para el botón del kernel
void on_kernel_button_clicked(GtkButton *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    if (!data) return;

    LOG_INFO("Botón del kernel presionado - abriendo ventana de selección de kernels");

    // Aquí se abrirá la ventana window_kernel.ui
    page6_open_kernel_selection_window(data);
}

// Función para abrir la ventana de selección de kernels
void page6_open_kernel_selection_window(Page6Data *data)
{
    if (!data) return;

    // Obtener o crear la instancia de WindowKernel
    WindowKernelData *kernel_window = window_kernel_get_instance();

    if (!kernel_window) {
        // Crear nueva instancia si no existe
        kernel_window = window_kernel_new();
        window_kernel_init(kernel_window);
    }

    // Obtener la ventana principal para establecer como padre
    GtkWindow *parent_window = NULL;
    if (data->carousel) {
        GtkWidget *parent = gtk_widget_get_ancestor(GTK_WIDGET(data->carousel), GTK_TYPE_WINDOW);
        if (parent) {
            parent_window = GTK_WINDOW(parent);
        }
    }

    // Conectar señal para actualizar UI cuando se presione el botón guardar
    if (kernel_window->save_button) {
        g_signal_connect(kernel_window->save_button, "clicked",
                         G_CALLBACK(on_kernel_save_clicked), data);
    }

    // Mostrar la ventana de kernels
    window_kernel_show(kernel_window, parent_window);

    LOG_INFO("Ventana de selección de kernels abierta usando window_kernel module");
}

// Callbacks para los switches
void on_essential_apps_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    if (!data || !data->essential_apps_switch || !data->essential_apps_button) return;

    gboolean active = adw_switch_row_get_active(data->essential_apps_switch);
    gtk_widget_set_sensitive(GTK_WIDGET(data->essential_apps_button), active);

    data->essential_apps_enabled = active;
    LOG_INFO("Aplicaciones esenciales %s - botón %s",
             active ? "activadas" : "desactivadas",
             active ? "habilitado" : "deshabilitado");

    // Guardar automáticamente en variables.sh
    save_page6_switches_to_file();
}

// Callback para cuando se presiona el botón guardar en la ventana de kernels
void on_kernel_save_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; // Suppress unused parameter warning
    Page6Data *data = (Page6Data *)user_data;

    // Usar un timeout pequeño para asegurar que el archivo se haya guardado
    g_timeout_add(100, (GSourceFunc)page6_update_kernel_ui_delayed, data);

    LOG_INFO("Botón guardar kernel presionado - UI será actualizada");
}

// Función para actualizar la UI con delay (usada con g_timeout_add)
gboolean page6_update_kernel_ui_delayed(gpointer user_data)
{
    (void)user_data; // Suppress unused parameter warning

    // Actualizar el subtítulo del kernel leyendo desde variables.sh
    page6_display_current_kernel();

    LOG_INFO("UI del kernel actualizada después del guardado");

    // Retornar FALSE para que el timeout se ejecute solo una vez
    return FALSE;
}



// Función eliminada - office_switch no existe en el UI

void on_utilities_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    if (!data || !data->utilities_switch || !data->utilities_button) return;

    gboolean active = adw_switch_row_get_active(data->utilities_switch);
    gtk_widget_set_sensitive(GTK_WIDGET(data->utilities_button), active);

    data->utilities_enabled = active;
    LOG_INFO("Utilities %s - botón %s",
             active ? "activado" : "desactivado",
             active ? "habilitado" : "deshabilitado");

    // Guardar automáticamente en variables.sh
    save_page6_switches_to_file();
}

// Funciones para manejo de hardware
void on_driver_hardware_button_clicked(GtkButton *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    if (!data) {
        LOG_ERROR("Page6Data es NULL en on_driver_hardware_button_clicked");
        return;
    }

    LOG_INFO("Botón de hardware clickeado - abriendo ventana de hardware");
    page6_open_hardware_window(data);
}

void page6_open_hardware_window(Page6Data *data)
{
    if (!data) {
        LOG_ERROR("Page6Data es NULL en page6_open_hardware_window");
        return;
    }

    // Obtener la instancia de la ventana de hardware
    WindowHardwareData *hardware_data = window_hardware_get_instance();
    if (!hardware_data) {
        LOG_ERROR("No se pudo obtener la instancia de la ventana de hardware");
        return;
    }

    // Obtener la ventana principal como padre
    GtkWindow *parent_window = NULL;
    if (data->main_content) {
        GtkRoot *root = gtk_widget_get_root(data->main_content);
        if (GTK_IS_WINDOW(root)) {
            parent_window = GTK_WINDOW(root);
        }
    }

    // Mostrar la ventana de hardware
    window_hardware_show(hardware_data, parent_window);
    LOG_INFO("Ventana de configuración de hardware abierta");
}

void on_essential_apps_button_clicked(GtkButton *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    if (!data) {
        LOG_ERROR("Page6Data es NULL en on_essential_apps_button_clicked");
        return;
    }

    page6_open_system_window(data);
    LOG_INFO("Botón de aplicaciones esenciales presionado");
}

void page6_open_system_window(Page6Data *data)
{
    if (!data) {
        LOG_ERROR("Page6Data es NULL en page6_open_system_window");
        return;
    }

    // Obtener la instancia de la ventana del sistema
    WindowSystemData *system_data = window_system_get_instance();
    if (!system_data) {
        LOG_ERROR("No se pudo obtener la instancia de la ventana del sistema");
        return;
    }

    // Obtener la ventana principal como padre
    GtkWindow *parent_window = NULL;
    if (data->main_content) {
        GtkRoot *root = gtk_widget_get_root(data->main_content);
        if (GTK_IS_WINDOW(root)) {
            parent_window = GTK_WINDOW(root);
        }
    }

    // Mostrar la ventana del sistema
    window_system_show(system_data, parent_window);
    LOG_INFO("Ventana de configuración del sistema abierta");
}

void load_page6_switches_from_file(void)
{
    if (!g_page6_data) {
        LOG_ERROR("g_page6_data es NULL en load_page6_switches_from_file");
        return;
    }

    LOG_INFO("=== load_page6_switches_from_file INICIADO ===");

    gchar *bash_file_path = g_build_filename(".", "data", "bash", "variables.sh", NULL);

    // Variables por defecto
    gboolean essential_apps_enabled = TRUE;  // Aplicaciones esenciales activadas por defecto
    gboolean utilities_enabled = FALSE;

    FILE *read_file = fopen(bash_file_path, "r");
    if (read_file) {
        char line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Leer ESSENTIAL_APPS_ENABLED
            if (g_str_has_prefix(line, "ESSENTIAL_APPS_ENABLED=")) {
                char *value = line + 23;
                line[strcspn(line, "\n")] = 0;
                value = line + 23;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                essential_apps_enabled = (g_strcmp0(value, "true") == 0);
                LOG_INFO("ESSENTIAL_APPS_ENABLED cargado: %s", essential_apps_enabled ? "true" : "false");
            }
            // Leer UTILITIES_ENABLED
            else if (g_str_has_prefix(line, "UTILITIES_ENABLED=")) {
                char *value = line + 18;
                line[strcspn(line, "\n")] = 0;
                value = line + 18;
                if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                    value[strlen(value)-1] = 0;
                    value++;
                }
                utilities_enabled = (g_strcmp0(value, "true") == 0);
                LOG_INFO("UTILITIES_ENABLED cargado: %s", utilities_enabled ? "true" : "false");
            }
        }
        fclose(read_file);
    } else {
        LOG_INFO("Archivo variables.sh no existe, usando valores por defecto para switches");
    }

    // Actualizar datos internos
    g_page6_data->essential_apps_enabled = essential_apps_enabled;
    g_page6_data->utilities_enabled = utilities_enabled;

    // Actualizar widgets UI si están disponibles
    if (g_page6_data->essential_apps_switch) {
        adw_switch_row_set_active(g_page6_data->essential_apps_switch, essential_apps_enabled);
        // Habilitar/deshabilitar botón según el estado del switch
        if (g_page6_data->essential_apps_button) {
            gtk_widget_set_sensitive(GTK_WIDGET(g_page6_data->essential_apps_button), essential_apps_enabled);
        }
    }

    if (g_page6_data->utilities_switch) {
        adw_switch_row_set_active(g_page6_data->utilities_switch, utilities_enabled);
        // Habilitar/deshabilitar botón según el estado del switch
        if (g_page6_data->utilities_button) {
            gtk_widget_set_sensitive(GTK_WIDGET(g_page6_data->utilities_button), utilities_enabled);
        }
    }

    g_free(bash_file_path);
    LOG_INFO("=== load_page6_switches_from_file FINALIZADO ===");
}

static void apply_page6_vars(GString *content, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;

    if (data->essential_apps_switch) {
        gboolean active = adw_switch_row_get_active(data->essential_apps_switch);
        vars_upsert(content, "ESSENTIAL_APPS_ENABLED", active ? "true" : "false");
    }

    if (data->utilities_switch) {
        gboolean active = adw_switch_row_get_active(data->utilities_switch);
        vars_upsert(content, "UTILITIES_ENABLED", active ? "true" : "false");
    }
}

void save_page6_switches_to_file(void)
{
    if (!g_page6_data) {
        LOG_ERROR("g_page6_data es NULL en save_page6_switches_to_file");
        return;
    }
    LOG_INFO("=== save_page6_switches_to_file INICIADO ===");
    if (!vars_update(apply_page6_vars, g_page6_data))
        LOG_WARNING("No se pudo guardar switches de página 6");
    else
        LOG_INFO("=== save_page6_switches_to_file FINALIZADO ===");
}

// Implementaciones para el botón de programas extra

void on_program_extra_button_clicked(AdwButtonRow *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    if (!data) {
        LOG_ERROR("Page6Data es NULL en on_program_extra_button_clicked");
        return;
    }

    LOG_INFO("Botón de programas extra clickeado - abriendo ventana de programas extra");
    page6_open_program_extra_window(data);
}

void page6_open_program_extra_window(Page6Data *data)
{
    if (!data) {
        LOG_ERROR("Page6Data es NULL en page6_open_program_extra_window");
        return;
    }

    LOG_INFO("Abriendo ventana de programas extra");

    // Obtener la instancia de la ventana de programas extra
    WindowProgramExtraData *program_extra_data = window_program_extra_get_instance();
    if (!program_extra_data) {
        LOG_ERROR("No se pudo obtener la instancia de la ventana de programas extra");
        return;
    }

    // Inicializar si es necesario
    if (!program_extra_data->is_initialized) {
        window_program_extra_init(program_extra_data);
    }

    // Obtener ventana padre (buscar ventana principal)
    GtkWindow *parent_window = NULL;
    if (data->main_content) {
        GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(data->main_content));
        if (GTK_IS_WINDOW(toplevel)) {
            parent_window = GTK_WINDOW(toplevel);
        }
    }

    // Mostrar la ventana
    window_program_extra_show(program_extra_data, parent_window);

    LOG_INFO("Ventana de programas extra mostrada exitosamente");
}

void on_utilities_button_clicked(GtkButton *button, gpointer user_data)
{
    Page6Data *data = (Page6Data *)user_data;
    if (!data) {
        LOG_ERROR("Page6Data es NULL en on_utilities_button_clicked");
        return;
    }

    page6_open_utilities_window(data);
    LOG_INFO("Botón de utilities presionado");
}

void page6_open_utilities_window(Page6Data *data)
{
    if (!data) {
        LOG_ERROR("Page6Data es NULL en page6_open_utilities_window");
        return;
    }

    LOG_INFO("Abriendo ventana de utilities apps");

    // Obtener la instancia de la ventana de utilities apps
    WindowAppsData *apps_data = window_apps_get_instance();
    if (!apps_data) {
        LOG_ERROR("No se pudo obtener la instancia de la ventana de utilities apps");
        return;
    }

    // Inicializar si es necesario
    if (!apps_data->is_initialized) {
        window_apps_init(apps_data);
    }

    // Obtener ventana padre (buscar ventana principal)
    GtkWindow *parent_window = NULL;
    if (data->main_content) {
        GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(data->main_content));
        if (GTK_IS_WINDOW(toplevel)) {
            parent_window = GTK_WINDOW(toplevel);
        }
    }

    // Mostrar la ventana
    window_apps_show(apps_data, parent_window);

    LOG_INFO("Ventana de utilities apps mostrada exitosamente");
}

void on_repository_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    Page6Data *data = (Page6Data *)user_data;
    if (!data) return;

    page6_open_repos_window(data);
}

void page6_open_repos_window(Page6Data *data)
{
    if (!data) return;

    WindowReposData *repos_data = window_repos_get_instance();
    if (!repos_data) {
        repos_data = window_repos_new();
    }

    GtkWindow *parent_window = NULL;
    if (data->main_content) {
        GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(data->main_content));
        if (GTK_IS_WINDOW(toplevel)) {
            parent_window = GTK_WINDOW(toplevel);
        }
    }

    window_repos_show(repos_data, parent_window);
    LOG_INFO("Ventana de repositorios abierta");
}

void page6_update_language(void)
{
    if (!g_page6_data) return;

    if (g_page6_data->status_page) {
        adw_status_page_set_title(g_page6_data->status_page,
            i18n_t("Sistema", "System", "Система"));
        adw_status_page_set_description(g_page6_data->status_page,
            i18n_t("Elige que Kernel, Drivers y Aplicaciones deseas instalar.",
                   "Choose the Kernel, Drivers and Applications to install.",
                   "Выберите ядро, драйверы и приложения для установки."));
    }
    if (g_page6_data->kernel_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page6_data->kernel_row),
            i18n_t("Kernel", "Kernel", "Ядро"));
        page6_display_current_kernel();
    }
    if (g_page6_data->drivers_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page6_data->drivers_row),
            i18n_t("Drivers", "Drivers", "Драйверы"));
        adw_action_row_set_subtitle(g_page6_data->drivers_row,
            i18n_t("Módulos de video, sonido, wifi, Bluetooth",
                   "Video, audio, wifi, Bluetooth modules",
                   "Модули видео, аудио, wifi, Bluetooth"));
    }
    if (g_page6_data->repos_row) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page6_data->repos_row),
            i18n_t("Repositorios", "Repositories", "Репозитории"));
        adw_action_row_set_subtitle(g_page6_data->repos_row,
            i18n_t("Official y Unofficial Repositorios de usuarios",
                   "Official and unofficial user repositories",
                   "Официальные и неофициальные репозитории"));
    }
    if (g_page6_data->essential_apps_switch) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page6_data->essential_apps_switch),
            i18n_t("Aplicaciones base", "Base Applications", "Базовые приложения"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(g_page6_data->essential_apps_switch),
            i18n_t("Sistemas de archivos, shell, códecs, etc",
                   "Filesystems, shell, codecs, etc",
                   "Файловые системы, shell, кодеки и т.д."));
    }
    if (g_page6_data->utilities_switch) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page6_data->utilities_switch),
            i18n_t("Utilidades", "Utilities", "Утилиты"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(g_page6_data->utilities_switch),
            i18n_t("Como navegadores, IDEs, Office, Vlc, Gamming, etc",
                   "Browsers, IDEs, Office, VLC, Gaming, etc",
                   "Браузеры, IDE, офис, VLC, игры и т.д."));
    }
    if (g_page6_data->program_extra_button)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page6_data->program_extra_button),
            i18n_t("Agrega Programas Extras", "Add Extra Programs", "Добавить дополнительные программы"));

    WindowKernelData *kernel_window = window_kernel_get_instance();
    if (kernel_window)
        window_kernel_update_language(kernel_window);

    WindowHardwareData *hardware_window = window_hardware_get_instance();
    if (hardware_window)
        window_hardware_update_language(hardware_window);

    WindowReposData *repos_window = window_repos_get_instance();
    if (repos_window)
        window_repos_update_language(repos_window);

    WindowSystemData *system_window = window_system_get_instance();
    if (system_window)
        window_system_update_language(system_window);

    WindowProgramExtraData *extra_window = window_program_extra_get_instance();
    if (extra_window && extra_window->is_initialized)
        window_program_extra_update_language(extra_window);

    WindowAppsData *apps_window = window_apps_get_instance();
    if (apps_window && apps_window->is_initialized)
        window_apps_update_language(apps_window);
}
