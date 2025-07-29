//sudo pacman -S libgnomekbd

#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Headers del proyecto
#include "config.h"
#include "window.h"
#include "carousel.h"
#include "window_hardware.h"
#include "page1.h"
#include "page2.h"
#include "page3.h"

#include "close.h"
#include "about.h"

// Manager global del carousel
static CarouselManager *g_app_carousel_manager = NULL;

// Función de limpieza de la aplicación
static void cleanup_application(void)
{
    LOG_INFO("Limpiando recursos de la aplicación...");

    if (g_app_carousel_manager) {
        carousel_manager_cleanup(g_app_carousel_manager);
        g_app_carousel_manager = NULL;
    }

    LOG_INFO("Recursos limpiados correctamente");
}

// Función para cerrar la aplicación con Ctrl+Q
static void quit_action(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkApplication *app = GTK_APPLICATION(user_data);
    LOG_INFO("Cerrando aplicación con Ctrl+Q...");

    // Ejecutar limpieza antes de cerrar
    cleanup_application();

    // Cerrar todas las ventanas y salir de la aplicación
    g_application_quit(G_APPLICATION(app));
}

// Función principal de activación
static void activate_cb(GtkApplication *app)
{
    LOG_INFO("Iniciando aplicación %s v%s", arcris_get_app_name(), arcris_get_app_version());

    // Configurar tema de iconos personalizados para recoloreado automático
    GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    if (icon_theme) {
        // Agregar rutas de recursos para iconos personalizados
        gtk_icon_theme_add_resource_path(icon_theme, "/org/gtk/arcris");
        gtk_icon_theme_add_resource_path(icon_theme, "/org/gtk/arcris/icons");

        // Configurar rutas específicas para iconos simbólicos y escalables
        gtk_icon_theme_add_resource_path(icon_theme, "/org/gtk/arcris/icons/symbolic");
        gtk_icon_theme_add_resource_path(icon_theme, "/org/gtk/arcris/icons/scalable");

        // Forzar recarga del tema para asegurar que los iconos se reconozcan
        const char *theme_name = gtk_icon_theme_get_theme_name(icon_theme);
        gtk_icon_theme_set_theme_name(icon_theme, theme_name);

        LOG_INFO("Tema de iconos personalizados configurado con recoloreado automático");
        LOG_INFO("Tema actual: %s", theme_name ? theme_name : "desconocido");
    } else {
        LOG_WARNING("No se pudo obtener el tema de iconos");
    }

    // Cargar la interfaz principal desde el archivo UI
    GtkBuilder *builder = gtk_builder_new_from_resource(RESOURCE_PATH_WINDOW);
    if (!builder) {
        LOG_ERROR("No se pudo cargar el archivo de interfaz %s", RESOURCE_PATH_WINDOW);
        return;
    }

    // Obtener la ventana principal
    AdwApplicationWindow *window = ADW_APPLICATION_WINDOW(gtk_builder_get_object(builder, "main_window"));
    if (!window) {
        LOG_ERROR("No se pudo obtener la ventana principal");
        g_object_unref(builder);
        return;
    }

    // Conectar la señal de cierre de ventana
    g_signal_connect(window, "close-request", G_CALLBACK(on_window_close_request), app);

    // Crear y configurar el manager del carousel
    g_app_carousel_manager = carousel_manager_new();
    if (!g_app_carousel_manager) {
        LOG_ERROR("No se pudo crear el CarouselManager");
        g_object_unref(builder);
        return;
    }

    // Inicializar el carousel con todas sus páginas
    carousel_manager_init(g_app_carousel_manager, builder);

    // Inicializar variables de drivers automáticamente (similar a INSTALLATION_TYPE="TERMINAL")
    window_hardware_init_auto_variables();

    // Verificar que la inicialización fue exitosa
    if (!g_app_carousel_manager->is_initialized) {
        LOG_ERROR("Error en la inicialización del CarouselManager");
        carousel_manager_cleanup(g_app_carousel_manager);
        g_app_carousel_manager = NULL;
        g_object_unref(builder);
        return;
    }

    // Configurar la aplicación y mostrar la ventana
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
    gtk_window_present(GTK_WINDOW(window));

    // Liberar el builder principal
    g_object_unref(builder);

    // Registrar función de limpieza
    g_signal_connect_swapped(app, "shutdown", G_CALLBACK(cleanup_application), NULL);

    LOG_INFO("Aplicación %s inicializada correctamente con %u páginas",
             arcris_get_app_name(), carousel_get_total_pages(g_app_carousel_manager));
}

// Función main
int main(int argc, char *argv[])
{
    // Inicializar configuración
    if (!arcris_config_init()) {
        g_critical("Error en la inicialización de configuración");
        return EXIT_FAILURE;
    }

    LOG_INFO("=== Iniciando %s ===", arcris_get_app_name());

    // Definir las acciones de la aplicación
    const GActionEntry app_entries[] = {
        { "about", about_action, NULL, NULL, NULL },
        { "quit", quit_action, NULL, NULL, NULL },
    };

    // Crear la aplicación
    g_autoptr(AdwApplication) app = NULL;
    app = adw_application_new(arcris_get_app_id(), 0);

    if (!app) {
        LOG_ERROR("No se pudo crear la aplicación %s", arcris_get_app_id());
        return EXIT_FAILURE;
    }

    // Agregar las acciones de la aplicación
    g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);

    // Configurar el atajo de teclado Ctrl+Q para cerrar la aplicación
    const char* quit_accels[] = {"<Primary>q", NULL};
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", quit_accels);

    // Conectar la señal de activación
    g_signal_connect(app, "activate", G_CALLBACK(activate_cb), NULL);

    // Ejecutar la aplicación
    int result = g_application_run(G_APPLICATION(app), argc, argv);

    LOG_INFO("=== Finalizando %s (código: %d) ===", arcris_get_app_name(), result);

    return result;
}
