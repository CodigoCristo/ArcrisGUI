#include "page4.h"
#include "config.h"
#include <stdio.h>

#include <string.h>

#include <glib.h>
#include <gio/gio.h>

// Variable global para datos de la página 4
static Page4Data *g_page4_data = NULL;

// Funciones privadas
static void page4_connect_signals(Page4Data *data);


// Función principal de inicialización de la página 4
void page4_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page4_data = g_malloc0(sizeof(Page4Data));

    // Guardar referencias importantes
    g_page4_data->carousel = carousel;
    g_page4_data->revealer = revealer;

    // Inicializar estados de validación
    g_page4_data->username_valid = FALSE;
    g_page4_data->hostname_valid = FALSE;
    g_page4_data->password_length_valid = FALSE;
    g_page4_data->passwords_match = FALSE;

    // Cargar la página 4 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page4.ui");
    GtkWidget *page4 = GTK_WIDGET(gtk_builder_get_object(page_builder, "main_bin"));

    if (!page4) {
        LOG_ERROR("No se pudo cargar la página 4 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }

    // Obtener widgets específicos de la página
    g_page4_data->main_content = page4;
    g_page4_data->username_entry = ADW_ENTRY_ROW(gtk_builder_get_object(page_builder, "username_entry"));
    g_page4_data->password_entry = ADW_PASSWORD_ENTRY_ROW(gtk_builder_get_object(page_builder, "password_entry"));
    g_page4_data->password_confirm_entry = ADW_PASSWORD_ENTRY_ROW(gtk_builder_get_object(page_builder, "password_confirm_entry"));
    g_page4_data->hostname_entry = ADW_ENTRY_ROW(gtk_builder_get_object(page_builder, "hostname_entry"));
    g_page4_data->password_error_label = GTK_LABEL(gtk_builder_get_object(page_builder, "password_error_label"));

    // Verificar que todos los widgets se obtuvieron correctamente
    if (!g_page4_data->username_entry || !g_page4_data->password_entry ||
        !g_page4_data->password_confirm_entry) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de la página 4");
        g_object_unref(page_builder);
        return;
    }

    // Cargar nombres reservados
    page4_load_reserved_usernames(g_page4_data);



    // Realizar configuraciones iniciales específicas de la página 4
    page4_setup_widgets(g_page4_data);
    page4_load_data(g_page4_data);

    // Conectar señales
    page4_connect_signals(g_page4_data);



    // Crear botones de navegación
    page4_create_navigation_buttons(g_page4_data);

    // Añadir la página al carousel
    adw_carousel_append(carousel, page4);

    // Liberar el builder de la página
    g_object_unref(page_builder);

    // Ejecutar una validación inicial después de un breve delay
    // para asegurar que todos los widgets estén completamente inicializados
    g_timeout_add(100, (GSourceFunc)page4_initial_validation, g_page4_data);

    LOG_INFO("Página 4 (Registro de Usuario) inicializada correctamente");
}

// Función para validación inicial con delay
gboolean page4_initial_validation(Page4Data *data)
{
    if (!data) return FALSE;

    LOG_INFO("Ejecutando validación inicial de campos...");

    // Validar todos los campos inicialmente
    page4_validate_username(data);
    page4_validate_hostname(data);
    page4_validate_password_length(data);
    page4_check_password_match(data);

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);

    LOG_INFO("Validación inicial completada");

    // Retornar FALSE para que el timeout se ejecute solo una vez
    return FALSE;
}

// Función de limpieza
void page4_cleanup(Page4Data *data)
{
    if (g_page4_data) {
        // Liberar lista de nombres reservados
        if (g_page4_data->reserved_usernames) {
            g_strfreev(g_page4_data->reserved_usernames);
        }
        g_free(g_page4_data);
        g_page4_data = NULL;
        LOG_INFO("Página 4 limpiada correctamente");
    }
}

// Función para configurar widgets
void page4_setup_widgets(Page4Data *data)
{
    if (!data) return;

    // Configurar estado inicial de validación
    data->passwords_match = FALSE;

    // Ocultar inicialmente el mensaje de error
    if (data->password_error_label) {
        gtk_widget_set_visible(GTK_WIDGET(data->password_error_label), FALSE);
    }

    // Desactivar el botón siguiente inicialmente
    if (data->revealer) {
        GtkWidget *revealer_child = gtk_revealer_get_child(data->revealer);
        if (revealer_child) {
            GtkWidget *next_button = page4_find_next_button_recursive(revealer_child);
            if (next_button) {
                gtk_widget_set_sensitive(next_button, FALSE);
                LOG_INFO("Botón siguiente desactivado inicialmente");
            }
        }
    }

    // Desactivar apply buttons inicialmente
    if (data->username_entry) {
        adw_entry_row_set_show_apply_button(data->username_entry, FALSE);
    }

    if (data->hostname_entry) {
        adw_entry_row_set_show_apply_button(data->hostname_entry, FALSE);
    }

    LOG_INFO("Widgets de la página 4 configurados");

    // No ejecutamos page4_update_next_button_state aquí porque los widgets
    // aún no están completamente inicializados
}

// Función para cargar datos
void page4_load_data(Page4Data *data)
{
    if (!data) return;

    if (data->hostname_entry) {
        gtk_editable_set_text(GTK_EDITABLE(data->hostname_entry), "arcris");
    }

    // Ejecutar validación después de establecer valores predeterminados
    page4_validate_username(data);
    page4_validate_hostname(data);
    page4_validate_password_length(data);
    page4_check_password_match(data);

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);

    LOG_INFO("Datos de la página 4 cargados");
}

// Función para conectar señales
static void page4_connect_signals(Page4Data *data)
{
    if (!data) return;

    // Conectar señales de entrada de texto
    if (data->password_entry) {
        g_signal_connect(data->password_entry, "changed",
                         G_CALLBACK(on_page4_password_changed), data);
    }

    if (data->password_confirm_entry) {
        g_signal_connect(data->password_confirm_entry, "changed",
                         G_CALLBACK(on_page4_password_confirm_changed), data);
    }

    if (data->username_entry) {
        g_signal_connect(data->username_entry, "changed",
                         G_CALLBACK(on_page4_username_changed), data);
    }

    if (data->hostname_entry) {
        g_signal_connect(data->hostname_entry, "changed",
                         G_CALLBACK(on_page4_hostname_changed), data);
    }

    LOG_INFO("Señales conectadas para página 4");
}



// Función para verificar coincidencia de contraseñas
void page4_check_password_match(Page4Data *data)
{
    if (!data || !data->password_entry || !data->password_confirm_entry) return;

    const gchar *password = gtk_editable_get_text(GTK_EDITABLE(data->password_entry));
    const gchar *confirm_password = gtk_editable_get_text(GTK_EDITABLE(data->password_confirm_entry));

    // Verificar si las contraseñas coinciden (solo si ambas tienen contenido)
    data->passwords_match = (password && confirm_password &&
                            strlen(password) > 0 &&
                            strlen(confirm_password) > 0 &&
                            strcmp(password, confirm_password) == 0);

    // Mostrar/ocultar mensaje de error y agregar clases CSS
    if (data->password_error_label) {
        if (strlen(confirm_password) > 0) {
            if (!data->passwords_match) {
                // Mostrar error cuando no coinciden
                gtk_widget_set_visible(GTK_WIDGET(data->password_error_label), TRUE);
                gtk_widget_add_css_class(GTK_WIDGET(data->password_confirm_entry), "error");
                gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "success");
            } else {
                // Ocultar error cuando coinciden y agregar success
                gtk_widget_set_visible(GTK_WIDGET(data->password_error_label), FALSE);
                gtk_widget_add_css_class(GTK_WIDGET(data->password_confirm_entry), "success");
                gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "error");
            }
        } else {
            // Campo vacío - remover todas las clases y marcar como inválido
            data->passwords_match = FALSE;
            gtk_widget_set_visible(GTK_WIDGET(data->password_error_label), FALSE);
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "error");
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "success");
        }
    }

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);
}



// Función para verificar si la contraseña es válida
gboolean page4_is_password_valid(Page4Data *data)
{
    if (!data) return FALSE;

    return data->passwords_match;
}

// Función para verificar si todo el formulario es válido
gboolean page4_is_form_valid(Page4Data *data)
{
    if (!data) return FALSE;

    const gchar *username = gtk_editable_get_text(GTK_EDITABLE(data->username_entry));
    const gchar *password = gtk_editable_get_text(GTK_EDITABLE(data->password_entry));
    const gchar *hostname = gtk_editable_get_text(GTK_EDITABLE(data->hostname_entry));

    return page4_is_password_valid(data) &&
           data->username_valid &&
           data->hostname_valid &&
           data->password_length_valid &&
           username && strlen(username) > 0 &&
           password && strlen(password) > 0 &&
           hostname && strlen(hostname) > 0;
}

// Callbacks para cambios en los campos
void on_page4_password_changed(AdwPasswordEntryRow *entry, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;
    if (!data) return;

    // Verificar longitud de contraseña
    page4_validate_password_length(data);

    // Verificar coincidencia cuando cambia la contraseña principal
    page4_check_password_match(data);

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);
}

void on_page4_password_confirm_changed(AdwPasswordEntryRow *entry, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;
    if (!data) return;

    // Verificar coincidencia de contraseñas
    page4_check_password_match(data);

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);
}

// Callback para cambios en el campo de usuario
void on_page4_username_changed(AdwEntryRow *entry, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;
    if (!data) return;

    // Obtener texto actual
    const gchar *current_text = gtk_editable_get_text(GTK_EDITABLE(entry));

    // Filtrar espacios automáticamente
    if (current_text && strchr(current_text, ' ') != NULL) {
        gchar *filtered_text = g_strdup(current_text);
        gchar *write_pos = filtered_text;

        // Remover todos los espacios
        for (const gchar *read_pos = current_text; *read_pos; read_pos++) {
            if (*read_pos != ' ') {
                *write_pos++ = *read_pos;
            }
        }
        *write_pos = '\0';

        // Actualizar el entry sin espacios
        g_signal_handlers_block_by_func(entry, on_page4_username_changed, user_data);
        gtk_editable_set_text(GTK_EDITABLE(entry), filtered_text);
        g_signal_handlers_unblock_by_func(entry, on_page4_username_changed, user_data);

        g_free(filtered_text);
        LOG_INFO("Espacios removidos automáticamente del campo usuario");
    }

    page4_validate_username(data);

    // Actualizar archivo variables.sh si el formulario es válido
    if (page4_is_form_valid(data)) {
        if (page4_save_user_data(data)) {
            LOG_INFO("Variables actualizadas automáticamente por cambio en usuario");
        }
    }
}

// Callback para cambios en el campo de hostname
void on_page4_hostname_changed(AdwEntryRow *entry, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;
    if (!data) return;

    // Obtener texto actual
    const gchar *current_text = gtk_editable_get_text(GTK_EDITABLE(entry));

    // Filtrar espacios automáticamente
    if (current_text && strchr(current_text, ' ') != NULL) {
        gchar *filtered_text = g_strdup(current_text);
        gchar *write_pos = filtered_text;

        // Remover todos los espacios
        for (const gchar *read_pos = current_text; *read_pos; read_pos++) {
            if (*read_pos != ' ') {
                *write_pos++ = *read_pos;
            }
        }
        *write_pos = '\0';

        // Actualizar el entry sin espacios
        g_signal_handlers_block_by_func(entry, on_page4_hostname_changed, user_data);
        gtk_editable_set_text(GTK_EDITABLE(entry), filtered_text);
        g_signal_handlers_unblock_by_func(entry, on_page4_hostname_changed, user_data);

        g_free(filtered_text);
        LOG_INFO("Espacios removidos automáticamente del campo hostname");
    }

    page4_validate_hostname(data);

    // Actualizar archivo variables.sh si el formulario es válido
    if (page4_is_form_valid(data)) {
        if (page4_save_user_data(data)) {
            LOG_INFO("Variables actualizadas automáticamente por cambio en hostname");
        }
    }
}

// Función para actualizar el estado del botón siguiente
void page4_update_next_button_state(Page4Data *data)
{
    if (!data) return;

    // Buscar el botón siguiente en el revealer
    GtkWidget *next_button = NULL;

    if (data->revealer) {
        GtkWidget *revealer_child = gtk_revealer_get_child(data->revealer);
        if (revealer_child) {
            next_button = page4_find_next_button_recursive(revealer_child);
        }
    }

    if (next_button) {
        gboolean form_valid = page4_is_form_valid(data);
        gtk_widget_set_sensitive(next_button, form_valid);

        // Variable estática para rastrear el estado anterior del botón
        static gboolean previous_button_state = FALSE;

        if (form_valid) {
            LOG_INFO("Botón siguiente activado - formulario válido");
            // Guardar variables solo cuando el botón pase de desactivado a activado
            if (!previous_button_state) {
                if (page4_save_user_data(data)) {
                    LOG_INFO("Variables de usuario guardadas automáticamente");
                } else {
                    LOG_WARNING("Error al guardar variables de usuario");
                }
            }
        } else {
            LOG_INFO("Botón siguiente desactivado - formulario inválido");
        }

        // Actualizar el estado anterior
        previous_button_state = form_valid;
    } else {
        LOG_WARNING("No se pudo encontrar el botón siguiente");
    }
}

// Función para buscar recursivamente el botón siguiente
GtkWidget* page4_find_next_button_recursive(GtkWidget *widget)
{
    if (!widget) return NULL;

    // Si es un botón, verificar si es el botón siguiente
    if (GTK_IS_BUTTON(widget)) {
        const gchar *label = gtk_button_get_label(GTK_BUTTON(widget));
        const gchar *widget_name = gtk_widget_get_name(widget);

        if ((label && (g_strcmp0(label, "Siguiente") == 0 ||
                      g_strcmp0(label, "Next") == 0 ||
                      g_strcmp0(label, "Continue") == 0)) ||
            (widget_name && (g_strcmp0(widget_name, "next_button") == 0))) {
            return widget;
        }
    }

    // Buscar recursivamente en los hijos
    GtkWidget *child = gtk_widget_get_first_child(widget);
    while (child) {
        GtkWidget *result = page4_find_next_button_recursive(child);
        if (result) return result;
        child = gtk_widget_get_next_sibling(child);
    }

    return NULL;
}

// Función para crear botones de navegación
void page4_create_navigation_buttons(Page4Data *data)
{
    if (!data) return;

    // Configurar botones de navegación
    page4_update_next_button_state(data);
    LOG_INFO("Botones de navegación creados para página 4");
}

// Función para ir a la página anterior
gboolean page4_go_to_previous_page(Page4Data *data)
{
    if (!data || !data->carousel) return FALSE;

    // Ir a la página anterior (página 3)
    GtkWidget *page3 = adw_carousel_get_nth_page(data->carousel, 2);
    if (page3) {
        adw_carousel_scroll_to(data->carousel, page3, TRUE);
        return TRUE;
    }

    return FALSE;
}

// Función para ir a la página siguiente
gboolean page4_go_to_next_page(Page4Data *data)
{
    if (!data || !data->carousel) return FALSE;

    // Verificar que el formulario sea válido antes de continuar
    if (!page4_is_form_valid(data)) {
        LOG_WARNING("Formulario no válido, no se puede continuar");
        return FALSE;
    }

    // Guardar datos del usuario
    if (!page4_save_user_data(data)) {
        LOG_ERROR("No se pudieron guardar los datos del usuario");
        return FALSE;
    }

    // Ir a la página siguiente (página 5)
    GtkWidget *page5 = adw_carousel_get_nth_page(data->carousel, 4);
    if (page5) {
        adw_carousel_scroll_to(data->carousel, page5, TRUE);
        return TRUE;
    }

    return FALSE;
}

// Callbacks de navegación
void on_page4_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;

    if (page4_go_to_next_page(data)) {
        LOG_INFO("Navegación exitosa desde página 4");
    } else {
        LOG_WARNING("No se pudo navegar a la página siguiente desde página 4");
    }
}

void on_page4_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page4Data *data = (Page4Data *)user_data;

    if (page4_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 4");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 4");
    }
}

// Funciones de utilidad
void page4_reset_form(Page4Data *data)
{
    if (!data) return;

    // Limpiar campos
    if (data->username_entry) {
        gtk_editable_set_text(GTK_EDITABLE(data->username_entry), "");
    }
    if (data->password_entry) {
        gtk_editable_set_text(GTK_EDITABLE(data->password_entry), "");
    }
    if (data->password_confirm_entry) {
        gtk_editable_set_text(GTK_EDITABLE(data->password_confirm_entry), "");
    }

    // Resetear estado
    data->passwords_match = FALSE;

    // Actualizar interfaz
    page4_check_password_match(data);

    LOG_INFO("Formulario de página 4 reseteado");
}

const gchar* page4_get_username(Page4Data *data)
{
    if (!data || !data->username_entry) return NULL;
    return gtk_editable_get_text(GTK_EDITABLE(data->username_entry));
}

const gchar* page4_get_password(Page4Data *data)
{
    if (!data || !data->password_entry) return NULL;
    return gtk_editable_get_text(GTK_EDITABLE(data->password_entry));
}

const gchar* page4_get_hostname(Page4Data *data)
{
    if (!data || !data->hostname_entry) return NULL;
    return gtk_editable_get_text(GTK_EDITABLE(data->hostname_entry));
}

// Función para manejar la entrada a page4
void page4_on_enter(void)
{
    if (!g_page4_data) return;

    // Desactivar el botón siguiente cuando se entra en page4
    if (g_page4_data->revealer) {
        GtkWidget *revealer_child = gtk_revealer_get_child(g_page4_data->revealer);
        if (revealer_child) {
            GtkWidget *next_button = page4_find_next_button_recursive(revealer_child);
            if (next_button) {
                gtk_widget_set_sensitive(next_button, FALSE);
                LOG_INFO("Botón siguiente desactivado al entrar en page4");
            }
        }
    }

    // Ejecutar validación para determinar si activar el botón
    g_timeout_add(20, (GSourceFunc)page4_delayed_validation, g_page4_data);
}

// Función auxiliar para validación con delay
gboolean page4_delayed_validation(Page4Data *data)
{
    if (!data) return FALSE;

    // Ejecutar validación completa
    page4_validate_username(data);
    page4_validate_hostname(data);
    page4_validate_password_length(data);
    page4_check_password_match(data);

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);

    LOG_INFO("Validación completada al entrar en page4");

    // Retornar FALSE para que el timeout se ejecute solo una vez
    return FALSE;
}

gboolean page4_save_user_data(Page4Data *data)
{
    if (!data) return FALSE;

    const gchar *username = page4_get_username(data);
    const gchar *password = page4_get_password(data);
    const gchar *hostname = page4_get_hostname(data);

    if (!username || !password) {
        LOG_ERROR("Datos de usuario incompletos");
        return FALSE;
    }

    // Leer el archivo existente
    gchar *file_content = NULL;
    gsize file_size = 0;
    GError *error = NULL;

    if (!g_file_get_contents("data/bash/variables.sh", &file_content, &file_size, &error)) {
        LOG_WARNING("No se pudo leer variables.sh existente: %s", error ? error->message : "Error desconocido");
        g_clear_error(&error);
        file_content = g_strdup("#!/bin/bash\n# Variables de configuración generadas por Arcris\n# Archivo generado automáticamente - No editar manualmente\n\n");
    }

    // Dividir el contenido en líneas
    gchar **lines = g_strsplit(file_content, "\n", -1);
    g_free(file_content);

    // Variables para rastrear si ya existen
    gboolean user_found = FALSE;
    gboolean password_user_found = FALSE;
    gboolean password_root_found = FALSE;
    gboolean hostname_found = FALSE;

    // Variables para preservar drivers de hardware
    gchar *driver_video_value = NULL;
    gchar *driver_audio_value = NULL;
    gchar *driver_wifi_value = NULL;
    gchar *driver_bluetooth_value = NULL;

    // Crear nuevo contenido
    GString *new_content = g_string_new("");

    // Procesar líneas existentes y actualizar variables si existen
    gboolean last_line_empty = FALSE;

    for (int i = 0; lines[i] != NULL; i++) {
        gchar *line = g_strstrip(g_strdup(lines[i]));

        if (g_str_has_prefix(line, "export USER=") || g_str_has_prefix(line, "USER=")) {
            g_string_append_printf(new_content, "export USER=\"%s\"\n", username);
            user_found = TRUE;
            last_line_empty = FALSE;
        } else if (g_str_has_prefix(line, "export PASSWORD_USER=") || g_str_has_prefix(line, "PASSWORD_USER=")) {
            g_string_append_printf(new_content, "export PASSWORD_USER=\"%s\"\n", password);
            password_user_found = TRUE;
            last_line_empty = FALSE;
        } else if (g_str_has_prefix(line, "export PASSWORD_ROOT=") || g_str_has_prefix(line, "PASSWORD_ROOT=")) {
            g_string_append_printf(new_content, "export PASSWORD_ROOT=\"%s\"\n", password);
            password_root_found = TRUE;
            last_line_empty = FALSE;
        } else if (g_str_has_prefix(line, "export HOSTNAME=") || g_str_has_prefix(line, "HOSTNAME=")) {
            g_string_append_printf(new_content, "export HOSTNAME=\"%s\"\n", hostname ? hostname : "arcris");
            hostname_found = TRUE;
            last_line_empty = FALSE;
        } else if (strlen(line) == 0) {
            // Solo agregar línea vacía si la anterior no era vacía
            if (!last_line_empty) {
                g_string_append_c(new_content, '\n');
                last_line_empty = TRUE;
            }
        } else {
            g_string_append_printf(new_content, "%s\n", lines[i]);
            last_line_empty = FALSE;
        }

        g_free(line);
    }

    // Añadir variables que no se encontraron
    if (!user_found || !password_user_found || !password_root_found || !hostname_found) {
        if (!last_line_empty) {
            g_string_append_c(new_content, '\n');
        }
        g_string_append(new_content, "# Variables de configuración del usuario\n");

        if (!user_found) {
            g_string_append_printf(new_content, "export USER=\"%s\"\n", username);
        }
        if (!password_user_found) {
            g_string_append_printf(new_content, "export PASSWORD_USER=\"%s\"\n", password);
        }
        if (!hostname_found) {
            g_string_append_printf(new_content, "export HOSTNAME=\"%s\"\n", hostname ? hostname : "arcris");
        }
        if (!password_root_found) {
            g_string_append(new_content, "# La contraseña del usuario también será la contraseña de root\n");
            g_string_append_printf(new_content, "export PASSWORD_ROOT=\"%s\"\n", password);
        }
    }

    // Añadir variables de drivers preservadas si existen
    if (driver_video_value || driver_audio_value || driver_wifi_value || driver_bluetooth_value) {
        // Si tenemos variables de drivers preservadas, agregarlas
        if (!last_line_empty) {
            g_string_append_c(new_content, '\n');
        }

        if (driver_video_value) {
            g_string_append_printf(new_content, "%s\n", driver_video_value);
        }
        if (driver_audio_value) {
            g_string_append_printf(new_content, "%s\n", driver_audio_value);
        }
        if (driver_wifi_value) {
            g_string_append_printf(new_content, "%s\n", driver_wifi_value);
        }
        if (driver_bluetooth_value) {
            g_string_append_printf(new_content, "%s\n", driver_bluetooth_value);
        }
    }

    // Escribir el archivo actualizado
    if (!g_file_set_contents("data/bash/variables.sh", new_content->str, -1, &error)) {
        LOG_ERROR("Error al escribir variables.sh: %s", error ? error->message : "Error desconocido");
        g_clear_error(&error);
        g_strfreev(lines);
        g_string_free(new_content, TRUE);
        return FALSE;
    }

    // Limpiar memoria
    g_strfreev(lines);
    g_string_free(new_content, TRUE);
    g_free(driver_video_value);
    g_free(driver_audio_value);
    g_free(driver_wifi_value);
    g_free(driver_bluetooth_value);

    LOG_INFO("Datos del usuario guardados correctamente en data/bash/variables.sh");
    LOG_INFO("Usuario: %s", username);
    LOG_INFO("Hostname: %s", hostname ? hostname : "arcris");

    return TRUE;
}

// Función para cargar nombres reservados desde archivo
gboolean page4_load_reserved_usernames(Page4Data *data)
{
    if (!data) return FALSE;

    GError *error = NULL;
    gchar *contents = NULL;
    gsize length;

    // Leer archivo de nombres reservados
    if (!g_file_get_contents("data/reserved_usernames", &contents, &length, &error)) {
        LOG_ERROR("No se pudo cargar el archivo de nombres reservados: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    // Dividir contenido en líneas
    gchar **lines = g_strsplit(contents, "\n", -1);
    g_free(contents);

    // Contar líneas válidas (no vacías y que no empiecen con #)
    int valid_lines = 0;
    for (int i = 0; lines[i]; i++) {
        g_strstrip(lines[i]);
        if (strlen(lines[i]) > 0 && lines[i][0] != '#') {
            valid_lines++;
        }
    }

    // Crear array de nombres reservados
    data->reserved_usernames = g_malloc0((valid_lines + 1) * sizeof(gchar*));
    int index = 0;

    for (int i = 0; lines[i]; i++) {
        g_strstrip(lines[i]);
        if (strlen(lines[i]) > 0 && lines[i][0] != '#') {
            data->reserved_usernames[index++] = g_strdup(lines[i]);
        }
    }

    g_strfreev(lines);

    LOG_INFO("Cargados %d nombres reservados", valid_lines);
    return TRUE;
}

// Función para validar si el nombre de usuario es válido
gboolean page4_is_username_valid(const gchar *username, Page4Data *data)
{
    if (!username || strlen(username) == 0) return FALSE;

    // Máximo 22 caracteres
    if (strlen(username) > 22) return FALSE;

    // No puede empezar con mayúscula
    if (g_ascii_isupper(username[0])) return FALSE;

    // No puede empezar con número
    if (g_ascii_isdigit(username[0])) return FALSE;

    // No puede contener espacios
    if (strchr(username, ' ') != NULL) return FALSE;

    // Verificar si está en la lista de nombres reservados
    if (data->reserved_usernames) {
        for (int i = 0; data->reserved_usernames[i]; i++) {
            if (g_strcmp0(username, data->reserved_usernames[i]) == 0) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

// Función para validar si el hostname es válido
gboolean page4_is_hostname_valid(const gchar *hostname, Page4Data *data)
{
    if (!hostname || strlen(hostname) == 0) return FALSE;

    // Máximo 22 caracteres
    if (strlen(hostname) > 22) return FALSE;

    // No puede empezar con mayúscula
    if (g_ascii_isupper(hostname[0])) return FALSE;

    // No puede empezar con número
    if (g_ascii_isdigit(hostname[0])) return FALSE;

    // No puede contener espacios
    if (strchr(hostname, ' ') != NULL) return FALSE;

    // Verificar si está en la lista de nombres reservados
    if (data->reserved_usernames) {
        for (int i = 0; data->reserved_usernames[i]; i++) {
            if (g_strcmp0(hostname, data->reserved_usernames[i]) == 0) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

// Función para validar si la contraseña tiene longitud mínima
gboolean page4_is_password_length_valid(const gchar *password)
{
    return password && strlen(password) >= 3;
}

// Función para validar el nombre de usuario
void page4_validate_username(Page4Data *data)
{
    if (!data || !data->username_entry) return;

    const gchar *username = gtk_editable_get_text(GTK_EDITABLE(data->username_entry));

    if (strlen(username) > 0) {
        data->username_valid = page4_is_username_valid(username, data);
        if (!data->username_valid) {
            gtk_widget_add_css_class(GTK_WIDGET(data->username_entry), "error");
            adw_entry_row_set_show_apply_button(data->username_entry, FALSE);
        } else {
            adw_entry_row_set_show_apply_button(data->username_entry, TRUE);
            gtk_widget_remove_css_class(GTK_WIDGET(data->username_entry), "error");
        }
    } else {
        // Campo vacío = inválido
        data->username_valid = FALSE;
        gtk_widget_remove_css_class(GTK_WIDGET(data->username_entry), "error");
        adw_entry_row_set_show_apply_button(data->username_entry, FALSE);
    }

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);
}

// Función para validar el hostname
void page4_validate_hostname(Page4Data *data)
{
    if (!data || !data->hostname_entry) return;

    const gchar *hostname = gtk_editable_get_text(GTK_EDITABLE(data->hostname_entry));

    if (strlen(hostname) > 0) {
        data->hostname_valid = page4_is_hostname_valid(hostname, data);
        if (!data->hostname_valid) {
            gtk_widget_add_css_class(GTK_WIDGET(data->hostname_entry), "error");
            adw_entry_row_set_show_apply_button(data->hostname_entry, FALSE);
        } else {
            adw_entry_row_set_show_apply_button(data->hostname_entry, TRUE);
            gtk_widget_remove_css_class(GTK_WIDGET(data->hostname_entry), "error");
        }
    } else {
        // Campo vacío = inválido
        data->hostname_valid = FALSE;
        gtk_widget_remove_css_class(GTK_WIDGET(data->hostname_entry), "error");
        adw_entry_row_set_show_apply_button(data->hostname_entry, FALSE);
    }

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);
}

// Función para validar la longitud de la contraseña
void page4_validate_password_length(Page4Data *data)
{
    if (!data || !data->password_entry) return;

    const gchar *password = gtk_editable_get_text(GTK_EDITABLE(data->password_entry));

    if (strlen(password) > 0) {
        data->password_length_valid = page4_is_password_length_valid(password);
        if (!data->password_length_valid) {
            gtk_widget_add_css_class(GTK_WIDGET(data->password_entry), "error");
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "success");
        } else {
            gtk_widget_add_css_class(GTK_WIDGET(data->password_entry), "success");
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "error");
        }
    } else {
        // Campo vacío = inválido
        data->password_length_valid = FALSE;
        gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "error");
        gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "success");
    }

    // Actualizar estado del botón siguiente
    page4_update_next_button_state(data);
}
