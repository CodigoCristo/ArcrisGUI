#include "page1.h"
#include "page2.h"
#include <stdlib.h>
#include <unistd.h>

#define ARCRIS_REMOTE_URL  "https://github.com/CodigoCristo/ArcrisGUI.git"
#define ARCRIS_COMMIT_FILE "/usr/share/arcrisgui/commit"

// Variable global para datos de la página 1
static Page1Data *g_page1_data = NULL;

// Función para verificar conectividad a internet
static gboolean check_internet_connectivity(void)
{
    // Método 1: ping a 1.1.1.1 (Cloudflare DNS)
    int result = system("ping -c 1 -W 2 1.1.1.1 > /dev/null 2>&1");
    if (result == 0) {
        return TRUE;
    }
    
    // Método 2: ping a 8.8.8.8 (Google DNS)
    result = system("ping -c 1 -W 2 8.8.8.8 > /dev/null 2>&1");
    if (result == 0) {
        return TRUE;
    }
    
    // Método 3: curl a un servicio web ligero
    result = system("curl -s --connect-timeout 3 --max-time 5 http://httpbin.org/ip > /dev/null 2>&1");
    if (result == 0) {
        return TRUE;
    }
    
    return FALSE;
}

// Función para actualizar la UI según el estado de internet
static void update_internet_ui(gboolean has_internet)
{
    if (!g_page1_data) {
        g_print("❌ Error: datos de página nulos\n");
        return;
    }

    // En modo actualización no tocamos la UI de internet
    if (g_page1_data->is_update_mode) return;
    
    if (has_internet) {
        g_print("✅ Internet conectado - Mostrando botón Iniciar\n");

        // Ocultar elementos de "sin internet"
        if (g_page1_data->internet_label) {
            gtk_widget_set_visible(g_page1_data->internet_label, FALSE);
        }
        if (g_page1_data->spinner) {
            gtk_widget_set_visible(g_page1_data->spinner, FALSE);
        }
        if (g_page1_data->no_internet_label) {
            gtk_widget_set_visible(g_page1_data->no_internet_label, FALSE);
        }
        if (g_page1_data->update_check_label) {
            gtk_widget_set_visible(g_page1_data->update_check_label, FALSE);
        }
        
        // Mostrar y habilitar botón
        if (g_page1_data->start_button) {
            gtk_widget_set_visible(g_page1_data->start_button, TRUE);
            gtk_widget_set_sensitive(g_page1_data->start_button, TRUE);

            // Activar con Enter: establecer como widget por defecto de la ventana
            GtkRoot *root = gtk_widget_get_root(g_page1_data->start_button);
            if (root && GTK_IS_WINDOW(root)) {
                gtk_window_set_default_widget(GTK_WINDOW(root), g_page1_data->start_button);
            }
        }
    } else {
        g_print("⚠️ Sin internet - Mostrando spinner y mensaje\n");
        
        // Ocultar botón
        if (g_page1_data->start_button) {
            gtk_widget_set_visible(g_page1_data->start_button, FALSE);
        }
        
        // Ocultar label inicial si está visible
        if (g_page1_data->internet_label) {
            gtk_widget_set_visible(g_page1_data->internet_label, FALSE);
        }
        
        // Mostrar spinner y mensaje de sin internet
        if (g_page1_data->spinner) {
            gtk_widget_set_visible(g_page1_data->spinner, TRUE);
        }
        if (g_page1_data->no_internet_label) {
            gtk_label_set_text(GTK_LABEL(g_page1_data->no_internet_label), "¡Conéctese primero a Internet!");
            gtk_widget_set_visible(g_page1_data->no_internet_label, TRUE);
        }
    }
}

// Función de monitoreo continuo de internet (callback del timer)
gboolean page1_check_internet_status(gpointer user_data)
{
    if (!g_page1_data) {
        g_print("❌ Error: datos de página nulos en monitoreo\n");
        return FALSE; // Detener timer
    }
    
    gboolean current_status = check_internet_connectivity();
    
    // Solo actualizar UI si el estado cambió
    if (current_status != g_page1_data->has_internet) {
        gboolean was_disconnected = !g_page1_data->has_internet;
        gboolean now_connected = current_status;
        
        g_print("🔄 Cambio de estado de internet: %s -> %s\n", 
                g_page1_data->has_internet ? "conectado" : "desconectado",
                current_status ? "conectado" : "desconectado");
        
        g_page1_data->has_internet = current_status;
        update_internet_ui(current_status);
        
        // Solo configurar combo rows cuando cambia de desconectado a conectado Y no se ha configurado antes
        if (was_disconnected && now_connected && !g_page1_data->auto_configured) {
            g_print("🌐 Configurando automáticamente combo rows con datos de geolocalización...\n");
            auto_configure_combo_rows();
            g_page1_data->auto_configured = TRUE;
            g_print("✅ Configuración automática completada (no se repetirá)\n");
        }
    }
    
    return TRUE; // Continuar monitoreo
}

// Función para iniciar el monitoreo de internet (callback de timer inicial)
gboolean page1_start_internet_monitoring_callback(gpointer user_data)
{
    if (!g_page1_data) {
        g_print("❌ Error: no se puede iniciar monitoreo sin datos de página\n");
        return FALSE;
    }

    // Limpiar el ID: este callback ya disparó, no se puede cancelar más
    g_page1_data->internet_monitor_initial_id = 0;

    // Si ya estamos en modo actualización, no iniciar monitoreo
    if (g_page1_data->is_update_mode) {
        g_print("⏭ Timer inicial de internet ignorado (modo actualización activo)\n");
        return FALSE;
    }

    g_print("🚀 Iniciando monitoreo de internet...\n");
    
    // Realizar verificación inicial
    g_page1_data->has_internet = check_internet_connectivity();
    update_internet_ui(g_page1_data->has_internet);
    
    // Iniciar timer para monitoreo continuo cada 3 segundos
    g_page1_data->internet_monitor_id = g_timeout_add_seconds(3, page1_check_internet_status, NULL);
    
    return FALSE; // No repetir este timer inicial
}

// Función para iniciar el monitoreo de internet
void page1_start_internet_monitoring(void)
{
    if (!g_page1_data) {
        g_print("❌ Error: no se puede iniciar monitoreo sin datos de página\n");
        return;
    }
    
    // Detener monitoreo anterior si existe
    page1_stop_internet_monitoring();
    
    g_print("🚀 Iniciando monitoreo de internet...\n");
    
    // Realizar verificación inicial
    g_page1_data->has_internet = check_internet_connectivity();
    update_internet_ui(g_page1_data->has_internet);
    
    // Iniciar timer para monitoreo continuo cada 3 segundos
    g_page1_data->internet_monitor_id = g_timeout_add_seconds(3, page1_check_internet_status, NULL);
}

// Función para detener el monitoreo de internet
void page1_stop_internet_monitoring(void)
{
    if (!g_page1_data) return;

    if (g_page1_data->internet_monitor_initial_id > 0) {
        g_print("🛑 Cancelando timer inicial de internet\n");
        g_source_remove(g_page1_data->internet_monitor_initial_id);
        g_page1_data->internet_monitor_initial_id = 0;
    }
    if (g_page1_data->internet_monitor_id > 0) {
        g_print("🛑 Deteniendo monitoreo de internet\n");
        g_source_remove(g_page1_data->internet_monitor_id);
        g_page1_data->internet_monitor_id = 0;
    }
}

// Callback para el botón "Iniciar"
static void page1_start_button_clicked(GtkButton *button, gpointer user_data)
{
    if (!g_page1_data) return;
    
    g_print("▶️ Botón Iniciar presionado\n");

    // Quitar el default widget para que no interfiera en páginas siguientes
    GtkRoot *root = gtk_widget_get_root(GTK_WIDGET(button));
    if (root && GTK_IS_WINDOW(root)) {
        gtk_window_set_default_widget(GTK_WINDOW(root), NULL);
    }

    // Detener monitoreo de internet ya que vamos a la siguiente página
    page1_stop_internet_monitoring();
    
    AdwCarousel *carousel = g_page1_data->carousel;
    GtkRevealer *revealer = g_page1_data->revealer;
    
    // Mostrar los controles de navegación
    gtk_revealer_set_reveal_child(revealer, TRUE);
    
    // Mover a la siguiente página del carousel
    guint current_page = adw_carousel_get_position(carousel);
    guint total_pages = adw_carousel_get_n_pages(carousel);
    
    if (current_page + 1 < total_pages) {
        GtkWidget *next_page = adw_carousel_get_nth_page(carousel, current_page + 1);
        adw_carousel_scroll_to(carousel, next_page, 300); // 300 ms de duración para la animación
    }
}

// Función de inicialización de la página 1
void page1_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    g_print("🏁 Inicializando página 1...\n");
    
    // Allocar memoria para los datos de la página
    g_page1_data = g_malloc0(sizeof(Page1Data));
    
    // Guardar referencias importantes
    g_page1_data->carousel = carousel;
    g_page1_data->revealer = revealer;
    g_page1_data->has_internet = FALSE;
    g_page1_data->internet_monitor_id = 0;
    g_page1_data->auto_configured = FALSE;
    
    // Cargar la página 1 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page1.ui");
    GtkWidget *page1 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page1"));
    
    // Obtener widgets específicos de la página
    g_page1_data->internet_label = GTK_WIDGET(gtk_builder_get_object(page_builder, "internet"));
    g_page1_data->spinner = GTK_WIDGET(gtk_builder_get_object(page_builder, "spinner"));
    g_page1_data->no_internet_label = GTK_WIDGET(gtk_builder_get_object(page_builder, "no_internet"));
    g_page1_data->update_check_label = GTK_WIDGET(gtk_builder_get_object(page_builder, "update_check_label"));
    g_page1_data->start_button = GTK_WIDGET(gtk_builder_get_object(page_builder, "start_button"));

    // Verificar que se cargaron todos los widgets correctamente
    if (!g_page1_data->internet_label || !g_page1_data->spinner ||
        !g_page1_data->no_internet_label || !g_page1_data->update_check_label ||
        !g_page1_data->start_button) {
        g_print("❌ Error: No se pudieron cargar todos los widgets de page1.ui\n");
        return;
    }
    
    // Configurar estado inicial - mostrar mensaje de prueba
    gtk_label_set_text(GTK_LABEL(g_page1_data->internet_label), "Probando Conexión a Internet...");
    gtk_widget_set_visible(g_page1_data->internet_label, TRUE);
    gtk_widget_set_visible(g_page1_data->spinner, TRUE);
    gtk_widget_set_visible(g_page1_data->no_internet_label, FALSE);
    gtk_widget_set_visible(g_page1_data->start_button, FALSE);
    
    // Conectar señales del botón de inicio
    g_signal_connect(g_page1_data->start_button, "clicked", 
                     G_CALLBACK(page1_start_button_clicked), NULL);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page1);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
    
    // Iniciar monitoreo de internet después de 1 segundo (guardar ID para poder cancelarlo)
    g_page1_data->internet_monitor_initial_id =
        g_timeout_add_seconds(1, page1_start_internet_monitoring_callback, NULL);
    
    g_print("✅ Página 1 inicializada correctamente\n");
}

// ── Actualización ──────────────────────────────────────────────────────────

static void on_update_install_done(GObject *source, GAsyncResult *result,
                                   gpointer user_data)
{
    GSubprocess *proc = G_SUBPROCESS(source);
    GError *error = NULL;

    g_subprocess_wait_finish(proc, result, &error);

    if (!g_page1_data) {
        if (error) g_error_free(error);
        g_object_unref(proc);
        return;
    }

    gboolean success = (error == NULL) &&
                       g_subprocess_get_if_exited(proc) &&
                       (g_subprocess_get_exit_status(proc) == 0);

    if (success) {
        // El script ya lanzó la nueva instancia con nohup setsid arcris
        // Solo cerramos esta instancia
        g_application_quit(g_application_get_default());
    } else {
        if (g_page1_data->spinner)
            gtk_widget_set_visible(g_page1_data->spinner, FALSE);
        if (g_page1_data->update_check_label) {
            gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                               "Error durante la actualización. Revisa la conexión.");
            gtk_widget_remove_css_class(g_page1_data->update_check_label, "dim-label");
            gtk_widget_add_css_class(g_page1_data->update_check_label, "error");
            gtk_widget_set_visible(g_page1_data->update_check_label, TRUE);
        }
    }

    if (error) g_error_free(error);
    g_object_unref(proc);
}

static void page1_start_update_install(void)
{
    if (!g_page1_data) return;

    // UI: spinner girando + mensaje de progreso
    if (g_page1_data->update_check_label) {
        gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                           "Arcris se está actualizando...");
        gtk_widget_remove_css_class(g_page1_data->update_check_label, "warning");
        gtk_widget_remove_css_class(g_page1_data->update_check_label, "error");
        gtk_widget_remove_css_class(g_page1_data->update_check_label, "success");
        gtk_widget_add_css_class(g_page1_data->update_check_label, "dim-label");
        gtk_widget_set_visible(g_page1_data->update_check_label, TRUE);
    }
    if (g_page1_data->spinner)
        gtk_widget_set_visible(g_page1_data->spinner, TRUE);

    GError *error = NULL;
    GSubprocess *proc = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_SILENCE,
        &error,
        "update-arcris", NULL);

    if (!proc) {
        g_warning("No se pudo lanzar update-arcris: %s",
                  error ? error->message : "desconocido");
        if (error) g_error_free(error);
        if (g_page1_data->spinner)
            gtk_widget_set_visible(g_page1_data->spinner, FALSE);
        if (g_page1_data->update_check_label) {
            gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                               "Error: update-arcris no encontrado en el sistema");
            gtk_widget_remove_css_class(g_page1_data->update_check_label, "dim-label");
            gtk_widget_add_css_class(g_page1_data->update_check_label, "error");
        }
        return;
    }

    g_subprocess_wait_async(proc, NULL, on_update_install_done, NULL);
}

static void on_update_run_response(AdwAlertDialog *dialog, const char *response,
                                   gpointer user_data)
{
    if (g_strcmp0(response, "update") == 0) {
        page1_start_update_install();
    }
}

static void on_update_check_done(GObject *source, GAsyncResult *result,
                                 gpointer user_data)
{
    GSubprocess *proc = G_SUBPROCESS(source);
    gchar *stdout_buf = NULL;
    GError *error = NULL;

    g_subprocess_communicate_utf8_finish(proc, result, &stdout_buf, NULL, &error);

    if (!g_page1_data) {
        g_free(stdout_buf);
        if (error) g_error_free(error);
        g_object_unref(proc);
        return;
    }

    // Ocultar spinner al terminar
    if (g_page1_data->spinner)
        gtk_widget_set_visible(g_page1_data->spinner, FALSE);

    if (error || !stdout_buf || strlen(stdout_buf) < 40) {
        // No se pudo conectar o git no disponible
        if (g_page1_data->update_check_label) {
            gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                               "Error al verificar actualizaciones. ¿Hay internet?");
            gtk_widget_remove_css_class(g_page1_data->update_check_label, "dim-label");
            gtk_widget_add_css_class(g_page1_data->update_check_label, "error");
            gtk_widget_set_visible(g_page1_data->update_check_label, TRUE);
        }
        if (error) g_error_free(error);
        g_free(stdout_buf);
        g_object_unref(proc);
        return;
    }

    // El output de git ls-remote es: "<sha>\tHEAD\n"
    gchar *remote_sha = g_strndup(stdout_buf, 40);

    // Leer SHA local guardado por el PKGBUILD
    gchar *local_sha = NULL;
    GError *file_error = NULL;
    gboolean has_updates = FALSE;

    if (g_file_get_contents(ARCRIS_COMMIT_FILE, &local_sha, NULL, &file_error)) {
        g_strstrip(local_sha);
        has_updates = (g_strcmp0(remote_sha, local_sha) != 0);
        g_free(local_sha);
    } else {
        // No existe el archivo → asumir que hay actualización
        has_updates = TRUE;
        if (file_error) g_error_free(file_error);
    }

    GtkRoot *root = gtk_widget_get_root(g_page1_data->spinner);

    if (has_updates) {
        if (g_page1_data->update_check_label) {
            gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                               "¡Hay una actualización disponible!");
            gtk_widget_remove_css_class(g_page1_data->update_check_label, "dim-label");
            gtk_widget_add_css_class(g_page1_data->update_check_label, "warning");
            gtk_widget_set_visible(g_page1_data->update_check_label, TRUE);
        }

        if (root && GTK_IS_WINDOW(root)) {
            AdwAlertDialog *dlg = ADW_ALERT_DIALOG(
                adw_alert_dialog_new("Actualización Disponible",
                                     "Se encontraron nuevas actualizaciones de Arcris.\n"
                                     "¿Desea actualizar ahora?"));
            adw_alert_dialog_add_responses(dlg,
                "cancel", "Cancelar",
                "update", "Actualizar",
                NULL);
            adw_alert_dialog_set_response_appearance(dlg, "update",
                                                     ADW_RESPONSE_SUGGESTED);
            g_signal_connect(dlg, "response",
                             G_CALLBACK(on_update_run_response), NULL);
            adw_dialog_present(ADW_DIALOG(dlg), GTK_WIDGET(root));
        }
    } else {
        if (g_page1_data->update_check_label) {
            gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                               "Arcris está al día ✓");
            gtk_widget_remove_css_class(g_page1_data->update_check_label, "dim-label");
            gtk_widget_add_css_class(g_page1_data->update_check_label, "success");
            gtk_widget_set_visible(g_page1_data->update_check_label, TRUE);
        }
        // Restaurar botón Iniciar si hay internet
        if (g_page1_data->start_button && g_page1_data->has_internet) {
            gtk_widget_set_visible(g_page1_data->start_button, TRUE);
            gtk_widget_set_sensitive(g_page1_data->start_button, TRUE);
        }
    }

    g_free(remote_sha);
    g_free(stdout_buf);
    g_object_unref(proc);
}

void page1_start_update_check(void)
{
    if (!g_page1_data) return;

    // Activar modo actualización: bloquea update_internet_ui
    g_page1_data->is_update_mode = TRUE;

    // Preparar UI para modo búsqueda
    if (g_page1_data->internet_label)
        gtk_widget_set_visible(g_page1_data->internet_label, FALSE);
    if (g_page1_data->no_internet_label)
        gtk_widget_set_visible(g_page1_data->no_internet_label, FALSE);
    if (g_page1_data->start_button)
        gtk_widget_set_visible(g_page1_data->start_button, FALSE);
    if (g_page1_data->update_check_label) {
        gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                           "Buscando actualizaciones desde repositorio del proyecto...");
        gtk_widget_remove_css_class(g_page1_data->update_check_label, "error");
        gtk_widget_remove_css_class(g_page1_data->update_check_label, "warning");
        gtk_widget_remove_css_class(g_page1_data->update_check_label, "success");
        gtk_widget_add_css_class(g_page1_data->update_check_label, "dim-label");
        gtk_widget_set_visible(g_page1_data->update_check_label, TRUE);
    }
    if (g_page1_data->spinner)
        gtk_widget_set_visible(g_page1_data->spinner, TRUE);

    // Lanzar git ls-remote de forma asíncrona
    GError *error = NULL;
    GSubprocess *proc = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE,
        &error,
        "git", "ls-remote", ARCRIS_REMOTE_URL, "HEAD",
        NULL);

    if (!proc) {
        g_warning("No se pudo lanzar git ls-remote: %s",
                  error ? error->message : "desconocido");
        if (error) g_error_free(error);
        if (g_page1_data->spinner)
            gtk_widget_set_visible(g_page1_data->spinner, FALSE);
        if (g_page1_data->update_check_label) {
            gtk_label_set_text(GTK_LABEL(g_page1_data->update_check_label),
                               "Error: git no encontrado en el sistema");
            gtk_widget_remove_css_class(g_page1_data->update_check_label, "dim-label");
            gtk_widget_add_css_class(g_page1_data->update_check_label, "error");
        }
        return;
    }

    g_subprocess_communicate_utf8_async(proc, NULL, NULL, on_update_check_done, NULL);
}

// ── Fin Actualización ───────────────────────────────────────────────────────

// Función de limpieza de recursos
void page1_cleanup(Page1Data *data)
{
    g_print("🧹 Limpiando recursos de página 1...\n");
    
    // Detener monitoreo de internet
    page1_stop_internet_monitoring();
    
    // Liberar memoria
    if (g_page1_data) {
        g_free(g_page1_data);
        g_page1_data = NULL;
    }
    
    g_print("✅ Limpieza de página 1 completada\n");
}