#include "page5.h"
#include "config.h"
#include <stdio.h>


// Variable global para datos de la página 5
static Page5Data *g_page5_data = NULL;

// Nombres de los entornos de escritorio
static const char* DE_NAMES[] = {
    "GNOME",
    "KDE",
    "XFCE4",
    "Deepin"
};

// Nombres de los gestores de ventanas
static const char* WM_NAMES[] = {
    "i3",
    "Bspwm",
    "dwm",
    "Qtile"
};

// Recursos de imágenes para DE
static const char* DE_IMAGE_RESOURCES[] = {
    "/org/gtk/arcris/GNOME.png",
    "/org/gtk/arcris/KDE.png",
    "/org/gtk/arcris/XFCE4.png",
    "/org/gtk/arcris/Deepin.png"
};

// Recursos de imágenes para WM
static const char* WM_IMAGE_RESOURCES[] = {
    "/org/gtk/arcris/i3wm.png",
    "/org/gtk/arcris/Bspwm.png",
    "/org/gtk/arcris/dwm.png",
    "/org/gtk/arcris/Qtile.png"
};

// Función de debugging para verificar que las imágenes se carguen correctamente
static void page5_debug_check_image_resources(void)
{
    LOG_INFO("Verificando recursos de imágenes...");

    // Verificar recursos DE
    for (int i = 0; i < G_N_ELEMENTS(DE_IMAGE_RESOURCES); i++) {
        GBytes *resource = g_resources_lookup_data(DE_IMAGE_RESOURCES[i], G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        if (resource) {
            LOG_INFO("✓ Recurso DE encontrado: %s", DE_IMAGE_RESOURCES[i]);
            g_bytes_unref(resource);
        } else {
            LOG_ERROR("✗ Recurso DE no encontrado: %s", DE_IMAGE_RESOURCES[i]);
        }
    }

    // Verificar recursos WM
    for (int i = 0; i < G_N_ELEMENTS(WM_IMAGE_RESOURCES); i++) {
        GBytes *resource = g_resources_lookup_data(WM_IMAGE_RESOURCES[i], G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        if (resource) {
            LOG_INFO("✓ Recurso WM encontrado: %s", WM_IMAGE_RESOURCES[i]);
            g_bytes_unref(resource);
        } else {
            LOG_ERROR("✗ Recurso WM no encontrado: %s", WM_IMAGE_RESOURCES[i]);
        }
    }
}

// Función para forzar la actualización de las imágenes
static void page5_force_image_refresh(Page5Data *data)
{
    if (!data) return;

    // Forzar actualización de imagen DE si está visible
    if (data->de_preview_image && GTK_IS_PICTURE(data->de_preview_image)) {
        const char *resource = page5_get_de_image_resource(data->current_de);
        if (resource) {
            gtk_picture_set_resource(data->de_preview_image, resource);
            gtk_widget_set_visible(GTK_WIDGET(data->de_preview_image), TRUE);
            gtk_widget_queue_draw(GTK_WIDGET(data->de_preview_image));
            gtk_widget_queue_resize(GTK_WIDGET(data->de_preview_image));
            LOG_INFO("Imagen DE forzada a actualizar: %s", resource);
        }
    }

    // Forzar actualización de imagen WM si está visible
    if (data->wm_preview_image && GTK_IS_PICTURE(data->wm_preview_image)) {
        const char *resource = page5_get_wm_image_resource(data->current_wm);
        if (resource) {
            gtk_picture_set_resource(data->wm_preview_image, resource);
            gtk_widget_set_visible(GTK_WIDGET(data->wm_preview_image), TRUE);
            gtk_widget_queue_draw(GTK_WIDGET(data->wm_preview_image));
            gtk_widget_queue_resize(GTK_WIDGET(data->wm_preview_image));
            LOG_INFO("Imagen WM forzada a actualizar: %s", resource);
        }
    }
}

// Función de prueba para verificar que las imágenes se cargan correctamente
static gboolean page5_test_images_loaded(gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return G_SOURCE_REMOVE;

    LOG_INFO("Ejecutando prueba de carga de imágenes...");

    // Verificar que las imágenes DE estén cargadas
    if (data->de_preview_image && GTK_IS_PICTURE(data->de_preview_image)) {
        GdkPaintable *paintable = gtk_picture_get_paintable(data->de_preview_image);
        if (paintable) {
            LOG_INFO("✓ Imagen DE cargada correctamente");
        } else {
            LOG_WARNING("✗ Imagen DE no está cargada, intentando recargar...");
            page5_update_de_preview(data);
        }
    }

    // Verificar que las imágenes WM estén cargadas
    if (data->wm_preview_image && GTK_IS_PICTURE(data->wm_preview_image)) {
        GdkPaintable *paintable = gtk_picture_get_paintable(data->wm_preview_image);
        if (paintable) {
            LOG_INFO("✓ Imagen WM cargada correctamente");
        } else {
            LOG_WARNING("✗ Imagen WM no está cargada, intentando recargar...");
            page5_update_wm_preview(data);
        }
    }

    return G_SOURCE_REMOVE;
}

// Función principal de inicialización de la página 5
void page5_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Verificar que los recursos de imágenes estén disponibles
    page5_debug_check_image_resources();

    // Allocar memoria para los datos de la página
    g_page5_data = g_malloc0(sizeof(Page5Data));

    // Guardar referencias importantes
    g_page5_data->carousel = carousel;
    g_page5_data->revealer = revealer;
    g_page5_data->next_button = GTK_BUTTON(gtk_builder_get_object(builder, "next_button"));

    // Cargar la página 5 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page5.ui");
    if (!page_builder) {
        LOG_ERROR("No se pudo cargar el builder de página 5");
        return;
    }

    // Obtener widgets principales
    g_page5_data->page5 = ADW_STATUS_PAGE(gtk_builder_get_object(page_builder, "page5"));
    g_page5_data->de_page = ADW_CLAMP(gtk_builder_get_object(page_builder, "de_page"));
    g_page5_data->wm_page = ADW_CLAMP(gtk_builder_get_object(page_builder, "wm_page"));

    if (!g_page5_data->page5) {
        LOG_ERROR("No se pudo obtener page5 de la página 5");
        g_object_unref(page_builder);
        return;
    }

    // Crear un stack para manejar las páginas
    g_page5_data->pages_stack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(g_page5_data->pages_stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(g_page5_data->pages_stack, 300);

    // Agregar páginas al stack
    if (g_page5_data->page5) {
        gtk_stack_add_named(g_page5_data->pages_stack, GTK_WIDGET(g_page5_data->page5), "main");
    }
    if (g_page5_data->de_page) {
        gtk_stack_add_named(g_page5_data->pages_stack, GTK_WIDGET(g_page5_data->de_page), "de");
    }
    if (g_page5_data->wm_page) {
        gtk_stack_add_named(g_page5_data->pages_stack, GTK_WIDGET(g_page5_data->wm_page), "wm");
    }

    // Establecer el widget principal
    g_page5_data->main_content = GTK_WIDGET(g_page5_data->pages_stack);

    // Obtener widgets específicos de cada página
    g_page5_data->terminal_check = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "terminal_check"));
    g_page5_data->desktop_check = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "desktop_check"));
    g_page5_data->wm_check = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "wm_check"));

    // Obtener botones go-next-symbolic
    g_page5_data->desktop_next_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "desktop_next_button"));
    g_page5_data->wm_next_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "wm_next_button"));

    g_page5_data->de_combo = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "de_combo"));
    g_page5_data->de_preview_image = GTK_PICTURE(gtk_builder_get_object(page_builder, "de_preview_picture"));
    g_page5_data->de_back_to_main_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "de_back_to_main_button"));
    g_page5_data->de_title_label = GTK_LABEL(gtk_builder_get_object(page_builder, "de_title_label"));

    g_page5_data->wm_combo = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "wm_combo"));
    g_page5_data->wm_preview_image = GTK_PICTURE(gtk_builder_get_object(page_builder, "wm_preview_picture"));
    g_page5_data->wm_back_to_main_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "wm_back_to_main_button"));
    g_page5_data->wm_title_label = GTK_LABEL(gtk_builder_get_object(page_builder, "wm_title_label"));

    // Verificar que los widgets de preview se obtuvieron correctamente
    if (!g_page5_data->de_preview_image) {
        LOG_ERROR("No se pudo obtener el widget de preview DE");
    }
    if (!g_page5_data->wm_preview_image) {
        LOG_ERROR("No se pudo obtener el widget de preview WM");
    }

    // Obtener los ActionRows para conectar sus señales
    AdwActionRow *desktop_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "desktop_row"));
    AdwActionRow *wm_row = ADW_ACTION_ROW(gtk_builder_get_object(page_builder, "wm_row"));

    // Conectar señales de activación de filas
    if (desktop_row) {
        g_signal_connect(desktop_row, "activated",
                        G_CALLBACK(on_page5_desktop_row_activated), g_page5_data);
    }

    if (wm_row) {
        g_signal_connect(wm_row, "activated",
                        G_CALLBACK(on_page5_wm_row_activated), g_page5_data);
    }

    // Configurar valores por defecto
    g_page5_data->current_type = INSTALL_TYPE_TERMINAL;
    g_page5_data->current_de = DE_TYPE_GNOME;
    g_page5_data->current_wm = WM_TYPE_I3;

    // Realizar configuraciones iniciales
    page5_setup_widgets(g_page5_data);
    page5_load_data(g_page5_data);

    // Inicializar las imágenes de preview inmediatamente
    if (g_page5_data->de_preview_image) {
        gtk_picture_set_resource(g_page5_data->de_preview_image, page5_get_de_image_resource(g_page5_data->current_de));
        gtk_widget_set_visible(GTK_WIDGET(g_page5_data->de_preview_image), TRUE);
        LOG_INFO("Imagen DE inicializada con: %s", page5_get_de_image_resource(g_page5_data->current_de));
    }

    if (g_page5_data->wm_preview_image) {
        gtk_picture_set_resource(g_page5_data->wm_preview_image, page5_get_wm_image_resource(g_page5_data->current_wm));
        gtk_widget_set_visible(GTK_WIDGET(g_page5_data->wm_preview_image), TRUE);
        LOG_INFO("Imagen WM inicializada con: %s", page5_get_wm_image_resource(g_page5_data->current_wm));
    }

    // Añadir la página al carousel
    adw_carousel_append(carousel, g_page5_data->main_content);

    // Programar prueba de imágenes para después de que la UI esté completamente cargada
    g_timeout_add(500, page5_test_images_loaded, g_page5_data);

    // Liberar el builder de la página
    g_object_unref(page_builder);

    LOG_INFO("Página 5 (Personalización) inicializada correctamente");
}

// Función de limpieza
void page5_cleanup(Page5Data *data)
{
    if (g_page5_data) {
        g_free(g_page5_data);
        g_page5_data = NULL;
        LOG_INFO("Página 5 limpiada correctamente");
    }
}

// Función para configurar widgets
void page5_setup_widgets(Page5Data *data)
{
    if (!data) return;

    // Conectar señales de los checkboxes
    if (data->terminal_check) {
        g_signal_connect(data->terminal_check, "toggled",
                        G_CALLBACK(on_page5_terminal_check_toggled), data);
    }

    if (data->desktop_check) {
        g_signal_connect(data->desktop_check, "toggled",
                        G_CALLBACK(on_page5_desktop_check_toggled), data);
    }

    if (data->wm_check) {
        g_signal_connect(data->wm_check, "toggled",
                        G_CALLBACK(on_page5_wm_check_toggled), data);
    }

    // Conectar señales de los combo boxes
    if (data->de_combo) {
        g_signal_connect(data->de_combo, "notify::selected",
                        G_CALLBACK(on_page5_de_combo_changed), data);
    }

    if (data->wm_combo) {
        g_signal_connect(data->wm_combo, "notify::selected",
                        G_CALLBACK(on_page5_wm_combo_changed), data);
    }

    // Conectar señales de los botones de navegación
    if (data->de_back_to_main_button) {
        g_signal_connect(data->de_back_to_main_button, "clicked",
                        G_CALLBACK(on_page5_de_back_to_main_button_clicked), data);
    }

    if (data->wm_back_to_main_button) {
        g_signal_connect(data->wm_back_to_main_button, "clicked",
                        G_CALLBACK(on_page5_wm_back_to_main_button_clicked), data);
    }

    // Conectar señales de los botones go-next-symbolic
    if (data->desktop_next_button) {
        g_signal_connect(data->desktop_next_button, "clicked",
                        G_CALLBACK(on_page5_desktop_next_button_clicked), data);
    }

    if (data->wm_next_button) {
        g_signal_connect(data->wm_next_button, "clicked",
                        G_CALLBACK(on_page5_wm_next_button_clicked), data);
    }

    // Mostrar la página principal por defecto
    page5_show_main_page(data);

    // Actualizar estado inicial de los botones
    page5_update_next_buttons_state(data);

    LOG_INFO("Widgets de la página 5 configurados");
}

// Función para cargar datos
void page5_load_data(Page5Data *data)
{
    if (!data) return;

    // Actualizar previews iniciales
    page5_update_de_preview(data);
    page5_update_wm_preview(data);

    LOG_INFO("Datos de la página 5 cargados");
}

// Función para mostrar la página principal
void page5_show_main_page(Page5Data *data)
{
    if (!data || !data->pages_stack) return;

    gtk_stack_set_visible_child_name(data->pages_stack, "main");
    LOG_INFO("Mostrando página principal de personalización");
}

// Función para mostrar la página de DE
void page5_show_de_page(Page5Data *data)
{
    if (!data || !data->pages_stack) return;

    gtk_stack_set_visible_child_name(data->pages_stack, "de");
    page5_update_de_preview(data);

    // Forzar actualización de imagen después de mostrar la página
    g_idle_add((GSourceFunc)page5_force_image_refresh, data);

    LOG_INFO("Mostrando página de selección de entorno de escritorio");
}

// Función para mostrar la página de WM
void page5_show_wm_page(Page5Data *data)
{
    if (!data || !data->pages_stack) return;

    gtk_stack_set_visible_child_name(data->pages_stack, "wm");
    page5_update_wm_preview(data);

    // Forzar actualización de imagen después de mostrar la página
    g_idle_add((GSourceFunc)page5_force_image_refresh, data);

    LOG_INFO("Mostrando página de selección de gestor de ventanas");
}

// Función para establecer el tipo de instalación
void page5_set_installation_type(Page5Data *data, InstallationType type)
{
    if (!data) return;

    data->current_type = type;

    // Actualizar estado de los checkboxes
    if (data->terminal_check) {
        gtk_check_button_set_active(data->terminal_check, type == INSTALL_TYPE_TERMINAL);
    }
    if (data->desktop_check) {
        gtk_check_button_set_active(data->desktop_check, type == INSTALL_TYPE_DESKTOP);
    }
    if (data->wm_check) {
        gtk_check_button_set_active(data->wm_check, type == INSTALL_TYPE_WINDOW_MANAGER);
    }

    LOG_INFO("Tipo de instalación establecido: %d", type);

    // Actualizar estado de los botones
    page5_update_next_buttons_state(data);
}

// Función para obtener el tipo de instalación
InstallationType page5_get_installation_type(Page5Data *data)
{
    if (!data) return INSTALL_TYPE_TERMINAL;
    return data->current_type;
}

// Función auxiliar para obtener el botón "next_button" del revealer
static GtkWidget* page5_get_next_button(Page5Data *data)
{
    if (!data || !data->next_button) return NULL;
    return GTK_WIDGET(data->next_button);
}

// Función auxiliar para guardar variable DE en el archivo de configuración
static gboolean page5_save_de_variable(DesktopEnvironmentType de)
{
    GError *error = NULL;
    gchar *config_content = NULL;
    const gchar *config_path = "Arcris2/data/variables.sh";

    // Leer el archivo actual
    if (!g_file_get_contents(config_path, &config_content, NULL, &error)) {
        LOG_ERROR("Error al leer archivo de configuración: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }

    // Obtener el nombre del DE
    gchar *de_name = NULL;
    switch (de) {
        case DE_TYPE_GNOME:
            de_name = g_strdup("GNOME");
            break;
        case DE_TYPE_KDE:
            de_name = g_strdup("KDE");
            break;
        case DE_TYPE_XFCE4:
            de_name = g_strdup("XFCE4");
            break;
        case DE_TYPE_DEEPIN:
            de_name = g_strdup("DEEPIN");
            break;
        default:
            de_name = g_strdup("GNOME");
            break;
    }

    // Crear el nuevo contenido
    gchar *new_content = g_strdup_printf("%s\n# Variable DE seleccionada\nDESKTOP_ENVIRONMENT=\"%s\"\n",
                                       config_content, de_name);

    // Escribir el nuevo contenido
    gboolean success = g_file_set_contents(config_path, new_content, -1, &error);

    if (error) {
        LOG_ERROR("Error al guardar variable DE: %s", error->message);
        g_error_free(error);
        success = FALSE;
    } else {
        LOG_INFO("Variable DE guardada: %s", de_name);
    }

    g_free(config_content);
    g_free(new_content);
    g_free(de_name);

    return success;
}

// Función auxiliar para guardar variable WM en el archivo de configuración
static gboolean page5_save_wm_variable(WindowManagerType wm)
{
    GError *error = NULL;
    gchar *config_content = NULL;
    const gchar *config_path = "Arcris2/data/variables.sh";

    // Leer el archivo actual
    if (!g_file_get_contents(config_path, &config_content, NULL, &error)) {
        LOG_ERROR("Error al leer archivo de configuración: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }

    // Obtener el nombre del WM
    gchar *wm_name = NULL;
    switch (wm) {
        case WM_TYPE_I3:
            wm_name = g_strdup("I3");
            break;
        case WM_TYPE_BSPWM:
            wm_name = g_strdup("BSPWM");
            break;
        case WM_TYPE_DWM:
            wm_name = g_strdup("DWM");
            break;
        case WM_TYPE_QTILE:
            wm_name = g_strdup("QTILE");
            break;
        default:
            wm_name = g_strdup("I3");
            break;
    }

    // Crear el nuevo contenido
    gchar *new_content = g_strdup_printf("%s\n# Variable WM seleccionada\nWINDOW_MANAGER=\"%s\"\n",
                                       config_content, wm_name);

    // Escribir el nuevo contenido
    gboolean success = g_file_set_contents(config_path, new_content, -1, &error);

    if (error) {
        LOG_ERROR("Error al guardar variable WM: %s", error->message);
        g_error_free(error);
        success = FALSE;
    } else {
        LOG_INFO("Variable WM guardada: %s", wm_name);
    }

    g_free(config_content);
    g_free(new_content);
    g_free(wm_name);

    return success;
}

// Función para actualizar el estado de los botones go-next-symbolic
void page5_update_next_buttons_state(Page5Data *data)
{
    if (!data) return;

    // Activar/desactivar botones según el tipo de instalación
    if (data->desktop_next_button) {
        gtk_widget_set_sensitive(GTK_WIDGET(data->desktop_next_button),
                                data->current_type == INSTALL_TYPE_DESKTOP);
    }

    if (data->wm_next_button) {
        gtk_widget_set_sensitive(GTK_WIDGET(data->wm_next_button),
                                data->current_type == INSTALL_TYPE_WINDOW_MANAGER);
    }

    LOG_INFO("Estado de botones actualizado - DE: %s, WM: %s",
             data->current_type == INSTALL_TYPE_DESKTOP ? "activo" : "inactivo",
             data->current_type == INSTALL_TYPE_WINDOW_MANAGER ? "activo" : "inactivo");
}

// Función para establecer el entorno de escritorio
void page5_set_desktop_environment(Page5Data *data, DesktopEnvironmentType de)
{
    if (!data) return;

    data->current_de = de;

    if (data->de_combo) {
        adw_combo_row_set_selected(data->de_combo, page5_de_type_to_index(de));
    }

    page5_update_de_preview(data);
    LOG_INFO("Entorno de escritorio establecido: %s", page5_get_de_name(de));
}

// Función para obtener el entorno de escritorio
DesktopEnvironmentType page5_get_desktop_environment(Page5Data *data)
{
    if (!data) return DE_TYPE_GNOME;
    return data->current_de;
}

// Función para actualizar la preview del DE
void page5_update_de_preview(Page5Data *data)
{
    if (!data || !data->de_preview_image) {
        LOG_ERROR("Datos nulos o widget de preview DE no disponible");
        return;
    }

    const char *resource = page5_get_de_image_resource(data->current_de);
    if (resource) {
        // Verificar que el widget sea válido antes de configurar la imagen
        if (!GTK_IS_PICTURE(data->de_preview_image)) {
            LOG_ERROR("Widget de preview DE no es un GtkPicture válido");
            return;
        }

        gtk_picture_set_resource(data->de_preview_image, resource);

        // Asegurar que el widget sea visible y se redibuje
        gtk_widget_set_visible(GTK_WIDGET(data->de_preview_image), TRUE);
        gtk_widget_queue_draw(GTK_WIDGET(data->de_preview_image));
        gtk_widget_queue_resize(GTK_WIDGET(data->de_preview_image));

        LOG_INFO("Preview DE actualizado: %s para %s", resource, page5_get_de_name(data->current_de));
    } else {
        LOG_ERROR("No se pudo obtener el recurso de imagen para DE: %d", data->current_de);
    }
}

// Función para establecer el gestor de ventanas
void page5_set_window_manager(Page5Data *data, WindowManagerType wm)
{
    if (!data) return;

    data->current_wm = wm;

    if (data->wm_combo) {
        adw_combo_row_set_selected(data->wm_combo, page5_wm_type_to_index(wm));
    }

    page5_update_wm_preview(data);
    LOG_INFO("Gestor de ventanas establecido: %s", page5_get_wm_name(wm));
}

// Función para obtener el gestor de ventanas
WindowManagerType page5_get_window_manager(Page5Data *data)
{
    if (!data) return WM_TYPE_I3;
    return data->current_wm;
}

// Función para actualizar la preview del WM
void page5_update_wm_preview(Page5Data *data)
{
    if (!data || !data->wm_preview_image) {
        LOG_ERROR("Datos nulos o widget de preview WM no disponible");
        return;
    }

    const char *resource = page5_get_wm_image_resource(data->current_wm);
    if (resource) {
        // Verificar que el widget sea válido antes de configurar la imagen
        if (!GTK_IS_PICTURE(data->wm_preview_image)) {
            LOG_ERROR("Widget de preview WM no es un GtkPicture válido");
            return;
        }

        gtk_picture_set_resource(data->wm_preview_image, resource);

        // Asegurar que el widget sea visible y se redibuje
        gtk_widget_set_visible(GTK_WIDGET(data->wm_preview_image), TRUE);
        gtk_widget_queue_draw(GTK_WIDGET(data->wm_preview_image));
        gtk_widget_queue_resize(GTK_WIDGET(data->wm_preview_image));

        LOG_INFO("Preview WM actualizado: %s para %s", resource, page5_get_wm_name(data->current_wm));
    } else {
        LOG_ERROR("No se pudo obtener el recurso de imagen para WM: %d", data->current_wm);
    }
}

// Función para verificar si la configuración es válida
gboolean page5_is_configuration_valid(Page5Data *data)
{
    if (!data) return FALSE;

    // Siempre es válida porque terminal está seleccionado por defecto
    return TRUE;
}

// Función para verificar si se puede proceder a la siguiente página
gboolean page5_can_proceed_to_next_page(Page5Data *data)
{
    return page5_is_configuration_valid(data);
}

// Callbacks para checkboxes
void on_page5_terminal_check_toggled(GtkCheckButton *check, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    if (gtk_check_button_get_active(check)) {
        page5_set_installation_type(data, INSTALL_TYPE_TERMINAL);

        // Activar el botón siguiente del GtkRevealer
        GtkWidget *next_button = page5_get_next_button(data);
        if (next_button) {
            gtk_widget_set_sensitive(next_button, TRUE);
            LOG_INFO("Botón siguiente activado al presionar terminal_check");
        }

        page5_show_main_page(data);
    }
}

void on_page5_desktop_check_toggled(GtkCheckButton *check, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    if (gtk_check_button_get_active(check)) {
        page5_set_installation_type(data, INSTALL_TYPE_DESKTOP);

        // Desactivar el botón siguiente del GtkRevealer
        GtkWidget *next_button = page5_get_next_button(data);
        if (next_button) {
            gtk_widget_set_sensitive(next_button, FALSE);
            LOG_INFO("Botón siguiente desactivado al presionar desktop_check");
        }

        // No navegar automáticamente, esperar a que se active el row
    }
}

void on_page5_wm_check_toggled(GtkCheckButton *check, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    if (gtk_check_button_get_active(check)) {
        page5_set_installation_type(data, INSTALL_TYPE_WINDOW_MANAGER);

        // Desactivar el botón siguiente del GtkRevealer
        GtkWidget *next_button = page5_get_next_button(data);
        if (next_button) {
            gtk_widget_set_sensitive(next_button, FALSE);
            LOG_INFO("Botón siguiente desactivado al presionar wm_check");
        }

        // No navegar automáticamente, esperar a que se active el row
    }
}

// Callbacks para combo boxes
void on_page5_de_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    guint selected = adw_combo_row_get_selected(combo);
    DesktopEnvironmentType de = page5_index_to_de_type(selected);

    data->current_de = de;
    page5_update_de_preview(data);
    page5_force_image_refresh(data);

    LOG_INFO("Entorno de escritorio cambiado a: %s", page5_get_de_name(de));
}

void on_page5_wm_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    guint selected = adw_combo_row_get_selected(combo);
    WindowManagerType wm = page5_index_to_wm_type(selected);

    data->current_wm = wm;
    page5_update_wm_preview(data);
    page5_force_image_refresh(data);

    LOG_INFO("Gestor de ventanas cambiado a: %s", page5_get_wm_name(wm));
}

// Callbacks para filas activables
void on_page5_desktop_row_activated(AdwActionRow *row, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    // Solo navegar si el tipo DE está seleccionado
    if (data->current_type == INSTALL_TYPE_DESKTOP) {
        page5_show_de_page(data);
        // Desactivar el botón siguiente
        if (data->desktop_next_button) {
            gtk_widget_set_sensitive(GTK_WIDGET(data->desktop_next_button), FALSE);
        }
    } else {
        // Activar el checkbox si no está seleccionado
        if (data->desktop_check) {
            gtk_check_button_set_active(data->desktop_check, TRUE);
        }
    }
}

void on_page5_wm_row_activated(AdwActionRow *row, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    // Solo navegar si el tipo WM está seleccionado
    if (data->current_type == INSTALL_TYPE_WINDOW_MANAGER) {
        page5_show_wm_page(data);
        // Desactivar el botón siguiente
        if (data->wm_next_button) {
            gtk_widget_set_sensitive(GTK_WIDGET(data->wm_next_button), FALSE);
        }
    } else {
        // Activar el checkbox si no está seleccionado
        if (data->wm_check) {
            gtk_check_button_set_active(data->wm_check, TRUE);
        }
    }
}



// Callbacks para botones de navegación hacia atrás
void on_page5_de_back_to_main_button_clicked(GtkButton *button, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    // Desactivar el botón siguiente
    GtkWidget *next_button = page5_get_next_button(data);
    if (next_button) {
        gtk_widget_set_sensitive(next_button, FALSE);
        LOG_INFO("Botón siguiente desactivado al presionar de_back_to_main_button");
    }

    page5_show_main_page(data);
    LOG_INFO("Regresando a página principal desde DE con botón go-previous");
}

void on_page5_wm_back_to_main_button_clicked(GtkButton *button, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    // Desactivar el botón siguiente
    GtkWidget *next_button = page5_get_next_button(data);
    if (next_button) {
        gtk_widget_set_sensitive(next_button, FALSE);
        LOG_INFO("Botón siguiente desactivado al presionar wm_back_to_main_button");
    }

    page5_show_main_page(data);
    LOG_INFO("Regresando a página principal desde WM con botón go-previous");
}

// Callbacks para botones go-next-symbolic
// Función de callback para el timeout del desktop_next_button
static gboolean on_desktop_next_button_timeout(gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return FALSE;

    // Habilitar el botón next_button
    GtkWidget *next_button = page5_get_next_button(data);
    if (next_button) {
        gtk_widget_set_sensitive(next_button, TRUE);
        LOG_INFO("Botón next_button habilitado desde desktop_next_button con timeout");
    }

    return FALSE; // No repetir el timeout
}

void on_page5_desktop_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    // Guardar la variable activa del combo DE
    if (data->de_combo) {
        guint selected = adw_combo_row_get_selected(data->de_combo);
        DesktopEnvironmentType de = page5_index_to_de_type(selected);

        // Guardar la variable usando la función auxiliar
        page5_save_de_variable(de);
    }

    if (data->current_type == INSTALL_TYPE_DESKTOP) {
        page5_show_de_page(data);
        LOG_INFO("Navegando a página DE con botón go-next");
    }

    if (data->current_type == INSTALL_TYPE_DESKTOP) {
        page5_show_de_page(data);
        LOG_INFO("Navegando a página DE con botón go-next");
    }

    // Programar activación del botón next_button con timeout
    g_timeout_add(100, on_desktop_next_button_timeout, data);
    LOG_INFO("Timeout programado para habilitar next_button desde desktop_next_button en 2 segundos");
}

// Función de callback para el timeout del wm_next_button
static gboolean on_wm_next_button_timeout(gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return FALSE;

    // Habilitar el botón next_button
    GtkWidget *next_button = page5_get_next_button(data);
    if (next_button) {
        gtk_widget_set_sensitive(next_button, TRUE);
        LOG_INFO("Botón next_button habilitado desde wm_next_button con timeout");
    }

    return FALSE; // No repetir el timeout
}

void on_page5_wm_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;
    if (!data) return;

    // Guardar la variable activa del combo WM
    if (data->wm_combo) {
        guint selected = adw_combo_row_get_selected(data->wm_combo);
        WindowManagerType wm = page5_index_to_wm_type(selected);

        // Guardar la variable usando la función auxiliar
        page5_save_wm_variable(wm);
    }

    // Mostrar la página WM
    if (data->current_type == INSTALL_TYPE_WINDOW_MANAGER) {
        page5_show_wm_page(data);
        LOG_INFO("Navegando a página WM con botón go-next");
    }

    // Programar activación del botón next_button con timeout
    g_timeout_add(100, on_wm_next_button_timeout, data);
    LOG_INFO("Timeout programado para habilitar next_button desde wm_next_button en 2 segundos");
}

// Función para ir a la página siguiente
gboolean page5_go_to_next_page(Page5Data *data)
{
    if (!data || !data->carousel) return FALSE;

    if (!page5_can_proceed_to_next_page(data)) {
        LOG_WARNING("No se puede proceder: configuración inválida");
        return FALSE;
    }

    // Ir a la página 6
    GtkWidget *next_page = adw_carousel_get_nth_page(data->carousel, 5);
    if (next_page) {
        adw_carousel_scroll_to(data->carousel, next_page, TRUE);
        LOG_INFO("Navegación a página 6 exitosa desde página 5");
        return TRUE;
    }

    return FALSE;
}

// Función para ir a la página anterior
gboolean page5_go_to_previous_page(Page5Data *data)
{
    if (!data || !data->carousel) return FALSE;

    // Ir a la página anterior (página 4)
    GtkWidget *page4 = adw_carousel_get_nth_page(data->carousel, 3);
    if (page4) {
        adw_carousel_scroll_to(data->carousel, page4, TRUE);
        LOG_INFO("Navegación a página anterior exitosa desde página 5");
        return TRUE;
    }

    return FALSE;
}

// Función para crear botones de navegación
void page5_create_navigation_buttons(Page5Data *data)
{
    if (!data) return;

    LOG_INFO("Botones de navegación creados para página 5");
}

// Callbacks de navegación
void on_page5_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;

    if (page5_go_to_next_page(data)) {
        LOG_INFO("Navegación exitosa desde página 5");
    } else {
        LOG_WARNING("No se pudo navegar desde página 5");
    }
}

void on_page5_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page5Data *data = (Page5Data *)user_data;

    if (page5_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 5");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 5");
    }
}

// Funciones de utilidad para recursos
const char* page5_get_de_image_resource(DesktopEnvironmentType de)
{
    if (de >= 0 && de < G_N_ELEMENTS(DE_IMAGE_RESOURCES)) {
        return DE_IMAGE_RESOURCES[de];
    }
    return DE_IMAGE_RESOURCES[0]; // GNOME por defecto
}

const char* page5_get_wm_image_resource(WindowManagerType wm)
{
    if (wm >= 0 && wm < G_N_ELEMENTS(WM_IMAGE_RESOURCES)) {
        return WM_IMAGE_RESOURCES[wm];
    }
    return WM_IMAGE_RESOURCES[0]; // i3 por defecto
}

const char* page5_get_de_name(DesktopEnvironmentType de)
{
    if (de >= 0 && de < G_N_ELEMENTS(DE_NAMES)) {
        return DE_NAMES[de];
    }
    return DE_NAMES[0]; // GNOME por defecto
}

const char* page5_get_wm_name(WindowManagerType wm)
{
    if (wm >= 0 && wm < G_N_ELEMENTS(WM_NAMES)) {
        return WM_NAMES[wm];
    }
    return WM_NAMES[0]; // i3 por defecto
}

// Función para guardar configuración
gboolean page5_save_configuration(Page5Data *data)
{
    if (!data) return FALSE;

    // Aquí se podría implementar el guardado de configuración
    LOG_INFO("Configuración guardada - Tipo: %d, DE: %d, WM: %d",
             data->current_type, data->current_de, data->current_wm);

    return TRUE;
}

// Función para aplicar configuración
gboolean page5_apply_configuration(Page5Data *data)
{
    if (!data) return FALSE;

    // Aquí se podría implementar la aplicación de configuración
    LOG_INFO("Configuración aplicada");

    return TRUE;
}

// Función llamada cuando se muestra la página 5
void page5_on_page_shown(void)
{
    LOG_INFO("Página 5 mostrada - Personalización del sistema");

    // Actualizar las imágenes cuando se muestra la página
    if (g_page5_data) {
        page5_update_de_preview(g_page5_data);
        page5_update_wm_preview(g_page5_data);
        page5_force_image_refresh(g_page5_data);
        LOG_INFO("Imágenes de preview actualizadas al mostrar la página");
    }
}

// Función para obtener el widget principal
GtkWidget* page5_get_widget(void)
{
    if (!g_page5_data) return NULL;
    return g_page5_data->main_content;
}

// Funciones de conversión
DesktopEnvironmentType page5_index_to_de_type(guint index)
{
    if (index < G_N_ELEMENTS(DE_NAMES)) {
        return (DesktopEnvironmentType)index;
    }
    return DE_TYPE_GNOME;
}

WindowManagerType page5_index_to_wm_type(guint index)
{
    if (index < G_N_ELEMENTS(WM_NAMES)) {
        return (WindowManagerType)index;
    }
    return WM_TYPE_I3;
}

guint page5_de_type_to_index(DesktopEnvironmentType de)
{
    if (de >= 0 && de < G_N_ELEMENTS(DE_NAMES)) {
        return (guint)de;
    }
    return 0;
}

guint page5_wm_type_to_index(WindowManagerType wm)
{
    if (wm >= 0 && wm < G_N_ELEMENTS(WM_NAMES)) {
        return (guint)wm;
    }
    return 0;
}
