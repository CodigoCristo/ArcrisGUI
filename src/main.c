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
#include "page4.h"
#include "page5.h"
#include "page6.h"
#include "page7.h"
#include "page8.h"
#include "page9.h"
#include "page10.h"
#include "i18n.h"

#include "close.h"
#include "about.h"

// Manager global del carousel
static CarouselManager *g_app_carousel_manager = NULL;

// Referencia global al botón de menú para actualizar el idioma
static GtkMenuButton *g_menu_button = NULL;

// Construir el menú de la aplicación con el idioma actual
static GMenuModel *build_app_menu(void)
{
    GMenu *menu = g_menu_new();

    // Sección: Actualizar
    GMenu *section_update = g_menu_new();
    g_menu_append(section_update,
        i18n_t("Arcris Update"),
        "app.check_updates");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(section_update));
    g_object_unref(section_update);

    // Sección: Idioma (submenú)
    GMenu *section_lang = g_menu_new();
    GMenu *submenu_lang = g_menu_new();
    g_menu_append(submenu_lang, "Español",    "app.set_language::es");
    g_menu_append(submenu_lang, "English",    "app.set_language::en");
    g_menu_append(submenu_lang, "Русский",    "app.set_language::ru");
    g_menu_append(submenu_lang, "Português",  "app.set_language::pt");
    g_menu_append(submenu_lang, "Français",   "app.set_language::fr");
    g_menu_append(submenu_lang, "Deutsch",    "app.set_language::de");
    GMenuItem *lang_item = g_menu_item_new_submenu("Idioma / Language", G_MENU_MODEL(submenu_lang));
    g_menu_append_item(section_lang, lang_item);
    g_object_unref(lang_item);
    g_object_unref(submenu_lang);
    g_menu_append_section(menu, NULL, G_MENU_MODEL(section_lang));
    g_object_unref(section_lang);

    // Sección: Acerca de
    GMenu *section_about = g_menu_new();
    g_menu_append(section_about,
        i18n_t("Arcris About"),
        "app.about");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(section_about));
    g_object_unref(section_about);

    // Sección: Salir
    GMenu *section_quit = g_menu_new();
    g_menu_append(section_quit,
        i18n_t("Salir"),
        "app.quit");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(section_quit));
    g_object_unref(section_quit);

    return G_MENU_MODEL(menu);
}

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

// Función para buscar actualizaciones desde el menú
static void check_updates_action(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    if (!g_app_carousel_manager) return;

    // Detener monitoreo de internet si estaba activo
    page1_stop_internet_monitoring();

    // Ir a la página 1 (índice 0) y ocultar navegación
    carousel_navigate_to_page(g_app_carousel_manager, 0);
    gtk_revealer_set_reveal_child(g_app_carousel_manager->revealer, FALSE);

    // Iniciar búsqueda de actualizaciones en page1
    page1_start_update_check();
}

// Función para cambiar el idioma de la interfaz (change-state callback para acción radio)
static void set_language_action(GSimpleAction *action, GVariant *new_state, gpointer user_data)
{
    if (!new_state) return;
    const char *lang_code = g_variant_get_string(new_state, NULL);

    AppLang lang = LANG_ES;
    if      (g_strcmp0(lang_code, "en") == 0) lang = LANG_EN;
    else if (g_strcmp0(lang_code, "ru") == 0) lang = LANG_RU;
    else if (g_strcmp0(lang_code, "pt") == 0) lang = LANG_PT;
    else if (g_strcmp0(lang_code, "fr") == 0) lang = LANG_FR;
    else if (g_strcmp0(lang_code, "de") == 0) lang = LANG_DE;

    // Actualizar el estado de la acción (muestra el radio seleccionado)
    g_simple_action_set_state(action, new_state);
    i18n_set_lang(lang);

    page1_update_language();
    page2_update_language();
    page3_update_language();
    page4_update_language();
    page5_update_language();
    page6_update_language();
    page7_update_language();
    page8_update_language();
    page9_update_language();
    page10_update_language();

    if (g_app_carousel_manager)
        carousel_update_button_labels(g_app_carousel_manager);

    // Reconstruir el menú con el nuevo idioma
    if (g_menu_button) {
        GMenuModel *new_menu = build_app_menu();
        gtk_menu_button_set_menu_model(g_menu_button, new_menu);
        g_object_unref(new_menu);
    }
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

        LOG_INFO("Tema de iconos personalizados configurado con recoloreado automático");
    } else {
        LOG_WARNING("No se pudo obtener el tema de iconos");
    }

    // Configurar tema dark adwaita por defecto y minimizar warnings
    AdwStyleManager *style_manager = adw_style_manager_get_default();
    if (style_manager) {
        // Configurar tema oscuro
        adw_style_manager_set_color_scheme(style_manager, ADW_COLOR_SCHEME_FORCE_DARK);
        LOG_INFO("Tema dark adwaita configurado por defecto");

        // El tema de iconos se configura automáticamente con el tema del sistema
    } else {
        LOG_WARNING("No se pudo obtener el style manager para configurar el tema");
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

    // Guardar referencia al botón de menú y establecer el menú inicial
    g_menu_button = GTK_MENU_BUTTON(gtk_builder_get_object(builder, "button_menu"));
    if (g_menu_button) {
        GMenuModel *initial_menu = build_app_menu();
        gtk_menu_button_set_menu_model(g_menu_button, initial_menu);
        g_object_unref(initial_menu);
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
    // Seleccionar el renderer GL correcto antes de cualquier inicialización de GTK
    g_setenv("GSK_RENDERER", "gl", TRUE);

    // Inicializar configuración
    if (!arcris_config_init()) {
        g_critical("Error en la inicialización de configuración");
        return EXIT_FAILURE;
    }

    LOG_INFO("=== Iniciando %s ===", arcris_get_app_name());

    // Definir las acciones de la aplicación
    const GActionEntry app_entries[] = {
        { "check_updates", check_updates_action, NULL, NULL, NULL },
        { "about", about_action, NULL, NULL, NULL },
        { "quit", quit_action, NULL, NULL, NULL },
        { "set_language", NULL, "s", "'es'", set_language_action },
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
