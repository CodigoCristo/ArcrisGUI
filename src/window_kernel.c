#include "window_kernel.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

// Variable global para datos de la ventana de kernel
static WindowKernelData *g_window_kernel_data = NULL;

// Nombres de los kernels para variables.sh
static const char* KERNEL_NAMES[] = {
    "linux",
    "linux-hardened", 
    "linux-lts",
    "linux-rt-lts",
    "linux-zen"
};

// Función para crear nueva instancia de WindowKernelData
WindowKernelData* window_kernel_new(void)
{
    WindowKernelData *data = g_malloc0(sizeof(WindowKernelData));
    
    data->window = NULL;
    data->builder = NULL;
    data->current_kernel = KERNEL_LINUX; // Por defecto
    data->is_initialized = FALSE;
    
    // Inicializar punteros de widgets
    data->close_button = NULL;
    data->save_button = NULL;
    data->kernel_linux_radio = NULL;
    data->hardened_radio = NULL;
    data->lts_radio = NULL;
    data->rt_lts_radio = NULL;
    data->zen_radio = NULL;
    
    LOG_INFO("WindowKernelData creada");
    return data;
}

// Función para inicializar la ventana
void window_kernel_init(WindowKernelData *data)
{
    if (!data) return;
    
    // Cargar el builder desde recursos
    data->builder = gtk_builder_new_from_resource("/org/gtk/arcris/window_kernel.ui");
    if (!data->builder) {
        LOG_ERROR("No se pudo cargar el builder de window_kernel.ui");
        return;
    }
    
    // Obtener la ventana principal
    data->window = GTK_WINDOW(gtk_builder_get_object(data->builder, "KernelListWindow"));
    if (!data->window) {
        LOG_ERROR("No se pudo obtener la ventana KernelListWindow");
        return;
    }
    
    // Cargar widgets desde el builder
    window_kernel_load_widgets_from_builder(data);
    
    // Configurar widgets
    window_kernel_setup_widgets(data);
    
    // Conectar señales
    window_kernel_connect_signals(data);
    
    // Cargar selección actual desde variables.sh
    window_kernel_load_from_variables(data);
    
    data->is_initialized = TRUE;
    g_window_kernel_data = data;
    
    LOG_INFO("WindowKernel inicializada correctamente");
}

// Función para obtener widgets desde el builder
void window_kernel_load_widgets_from_builder(WindowKernelData *data)
{
    if (!data || !data->builder) return;
    
    // Obtener botones
    data->close_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));
    
    // Obtener radio buttons
    data->kernel_linux_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "kernel_linux_radio"));
    data->hardened_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "hardened_radio"));
    data->lts_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "lts_radio"));
    data->rt_lts_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "rt_lts_radio"));
    data->zen_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, "zen_radio"));
    
    // Verificar que se obtuvieron correctamente
    if (!data->close_button) LOG_WARNING("No se pudo obtener close_button");
    if (!data->save_button) LOG_WARNING("No se pudo obtener save_button");
    if (!data->kernel_linux_radio) LOG_WARNING("No se pudo obtener kernel_linux_radio");
    if (!data->hardened_radio) LOG_WARNING("No se pudo obtener hardened_radio");
    if (!data->lts_radio) LOG_WARNING("No se pudo obtener lts_radio");
    if (!data->rt_lts_radio) LOG_WARNING("No se pudo obtener rt_lts_radio");
    if (!data->zen_radio) LOG_WARNING("No se pudo obtener zen_radio");
    
    LOG_INFO("Widgets de WindowKernel cargados desde builder");
}

// Función para configurar widgets
void window_kernel_setup_widgets(WindowKernelData *data)
{
    if (!data) return;
    
    // Configurar ventana
    if (data->window) {
        gtk_window_set_modal(data->window, TRUE);
        gtk_window_set_resizable(data->window, FALSE);
    }
    
    // Configurar selección por defecto (linux)
    window_kernel_set_selected_kernel(data, KERNEL_LINUX);
    
    LOG_INFO("Widgets de WindowKernel configurados");
}

// Función para conectar señales
void window_kernel_connect_signals(WindowKernelData *data)
{
    if (!data) return;
    
    // Conectar señales de botones
    if (data->close_button) {
        g_signal_connect(data->close_button, "clicked", G_CALLBACK(on_kernel_close_button_clicked), data);
    }
    
    if (data->save_button) {
        g_signal_connect(data->save_button, "clicked", G_CALLBACK(on_kernel_save_button_clicked), data);
    }
    
    // Conectar señales de radio buttons
    if (data->kernel_linux_radio) {
        g_signal_connect(data->kernel_linux_radio, "toggled", G_CALLBACK(on_kernel_linux_radio_toggled), data);
    }
    
    if (data->hardened_radio) {
        g_signal_connect(data->hardened_radio, "toggled", G_CALLBACK(on_kernel_hardened_radio_toggled), data);
    }
    
    if (data->lts_radio) {
        g_signal_connect(data->lts_radio, "toggled", G_CALLBACK(on_kernel_lts_radio_toggled), data);
    }
    
    if (data->rt_lts_radio) {
        g_signal_connect(data->rt_lts_radio, "toggled", G_CALLBACK(on_kernel_rt_lts_radio_toggled), data);
    }
    
    if (data->zen_radio) {
        g_signal_connect(data->zen_radio, "toggled", G_CALLBACK(on_kernel_zen_radio_toggled), data);
    }
    
    LOG_INFO("Señales de WindowKernel conectadas");
}

// Función para mostrar la ventana
void window_kernel_show(WindowKernelData *data, GtkWindow *parent)
{
    if (!data || !data->window) return;
    
    // Configurar ventana padre si se proporciona
    if (parent) {
        gtk_window_set_transient_for(data->window, parent);
    }
    
    // Mostrar la ventana
    gtk_window_present(data->window);
    
    LOG_INFO("WindowKernel mostrada");
}

// Función para ocultar la ventana
void window_kernel_hide(WindowKernelData *data)
{
    if (!data || !data->window) return;
    
    gtk_window_close(data->window);
    
    LOG_INFO("WindowKernel ocultada");
}

// Función para obtener el kernel seleccionado
KernelType window_kernel_get_selected_kernel(WindowKernelData *data)
{
    if (!data) return KERNEL_LINUX;
    
    // Verificar cuál radio button está activo
    if (data->kernel_linux_radio && gtk_check_button_get_active(data->kernel_linux_radio)) {
        return KERNEL_LINUX;
    } else if (data->hardened_radio && gtk_check_button_get_active(data->hardened_radio)) {
        return KERNEL_HARDENED;
    } else if (data->lts_radio && gtk_check_button_get_active(data->lts_radio)) {
        return KERNEL_LTS;
    } else if (data->rt_lts_radio && gtk_check_button_get_active(data->rt_lts_radio)) {
        return KERNEL_RT_LTS;
    } else if (data->zen_radio && gtk_check_button_get_active(data->zen_radio)) {
        return KERNEL_ZEN;
    }
    
    return data->current_kernel;
}

// Función para establecer el kernel seleccionado
void window_kernel_set_selected_kernel(WindowKernelData *data, KernelType kernel)
{
    if (!data || !window_kernel_is_valid_kernel_type(kernel)) return;
    
    KernelType old_kernel = data->current_kernel;
    data->current_kernel = kernel;
    
    // Actualizar la UI
    window_kernel_update_ui_selection(data);
    
    // Log del cambio
    window_kernel_log_selection_change(old_kernel, kernel);
}

// Función para actualizar la selección en la UI
void window_kernel_update_ui_selection(WindowKernelData *data)
{
    if (!data) return;
    
    // Desactivar todas las señales temporalmente para evitar loops
    if (data->kernel_linux_radio) {
        g_signal_handlers_block_by_func(data->kernel_linux_radio, on_kernel_linux_radio_toggled, data);
    }
    if (data->hardened_radio) {
        g_signal_handlers_block_by_func(data->hardened_radio, on_kernel_hardened_radio_toggled, data);
    }
    if (data->lts_radio) {
        g_signal_handlers_block_by_func(data->lts_radio, on_kernel_lts_radio_toggled, data);
    }
    if (data->rt_lts_radio) {
        g_signal_handlers_block_by_func(data->rt_lts_radio, on_kernel_rt_lts_radio_toggled, data);
    }
    if (data->zen_radio) {
        g_signal_handlers_block_by_func(data->zen_radio, on_kernel_zen_radio_toggled, data);
    }
    
    // Actualizar los radio buttons según la selección actual
    switch (data->current_kernel) {
        case KERNEL_LINUX:
            if (data->kernel_linux_radio) gtk_check_button_set_active(data->kernel_linux_radio, TRUE);
            break;
        case KERNEL_HARDENED:
            if (data->hardened_radio) gtk_check_button_set_active(data->hardened_radio, TRUE);
            break;
        case KERNEL_LTS:
            if (data->lts_radio) gtk_check_button_set_active(data->lts_radio, TRUE);
            break;
        case KERNEL_RT_LTS:
            if (data->rt_lts_radio) gtk_check_button_set_active(data->rt_lts_radio, TRUE);
            break;
        case KERNEL_ZEN:
            if (data->zen_radio) gtk_check_button_set_active(data->zen_radio, TRUE);
            break;
    }
    
    // Reactivar señales
    if (data->kernel_linux_radio) {
        g_signal_handlers_unblock_by_func(data->kernel_linux_radio, on_kernel_linux_radio_toggled, data);
    }
    if (data->hardened_radio) {
        g_signal_handlers_unblock_by_func(data->hardened_radio, on_kernel_hardened_radio_toggled, data);
    }
    if (data->lts_radio) {
        g_signal_handlers_unblock_by_func(data->lts_radio, on_kernel_lts_radio_toggled, data);
    }
    if (data->rt_lts_radio) {
        g_signal_handlers_unblock_by_func(data->rt_lts_radio, on_kernel_rt_lts_radio_toggled, data);
    }
    if (data->zen_radio) {
        g_signal_handlers_unblock_by_func(data->zen_radio, on_kernel_zen_radio_toggled, data);
    }
}

// Función para obtener el nombre del kernel
const char* window_kernel_get_kernel_name(KernelType kernel)
{
    if (window_kernel_is_valid_kernel_type(kernel)) {
        return KERNEL_NAMES[kernel];
    }
    return KERNEL_NAMES[KERNEL_LINUX]; // Fallback
}

// Función para obtener el tipo de kernel desde el nombre
KernelType window_kernel_get_kernel_from_name(const char* name)
{
    if (!name) return KERNEL_LINUX;
    
    for (int i = 0; i < sizeof(KERNEL_NAMES) / sizeof(KERNEL_NAMES[0]); i++) {
        if (g_strcmp0(name, KERNEL_NAMES[i]) == 0) {
            return (KernelType)i;
        }
    }
    
    return KERNEL_LINUX; // Fallback
}

// Función para validar tipo de kernel
gboolean window_kernel_is_valid_kernel_type(KernelType kernel)
{
    return (kernel >= KERNEL_LINUX && kernel <= KERNEL_ZEN);
}

// Función para cargar desde variables.sh
gboolean window_kernel_load_from_variables(WindowKernelData *data)
{
    if (!data) return FALSE;
    
    GError *error = NULL;
    gchar *config_content = NULL;
    const gchar *config_path = "data/variables.sh";
    
    // Leer el archivo variables.sh
    if (!g_file_get_contents(config_path, &config_content, NULL, &error)) {
        LOG_WARNING("No se pudo leer variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }
    
    // Buscar la variable SELECTED_KERNEL
    gchar **lines = g_strsplit(config_content, "\n", -1);
    KernelType loaded_kernel = KERNEL_LINUX; // Por defecto
    gboolean found = FALSE;
    
    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "SELECTED_KERNEL=")) {
            char *value = lines[i] + 16; // Saltar "SELECTED_KERNEL="
            
            // Remover comillas si existen
            if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                value[strlen(value)-1] = 0;
                value++;
            }
            
            loaded_kernel = window_kernel_get_kernel_from_name(value);
            found = TRUE;
            LOG_INFO("SELECTED_KERNEL cargado desde variables.sh: %s", value);
            break;
        }
    }
    
    g_strfreev(lines);
    g_free(config_content);
    
    if (found) {
        window_kernel_set_selected_kernel(data, loaded_kernel);
    } else {
        LOG_INFO("SELECTED_KERNEL no encontrado en variables.sh, usando por defecto: %s", 
                window_kernel_get_kernel_name(KERNEL_LINUX));
    }
    
    return found;
}

// Función para guardar en variables.sh
gboolean window_kernel_save_kernel_variable(KernelType kernel)
{
    GError *error = NULL;
    gchar *config_content = NULL;
    const gchar *config_path = "data/variables.sh";
    const char *kernel_name = window_kernel_get_kernel_name(kernel);
    
    // Leer el archivo actual
    if (!g_file_get_contents(config_path, &config_content, NULL, &error)) {
        LOG_ERROR("Error al leer archivo de configuración: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }
    
    // Buscar si ya existe la variable SELECTED_KERNEL
    gchar **lines = g_strsplit(config_content, "\n", -1);
    GString *new_content = g_string_new("");
    gboolean found = FALSE;
    
    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "SELECTED_KERNEL=")) {
            // Reemplazar la línea existente
            g_string_append_printf(new_content, "SELECTED_KERNEL=\"%s\"\n", kernel_name);
            found = TRUE;
        } else if (g_str_has_prefix(lines[i], "# Kernel seleccionado")) {
            // Saltar el comentario anterior (se añadirá nuevo)
            continue;
        } else {
            g_string_append_printf(new_content, "%s\n", lines[i]);
        }
    }
    
    // Si no se encontró, añadir al final
    if (!found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append_printf(new_content, "\n# Kernel seleccionado\nSELECTED_KERNEL=\"%s\"\n", kernel_name);
    }
    
    // Escribir el archivo actualizado
    gboolean success = g_file_set_contents(config_path, new_content->str, -1, &error);
    if (!success) {
        LOG_ERROR("Error al escribir archivo de configuración: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    } else {
        LOG_INFO("SELECTED_KERNEL guardado en variables.sh: %s", kernel_name);
    }
    
    g_strfreev(lines);
    g_free(config_content);
    g_string_free(new_content, TRUE);
    
    return success;
}

// Función para guardar a variables.sh (wrapper)
gboolean window_kernel_save_to_variables(WindowKernelData *data)
{
    if (!data) return FALSE;
    
    return window_kernel_save_kernel_variable(data->current_kernel);
}

// Callbacks de botones

void on_kernel_close_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowKernelData *data = (WindowKernelData *)user_data;
    
    LOG_INFO("Botón cerrar presionado en WindowKernel");
    window_kernel_hide(data);
}

void on_kernel_save_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowKernelData *data = (WindowKernelData *)user_data;
    
    if (!data) return;
    
    // Obtener la selección actual
    KernelType selected = window_kernel_get_selected_kernel(data);
    data->current_kernel = selected;
    
    // Guardar en variables.sh
    if (window_kernel_save_to_variables(data)) {
        LOG_INFO("Kernel seleccionado guardado exitosamente: %s", window_kernel_get_kernel_name(selected));
    } else {
        LOG_ERROR("Error al guardar kernel seleccionado");
    }
    
    // Cerrar la ventana
    window_kernel_hide(data);
}

// Callbacks de radio buttons

void on_kernel_linux_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    WindowKernelData *data = (WindowKernelData *)user_data;
    
    if (gtk_check_button_get_active(radio)) {
        data->current_kernel = KERNEL_LINUX;
        LOG_INFO("Kernel linux seleccionado");
    }
}

void on_kernel_hardened_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    WindowKernelData *data = (WindowKernelData *)user_data;
    
    if (gtk_check_button_get_active(radio)) {
        data->current_kernel = KERNEL_HARDENED;
        LOG_INFO("Kernel linux-hardened seleccionado");
    }
}

void on_kernel_lts_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    WindowKernelData *data = (WindowKernelData *)user_data;
    
    if (gtk_check_button_get_active(radio)) {
        data->current_kernel = KERNEL_LTS;
        LOG_INFO("Kernel linux-lts seleccionado");
    }
}

void on_kernel_rt_lts_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    WindowKernelData *data = (WindowKernelData *)user_data;
    
    if (gtk_check_button_get_active(radio)) {
        data->current_kernel = KERNEL_RT_LTS;
        LOG_INFO("Kernel linux-rt-lts seleccionado");
    }
}

void on_kernel_zen_radio_toggled(GtkCheckButton *radio, gpointer user_data)
{
    WindowKernelData *data = (WindowKernelData *)user_data;
    
    if (gtk_check_button_get_active(radio)) {
        data->current_kernel = KERNEL_ZEN;
        LOG_INFO("Kernel linux-zen seleccionado");
    }
}

// Funciones de utilidad

void window_kernel_reset_to_default(WindowKernelData *data)
{
    if (!data) return;
    
    window_kernel_set_selected_kernel(data, KERNEL_LINUX);
    LOG_INFO("WindowKernel reseteada a valores por defecto");
}

void window_kernel_log_selection_change(KernelType old_kernel, KernelType new_kernel)
{
    if (old_kernel != new_kernel) {
        LOG_INFO("Cambio de kernel: %s -> %s", 
                window_kernel_get_kernel_name(old_kernel),
                window_kernel_get_kernel_name(new_kernel));
    }
}

// Función de limpieza
void window_kernel_cleanup(WindowKernelData *data)
{
    if (!data) return;
    
    LOG_INFO("Limpiando WindowKernelData...");
    
    if (data->builder) {
        g_object_unref(data->builder);
    }
    
    g_free(data);
    
    if (g_window_kernel_data == data) {
        g_window_kernel_data = NULL;
    }
    
    LOG_INFO("WindowKernelData limpiada correctamente");
}

// Función pública para obtener la instancia global
WindowKernelData* window_kernel_get_instance(void)
{
    return g_window_kernel_data;
}