#include "page8.h"
#include "page9.h"
#include "config.h"
#include <glib/gstdio.h>
#include <vte/vte.h>

// Instancia global
static Page8Data *g_page8_data = NULL;

// Constantes
#define CAROUSEL_ADVANCE_INTERVAL 4000 // 4 segundos en milisegundos

// Forward declarations
static gboolean page8_navigate_to_completion(Page8Data *data);
#define TOTAL_CAROUSEL_IMAGES 4

Page8Data* page8_new(void)
{
    if (g_page8_data) {
        return g_page8_data;
    }

    Page8Data *data = g_new0(Page8Data, 1);
    data->carousel_timeout_id = 0;
    data->current_image_index = 0;
    data->total_images = TOTAL_CAROUSEL_IMAGES;
    data->progress_bar_timeout_id = 0;
    data->is_installing = FALSE;
    data->carousel_auto_advance = FALSE;
    data->terminal_visible = FALSE;

    g_page8_data = data;
    return data;
}

void page8_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    if (g_page8_data) {
        LOG_INFO("Page8 ya está inicializada");
        return;
    }

    LOG_INFO("=== DEBUG: Inicializando página 8 ===");
    LOG_INFO("DEBUG: carousel = %p, revealer = %p", carousel, revealer);

    // Crear builder específico para la página 8
    GtkBuilder *page_builder = gtk_builder_new();
    GError *error = NULL;
    LOG_INFO("DEBUG: Builder creado, cargando page8.ui...");

    if (!gtk_builder_add_from_resource(page_builder, "/org/gtk/arcris/page8.ui", &error)) {
        LOG_ERROR("DEBUG: Error cargando page8.ui: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        g_object_unref(page_builder);
        return;
    }
    LOG_INFO("DEBUG: page8.ui cargado exitosamente");

    // Crear datos de la página 8
    LOG_INFO("DEBUG: Creando datos de la página 8...");
    g_page8_data = page8_new();
    g_page8_data->carousel = carousel;
    g_page8_data->revealer = revealer;
    LOG_INFO("DEBUG: g_page8_data creado = %p", g_page8_data);

    // Obtener el widget principal
    LOG_INFO("DEBUG: Obteniendo widget principal main_bin...");
    g_page8_data->main_content = GTK_WIDGET(gtk_builder_get_object(page_builder, "main_bin"));
    if (!g_page8_data->main_content) {
        LOG_ERROR("DEBUG: No se pudo obtener main_bin de page8.ui");
        g_object_unref(page_builder);
        return;
    }
    LOG_INFO("DEBUG: main_content obtenido = %p", g_page8_data->main_content);

    // Obtener stack principal y botón de terminal
    LOG_INFO("DEBUG: Obteniendo widgets principales...");
    g_page8_data->main_stack = GTK_STACK(gtk_builder_get_object(page_builder, "main_stack"));
    g_page8_data->terminal_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(page_builder, "terminal_button"));

    // Obtener widgets del carousel
    g_page8_data->image_carousel = ADW_CAROUSEL(gtk_builder_get_object(page_builder, "image_carousel"));
    g_page8_data->carousel_indicators = ADW_CAROUSEL_INDICATOR_DOTS(gtk_builder_get_object(page_builder, "carousel_indicators"));
    LOG_INFO("DEBUG: main_stack = %p, terminal_button = %p, image_carousel = %p",
             g_page8_data->main_stack, g_page8_data->terminal_button, g_page8_data->image_carousel);

    // Obtener imágenes del carousel
    g_page8_data->carousel_image1 = GTK_PICTURE(gtk_builder_get_object(page_builder, "carousel_image1"));
    g_page8_data->carousel_image2 = GTK_PICTURE(gtk_builder_get_object(page_builder, "carousel_image2"));
    g_page8_data->carousel_image3 = GTK_PICTURE(gtk_builder_get_object(page_builder, "carousel_image3"));
    g_page8_data->carousel_image4 = GTK_PICTURE(gtk_builder_get_object(page_builder, "carousel_image4"));

    // Configurar las rutas de las imágenes programáticamente
    LOG_INFO("DEBUG: Configurando rutas de imágenes del carousel...");
    if (g_page8_data->carousel_image1) {
        gtk_picture_set_resource(g_page8_data->carousel_image1, "/org/gtk/arcris/carousel-image1.png");
        LOG_INFO("DEBUG: carousel_image1 configurado");
    }
    if (g_page8_data->carousel_image2) {
        gtk_picture_set_resource(g_page8_data->carousel_image2, "/org/gtk/arcris/carousel-image2.png");
        LOG_INFO("DEBUG: carousel_image2 configurado");
    }
    if (g_page8_data->carousel_image3) {
        gtk_picture_set_resource(g_page8_data->carousel_image3, "/org/gtk/arcris/carousel-image3.png");
        LOG_INFO("DEBUG: carousel_image3 configurado");
    }
    if (g_page8_data->carousel_image4) {
        gtk_picture_set_resource(g_page8_data->carousel_image4, "/org/gtk/arcris/carousel-image4.png");
        LOG_INFO("DEBUG: carousel_image4 configurado");
    }

    // Configurar aspectos visuales de las imágenes
    LOG_INFO("DEBUG: Configurando aspectos visuales de las imágenes...");
    if (g_page8_data->carousel_image1) {
        gtk_picture_set_can_shrink(g_page8_data->carousel_image1, TRUE);
        gtk_picture_set_content_fit(g_page8_data->carousel_image1, GTK_CONTENT_FIT_CONTAIN);
    }
    if (g_page8_data->carousel_image2) {
        gtk_picture_set_can_shrink(g_page8_data->carousel_image2, TRUE);
        gtk_picture_set_content_fit(g_page8_data->carousel_image2, GTK_CONTENT_FIT_CONTAIN);
    }
    if (g_page8_data->carousel_image3) {
        gtk_picture_set_can_shrink(g_page8_data->carousel_image3, TRUE);
        gtk_picture_set_content_fit(g_page8_data->carousel_image3, GTK_CONTENT_FIT_CONTAIN);
    }
    if (g_page8_data->carousel_image4) {
        gtk_picture_set_can_shrink(g_page8_data->carousel_image4, TRUE);
        gtk_picture_set_content_fit(g_page8_data->carousel_image4, GTK_CONTENT_FIT_CONTAIN);
    }
    LOG_INFO("DEBUG: Aspectos visuales configurados correctamente");

    // Obtener otros widgets
    g_page8_data->install_title = GTK_LABEL(gtk_builder_get_object(page_builder, "install_title"));
    g_page8_data->progress_bar = GTK_PROGRESS_BAR(gtk_builder_get_object(page_builder, "progress_bar"));

    // Obtener widgets de terminal
    g_page8_data->vte_terminal = VTE_TERMINAL(gtk_builder_get_object(page_builder, "vte_terminal"));
    g_page8_data->terminal_title = GTK_LABEL(gtk_builder_get_object(page_builder, "terminal_title"));
    g_page8_data->terminal_info = GTK_LABEL(gtk_builder_get_object(page_builder, "terminal_info"));

    // Verificar que se obtuvieron los widgets principales
    LOG_INFO("DEBUG: Verificando widgets obtenidos...");
    if (!g_page8_data->main_stack || !g_page8_data->terminal_button ||
        !g_page8_data->image_carousel || !g_page8_data->carousel_indicators ||
        !g_page8_data->install_title || !g_page8_data->progress_bar ||
        !g_page8_data->vte_terminal) {
        LOG_ERROR("DEBUG: No se pudieron obtener todos los widgets necesarios de page8.ui");
        LOG_ERROR("DEBUG: main_stack=%p, terminal_button=%p, image_carousel=%p, progress_bar=%p, vte_terminal=%p",
                  g_page8_data->main_stack, g_page8_data->terminal_button, g_page8_data->image_carousel,
                  g_page8_data->progress_bar, g_page8_data->vte_terminal);
        g_object_unref(page_builder);
        return;
    }
    LOG_INFO("DEBUG: Todos los widgets obtenidos correctamente");

    // Agregar la página al carousel principal
    LOG_INFO("DEBUG: Agregando página al carousel principal...");
    adw_carousel_append(carousel, g_page8_data->main_content);
    LOG_INFO("DEBUG: Página agregada al carousel. Total páginas ahora: %u", adw_carousel_get_n_pages(carousel));

    // Configurar widgets
    LOG_INFO("DEBUG: Configurando widgets...");
    page8_setup_widgets(g_page8_data);

    // Configurar terminal
    LOG_INFO("DEBUG: Configurando terminal...");
    page8_setup_terminal(g_page8_data);

    // Conectar señales
    LOG_INFO("DEBUG: Conectando señales...");
    g_signal_connect(g_page8_data->terminal_button, "toggled",
                     G_CALLBACK(on_terminal_button_toggled), g_page8_data);

    LOG_INFO("=== DEBUG: page8_init completado exitosamente ===");

    g_object_unref(page_builder);
    LOG_INFO("Página 8 inicializada correctamente");
}

void page8_cleanup(Page8Data *data)
{
    if (!data) return;

    LOG_INFO("Limpiando página 8");

    // Detener el timer del carousel
    page8_stop_carousel_timer(data);

    // Detener el timer del progress bar
    page8_stop_progress_bar_pulse(data);

    // Detener instalación si está en progreso
    page8_stop_installation(data);

    // Liberar memoria
    g_free(data);
    g_page8_data = NULL;

    LOG_INFO("Página 8 limpiada");
}

void page8_setup_widgets(Page8Data *data)
{
    if (!data) return;

    LOG_INFO("Configurando widgets de página 8");

    // Configurar el carousel para que se pueda navegar manualmente
    if (data->image_carousel) {
        adw_carousel_set_allow_mouse_drag(data->image_carousel, TRUE);
        adw_carousel_set_allow_scroll_wheel(data->image_carousel, TRUE);
        adw_carousel_set_allow_long_swipes(data->image_carousel, TRUE);
    }

    // Configurar la barra de progreso (la animación se inicia en page8_start_installation)
    if (data->progress_bar) {
        gtk_progress_bar_set_pulse_step(data->progress_bar, 0.1);
        gtk_progress_bar_set_show_text(data->progress_bar, FALSE);
        gtk_progress_bar_set_fraction(data->progress_bar, 0.0);
        gtk_widget_set_visible(GTK_WIDGET(data->progress_bar), TRUE);
        LOG_INFO("Progress bar configurado correctamente");
    }

    // Inicializar el stack mostrando el carousel
    if (data->main_stack) {
        gtk_stack_set_visible_child_name(data->main_stack, "carousel_page");
    }

    LOG_INFO("Widgets de página 8 configurados");
}

void page8_load_data(Page8Data *data)
{
    if (!data) return;

    LOG_INFO("Cargando datos de página 8");

    // Inicializar estado de instalación

    LOG_INFO("Datos de página 8 cargados");
}

void page8_start_carousel_timer(Page8Data *data)
{
    if (!data || !data->carousel_auto_advance) return;

    // Detener timer previo si existe
    page8_stop_carousel_timer(data);

    LOG_INFO("Iniciando timer del carousel (intervalo: %d ms)", CAROUSEL_ADVANCE_INTERVAL);

    // Crear nuevo timer
    data->carousel_timeout_id = g_timeout_add(CAROUSEL_ADVANCE_INTERVAL,
                                            page8_carousel_timeout_callback,
                                            data);
}

void page8_stop_carousel_timer(Page8Data *data)
{
    if (!data || data->carousel_timeout_id == 0) return;

    LOG_INFO("Deteniendo timer del carousel");

    g_source_remove(data->carousel_timeout_id);
    data->carousel_timeout_id = 0;
}

// Funciones para animación del progress bar
static gboolean page8_progress_bar_pulse_callback(gpointer user_data)
{
    Page8Data *data = (Page8Data*)user_data;
    if (!data || !data->progress_bar) return G_SOURCE_REMOVE;

    gtk_progress_bar_pulse(data->progress_bar);
    return G_SOURCE_CONTINUE;
}

void page8_start_progress_bar_pulse(Page8Data *data)
{
    if (!data || !data->progress_bar) return;

    // Detener timer previo si existe
    page8_stop_progress_bar_pulse(data);

    LOG_INFO("Iniciando animación del progress bar con pulse");

    // Configurar el paso del pulse
    gtk_progress_bar_set_pulse_step(data->progress_bar, 0.1);

    // Crear timer para el pulse cada 100ms
    data->progress_bar_timeout_id = g_timeout_add(100,
                                                 page8_progress_bar_pulse_callback,
                                                 data);
}

void page8_stop_progress_bar_pulse(Page8Data *data)
{
    if (!data || data->progress_bar_timeout_id == 0) return;

    LOG_INFO("Deteniendo animación del progress bar");

    g_source_remove(data->progress_bar_timeout_id);
    data->progress_bar_timeout_id = 0;
}

gboolean page8_carousel_timeout_callback(gpointer user_data)
{
    Page8Data *data = (Page8Data*)user_data;
    if (!data || !data->carousel_auto_advance) {
        return G_SOURCE_REMOVE;
    }

    page8_advance_carousel(data);

    // Continuar el timer
    return G_SOURCE_CONTINUE;
}

void page8_advance_carousel(Page8Data *data)
{
    if (!data || !data->image_carousel) return;

    // Avanzar al siguiente índice
    data->current_image_index = (data->current_image_index + 1) % data->total_images;

    LOG_INFO("Avanzando carousel a imagen %d de %d",
             data->current_image_index + 1, data->total_images);

    // Animar el carousel a la siguiente página
    GtkWidget *page = adw_carousel_get_nth_page(data->image_carousel, data->current_image_index);
    if (page) {
        adw_carousel_scroll_to(data->image_carousel, page, TRUE);
    }
}

void page8_start_installation(Page8Data *data)
{
    if (!data) return;

    LOG_INFO("Iniciando proceso de instalación");

    data->is_installing = TRUE;

    // Iniciar el carousel automático
    data->carousel_auto_advance = TRUE;
    page8_start_carousel_timer(data);

    // Iniciar animación de la barra de progreso
    page8_start_progress_bar_pulse(data);

    // Iniciar instalación automáticamente

    // Ejecutar script de instalación en la terminal VTE
    page8_execute_install_script(data);

    LOG_INFO("Instalación iniciada - carousel automático activado");
}

void page8_stop_installation(Page8Data *data)
{
    if (!data) return;

    LOG_INFO("Deteniendo proceso de instalación");

    data->is_installing = FALSE;

    // Detener el carousel automático
    data->carousel_auto_advance = FALSE;
    page8_stop_carousel_timer(data);

    // Detener la animación del progress bar
    page8_stop_progress_bar_pulse(data);

    // Instalación completada

    LOG_INFO("Instalación detenida");
}



void page8_setup_terminal(Page8Data *data)
{
    if (!data || !data->vte_terminal) return;

    LOG_INFO("Configurando terminal VTE");

    // Configurar el terminal VTE
    vte_terminal_set_scrollback_lines(data->vte_terminal, 1000);
    vte_terminal_set_scroll_on_output(data->vte_terminal, TRUE);
    vte_terminal_set_scroll_on_keystroke(data->vte_terminal, TRUE);
    vte_terminal_set_audible_bell(data->vte_terminal, FALSE);

    // Configurar colores del terminal (tema oscuro)
    GdkRGBA fg_color, bg_color;
    gdk_rgba_parse(&fg_color, "#FFFFFF");
    gdk_rgba_parse(&bg_color, "#1E1E1E");
    vte_terminal_set_colors(data->vte_terminal, &fg_color, &bg_color, NULL, 0);

    // Configurar fuente
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 11");
    vte_terminal_set_font(data->vte_terminal, font_desc);
    pango_font_description_free(font_desc);

    // Mostrar mensaje inicial en la terminal
    page8_terminal_output(data, "=== Terminal de Instalación de Arcris Linux ===\n");
    page8_terminal_output(data, "Preparando instalación del sistema...\n\n");

    LOG_INFO("Terminal VTE configurada correctamente");
}

void page8_toggle_terminal(Page8Data *data)
{
    if (!data || !data->main_stack) return;

    data->terminal_visible = !data->terminal_visible;

    if (data->terminal_visible) {
        page8_show_terminal(data);
    } else {
        page8_show_carousel(data);
    }
}

void page8_show_terminal(Page8Data *data)
{
    if (!data || !data->main_stack) return;

    LOG_INFO("Mostrando terminal VTE");

    gtk_stack_set_visible_child_name(data->main_stack, "terminal_page");
    data->terminal_visible = TRUE;

    // Detener el carousel automático cuando se muestra la terminal
    page8_stop_carousel_timer(data);
}

void page8_show_carousel(Page8Data *data)
{
    if (!data || !data->main_stack) return;

    LOG_INFO("Mostrando carousel de imágenes");

    gtk_stack_set_visible_child_name(data->main_stack, "carousel_page");
    data->terminal_visible = FALSE;

    // Reanudar el carousel automático cuando se muestra el carousel
    if (data->is_installing && data->carousel_auto_advance) {
        page8_start_carousel_timer(data);
    }
}

void page8_terminal_output(Page8Data *data, const gchar *text)
{
    if (!data || !data->vte_terminal || !text) return;

    // Escribir texto en la terminal
    vte_terminal_feed(data->vte_terminal, text, -1);
}

// Callback para cuando el script de instalación termine
static void on_install_script_finished(VteTerminal *terminal, gint status, gpointer user_data)
{
    Page8Data *data = (Page8Data*)user_data;

    LOG_INFO("=== DEBUG: CALLBACK on_install_script_finished EJECUTADO ===");
    LOG_INFO("DEBUG: terminal=%p, status=%d, user_data=%p", terminal, status, user_data);
    LOG_INFO("DEBUG: data=%p", data);
    LOG_INFO("Script de instalación terminado con estado: %d", status);

    if (status == 0) {
        LOG_INFO("=== DEBUG: Script terminado exitosamente ===");
        LOG_INFO("Instalación completada exitosamente - navegando a página 9");
        LOG_INFO("DEBUG: Programando timeout de 1 segundo para navegación...");

        // Dar un pequeño delay para que el usuario vea el mensaje final
        guint timeout_id = g_timeout_add(1000, (GSourceFunc)page8_navigate_to_completion, data);
        LOG_INFO("DEBUG: Timeout programado con ID: %u", timeout_id);
    } else {
        LOG_ERROR("Script de instalación falló con código: %d", status);
        page8_terminal_output(data, "\nERROR: La instalación falló. Revise los mensajes anteriores.\n");
    }

    // Desconectar la señal para evitar múltiples llamadas
    g_signal_handlers_disconnect_by_func(terminal, on_install_script_finished, user_data);
}

// Función para navegar a la página de finalización
static gboolean page8_navigate_to_completion(Page8Data *data)
{
    if (!data || !data->carousel) {
        LOG_ERROR("DEBUG: page8_navigate_to_completion - data o carousel es NULL");
        LOG_ERROR("DEBUG: data=%p, carousel=%p", data, data ? data->carousel : NULL);
        return FALSE;
    }

    LOG_INFO("=== DEBUG: Navegando a página de finalización (page9) ===");

    // Navegar a la página 9 (debería ser la última - ya inicializada en carousel)
    guint total_pages = adw_carousel_get_n_pages(data->carousel);
    LOG_INFO("DEBUG: Total de páginas en carousel: %u", total_pages);

    if (total_pages == 0) {
        LOG_ERROR("DEBUG: Carousel no tiene páginas!");
        return FALSE;
    }

    guint page9_index = total_pages - 1;
    LOG_INFO("DEBUG: Calculado page9_index = %u (última página)", page9_index);

    // Listar todas las páginas para debugging
    for (guint i = 0; i < total_pages; i++) {
        GtkWidget *widget = adw_carousel_get_nth_page(data->carousel, i);
        LOG_INFO("DEBUG: Página %u: widget=%p", i, widget);
    }

    GtkWidget *page9_widget = adw_carousel_get_nth_page(data->carousel, page9_index);
    LOG_INFO("DEBUG: page9_widget obtenido = %p", page9_widget);

    if (page9_widget) {
        LOG_INFO("DEBUG: Ejecutando adw_carousel_scroll_to...");
        adw_carousel_scroll_to(data->carousel, page9_widget, TRUE);
        LOG_INFO("Navegación a página 9 completada exitosamente");

        // Verificar que realmente cambió
        guint current_page = (guint)adw_carousel_get_position(data->carousel);
        LOG_INFO("DEBUG: Página actual después de scroll_to: %u", current_page);
    } else {
        LOG_ERROR("DEBUG: No se pudo encontrar la página 9 en el carousel");
        LOG_ERROR("DEBUG: page9_index=%u, total_pages=%u", page9_index, total_pages);
    }

    return FALSE; // No repetir el timeout
}

void page8_execute_install_script(Page8Data *data)
{
    if (!data || !data->vte_terminal) return;

    LOG_INFO("Ejecutando script de instalación en terminal VTE");

    // Ruta al script de instalación
    gchar *script_path = g_build_filename(g_get_current_dir(), "data", "install.sh", NULL);

    LOG_INFO("Ruta del script: %s", script_path);

    // Verificar que el script existe
    if (!g_file_test(script_path, G_FILE_TEST_EXISTS)) {
        LOG_ERROR("Script de instalación no encontrado: %s", script_path);
        page8_terminal_output(data, "ERROR: Script de instalación no encontrado\n");
        g_free(script_path);
        return;
    }

    // Hacer el script ejecutable
    gchar *chmod_command = g_strdup_printf("chmod +x %s", script_path);
    if (system(chmod_command) != 0) {
        LOG_WARNING("No se pudo hacer ejecutable el script");
    }
    g_free(chmod_command);

    // Preparar argumentos para ejecutar el script
    gchar *argv[] = {
        "/usr/bin/script",
        "-q",
        "-c",
        g_strdup_printf("bash %s", script_path),
        g_strdup_printf("%s/install.log", g_get_home_dir()),
        NULL
    };

    // Variables de entorno
    gchar *envp[] = {
        "TERM=xterm-256color",
        "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        NULL
    };

    // Conectar señal para detectar cuando el proceso termine
    LOG_INFO("DEBUG: Conectando señal 'child-exited' para detectar fin del script");
    LOG_INFO("DEBUG: vte_terminal=%p, callback=%p, data=%p",
             data->vte_terminal, on_install_script_finished, data);
    g_signal_connect(data->vte_terminal, "child-exited",
                     G_CALLBACK(on_install_script_finished), data);
    LOG_INFO("DEBUG: Señal 'child-exited' conectada exitosamente");

    // Ejecutar el script en la terminal VTE
    GError *error = NULL;
    vte_terminal_spawn_async(
        data->vte_terminal,
        VTE_PTY_DEFAULT,
        NULL,                    // working directory (usar actual)
        argv,                    // argumentos
        envp,                    // variables de entorno
        G_SPAWN_DEFAULT,         // flags
        NULL,                    // child setup function
        NULL,                    // child setup data
        NULL,                    // child setup data destroy
        -1,                      // timeout
        NULL,                    // cancellable
        NULL,                    // callback
        NULL                     // user data
    );

    if (error) {
        LOG_ERROR("Error ejecutando script de instalación: %s", error->message);
        page8_terminal_output(data, "ERROR: No se pudo ejecutar el script de instalación\n");
        g_error_free(error);
    } else {
        LOG_INFO("Script de instalación ejecutado correctamente en VTE");
        LOG_INFO("DEBUG: Script iniciado - esperando señal 'child-exited' al terminar");
        page8_terminal_output(data, "Ejecutando script de instalación...\n");
        page8_terminal_output(data, "DEBUG: Script iniciado - se detectará automáticamente cuando termine\n");
    }

    g_free(script_path);
}

void on_terminal_button_toggled(GtkToggleButton *button, gpointer user_data)
{
    Page8Data *data = (Page8Data*)user_data;
    if (!data) return;

    gboolean is_active = gtk_toggle_button_get_active(button);

    LOG_INFO("Botón de terminal %s", is_active ? "activado" : "desactivado");

    if (is_active) {
        page8_show_terminal(data);
    } else {
        page8_show_carousel(data);
    }
}

void page8_on_page_shown(void)
{
    LOG_INFO("Página 8 mostrada");

    Page8Data *data = page8_get_data();
    if (data) {
        // Cargar datos si es necesario
        page8_load_data(data);
    }
}

void page8_on_page_hidden(void)
{
    LOG_INFO("Página 8 ocultada");

    Page8Data *data = page8_get_data();
    if (data) {
        // Detener el carousel cuando se oculta la página
        page8_stop_carousel_timer(data);
    }
}

GtkWidget* page8_get_widget(void)
{
    if (g_page8_data && g_page8_data->main_content) {
        return g_page8_data->main_content;
    }
    return NULL;
}

gboolean page8_is_final_page(void)
{
    // Page8 es la página final de instalación
    return TRUE;
}

Page8Data* page8_get_data(void)
{
    return g_page8_data;
}
