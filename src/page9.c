#include "page9.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <gio/gio.h>

// Variable global para datos de la página 9
static Page9Data *g_page9_data = NULL;

// Forward declarations for static functions
static void on_restart_dialog_response(AdwAlertDialog *dialog, GAsyncResult *result, gpointer user_data);
static void on_exit_dialog_response(AdwAlertDialog *dialog, GAsyncResult *result, gpointer user_data);

// Función principal de inicialización de la página 9
void page9_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page9_data = g_malloc0(sizeof(Page9Data));
    
    // Guardar referencias importantes
    g_page9_data->carousel = carousel;
    g_page9_data->revealer = revealer;
    
    LOG_INFO("=== DEBUG: Inicializando página 9 (finalización) ===");
    LOG_INFO("DEBUG: builder=%p, carousel=%p, revealer=%p", builder, carousel, revealer);
    
    // Crear el builder para esta página específica
    GtkBuilder *page_builder = gtk_builder_new();
    GError *error = NULL;
    
    if (!gtk_builder_add_from_resource(page_builder, "/org/gtk/arcris/page9.ui", &error)) {
        LOG_ERROR("Error cargando page9.ui: %s", error->message);
        g_error_free(error);
        g_object_unref(page_builder);
        return;
    }
    
    // Obtener el widget principal
    g_page9_data->main_content = GTK_WIDGET(gtk_builder_get_object(page_builder, "main_bin"));
    LOG_INFO("DEBUG: main_content obtenido = %p", g_page9_data->main_content);
    if (!g_page9_data->main_content) {
        LOG_ERROR("No se pudo obtener main_bin de page9.ui");
        g_object_unref(page_builder);
        return;
    }
    
    LOG_INFO("DEBUG: Obteniendo widgets principales...");
    
    // Obtener widgets de la interfaz
    g_page9_data->success_icon = GTK_IMAGE(gtk_builder_get_object(page_builder, "success_icon"));
    g_page9_data->completion_message = GTK_LABEL(gtk_builder_get_object(page_builder, "completion_message"));
    g_page9_data->secondary_message = GTK_LABEL(gtk_builder_get_object(page_builder, "secondary_message"));
    g_page9_data->info_label = GTK_LABEL(gtk_builder_get_object(page_builder, "info_label"));
    
    // Obtener botones
    g_page9_data->restart_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "restart_button"));
    g_page9_data->exit_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "exit_button"));
    
    LOG_INFO("DEBUG: Widgets obtenidos - success_icon=%p, completion_message=%p, restart_button=%p, exit_button=%p",
             g_page9_data->success_icon, g_page9_data->completion_message, 
             g_page9_data->restart_button, g_page9_data->exit_button);
    
    // Verificar que se obtuvieron todos los widgets necesarios
    if (!g_page9_data->success_icon || !g_page9_data->completion_message ||
        !g_page9_data->secondary_message || !g_page9_data->info_label ||
        !g_page9_data->restart_button || !g_page9_data->exit_button) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de page9.ui");
        LOG_ERROR("DEBUG: success_icon=%p, completion_message=%p, restart_button=%p, exit_button=%p",
                  g_page9_data->success_icon, g_page9_data->completion_message,
                  g_page9_data->restart_button, g_page9_data->exit_button);
        g_object_unref(page_builder);
        return;
    }
    
    // Configurar widgets
    page9_setup_widgets(g_page9_data);
    page9_setup_styles(g_page9_data);
    
    // Agregar la página al carousel si está disponible
    if (carousel) {
        LOG_INFO("DEBUG: Carousel disponible, agregando page9...");
        LOG_INFO("DEBUG: main_content = %p", g_page9_data->main_content);
        
        guint pages_before = adw_carousel_get_n_pages(carousel);
        LOG_INFO("DEBUG: Páginas en carousel antes de agregar page9: %u", pages_before);
        
        adw_carousel_append(carousel, g_page9_data->main_content);
        
        guint pages_after = adw_carousel_get_n_pages(carousel);
        LOG_INFO("DEBUG: Páginas en carousel después de agregar page9: %u", pages_after);
        LOG_INFO("Página 9 agregada al carousel correctamente");
    } else {
        LOG_ERROR("DEBUG: Carousel es NULL - no se puede agregar page9");
    }
    
    // Conectar señales
    LOG_INFO("DEBUG: Conectando señales...");
    g_signal_connect(g_page9_data->restart_button, "clicked", 
                     G_CALLBACK(on_restart_button_clicked), g_page9_data);
    g_signal_connect(g_page9_data->exit_button, "clicked", 
                     G_CALLBACK(on_exit_button_clicked), g_page9_data);
    
    // Marcar como inicializada
    g_page9_data->is_initialized = TRUE;
    g_page9_data->show_completion_animation = TRUE;
    
    LOG_INFO("Página 9 inicializada correctamente");
    
    // Limpiar
    g_object_unref(page_builder);
}

// Función de limpieza
void page9_cleanup(void)
{
    if (g_page9_data) {
        LOG_INFO("Limpiando datos de página 9");
        g_free(g_page9_data);
        g_page9_data = NULL;
    }
}

// Obtener datos de la página
Page9Data* page9_get_data(void)
{
    return g_page9_data;
}

// Configurar widgets
void page9_setup_widgets(Page9Data *data)
{
    if (!data) return;
    
    LOG_INFO("Configurando widgets de página 9");
    
    // Configurar el icono de éxito con color verde
    if (data->success_icon) {
        gtk_widget_add_css_class(GTK_WIDGET(data->success_icon), "success-icon");
    }
    
    // Configurar mensajes
    if (data->completion_message) {
        gtk_widget_add_css_class(GTK_WIDGET(data->completion_message), "success-text");
    }
    
    // Configurar botones con estilos específicos
    if (data->restart_button) {
        gtk_widget_add_css_class(GTK_WIDGET(data->restart_button), "restart-button");
        gtk_widget_add_css_class(GTK_WIDGET(data->restart_button), "destructive-action");
    }
    
    if (data->exit_button) {
        gtk_widget_add_css_class(GTK_WIDGET(data->exit_button), "exit-button");
        gtk_widget_add_css_class(GTK_WIDGET(data->exit_button), "suggested-action");
    }
    
    LOG_INFO("Widgets configurados correctamente");
}

// Configurar estilos CSS
void page9_setup_styles(Page9Data *data)
{
    if (!data) return;
    
    LOG_INFO("Configurando estilos CSS para página 9");
    
    // CSS personalizado para los estilos específicos
    const gchar *css_data = 
        ".success-icon { "
        "  color: #26a269; "
        "  -gtk-icon-shadow: 0 1px 3px rgba(0,0,0,0.2); "
        "} "
        ".success-text { "
        "  color: #26a269; "
        "  font-weight: bold; "
        "} "
        ".restart-button { "
        "  background: linear-gradient(to bottom, #e01b24, #c01c28); "
        "  color: white; "
        "  border: 1px solid #a51d2d; "
        "  font-weight: bold; "
        "} "
        ".restart-button:hover { "
        "  background: linear-gradient(to bottom, #ed333b, #e01b24); "
        "} "
        ".exit-button { "
        "  background: linear-gradient(to bottom, #3584e4, #1c71d8); "
        "  color: white; "
        "  border: 1px solid #1a5fb4; "
        "  font-weight: bold; "
        "} "
        ".exit-button:hover { "
        "  background: linear-gradient(to bottom, #62a0ea, #3584e4); "
        "}";
    
    // Aplicar CSS
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider, css_data);
    
    GdkDisplay *display = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(display, 
                                                GTK_STYLE_PROVIDER(css_provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(css_provider);
    
    LOG_INFO("Estilos CSS aplicados");
}

// Cargar datos de la página
void page9_load_data(Page9Data *data)
{
    if (!data) return;
    
    LOG_INFO("Cargando datos para página 9");
    
    // Aquí se pueden cargar datos específicos si es necesario
    // Por ahora, los textos están definidos en el UI
    
    LOG_INFO("Datos cargados para página 9");
}

// Mostrar animación de finalización
void page9_show_completion_animation(Page9Data *data)
{
    if (!data || !data->show_completion_animation) return;
    
    LOG_INFO("Mostrando animación de finalización");
    
    // Implementar animación de fade-in o efectos visuales
    page9_fade_in_elements(data);
    
    data->show_completion_animation = FALSE; // Solo mostrar una vez
}

// Animación de fade-in para elementos
void page9_fade_in_elements(Page9Data *data)
{
    if (!data) return;
    
    LOG_INFO("Aplicando efectos de fade-in");
    
    // Ocultar inicialmente y luego mostrar con animación
    if (data->success_icon) {
        gtk_widget_set_opacity(GTK_WIDGET(data->success_icon), 0.0);
        // Aquí se podría agregar una animación con GtkTimout si es necesario
        gtk_widget_set_opacity(GTK_WIDGET(data->success_icon), 1.0);
    }
}

// Callback del botón reiniciar
void on_restart_button_clicked(GtkButton *button, gpointer user_data)
{
    LOG_INFO("Botón 'Reiniciar' presionado");
    
    if (!page9_can_restart()) {
        LOG_WARNING("No se puede reiniciar en este momento");
        return;
    }
    
    // Mostrar diálogo de confirmación usando AdwAlertDialog
    AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
        "¿Está seguro que desea reiniciar el sistema ahora?",
        "El sistema se reiniciará inmediatamente. Asegúrese de haber guardado todo su trabajo."
    ));
    
    adw_alert_dialog_add_response(dialog, "cancel", "Cancelar");
    adw_alert_dialog_add_response(dialog, "restart", "Reiniciar");
    adw_alert_dialog_set_response_appearance(dialog, "restart", ADW_RESPONSE_DESTRUCTIVE);
    
    // Obtener la ventana principal para mostrar el diálogo
    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(g_page9_data->main_content)));
    if (parent_window) {
        adw_alert_dialog_choose(dialog, GTK_WIDGET(parent_window), NULL, 
                               (GAsyncReadyCallback)on_restart_dialog_response, NULL);
    } else {
        adw_alert_dialog_choose(dialog, NULL, NULL, 
                               (GAsyncReadyCallback)on_restart_dialog_response, NULL);
    }
}

// Callback del botón salir
void on_exit_button_clicked(GtkButton *button, gpointer user_data)
{
    LOG_INFO("Botón 'Salir' presionado");
    
    if (!page9_can_exit()) {
        LOG_WARNING("No se puede salir en este momento");
        return;
    }
    
    // Mostrar diálogo de confirmación usando AdwAlertDialog
    AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
        "¿Está seguro que desea salir del instalador?",
        "Podrá reiniciar el sistema manualmente más tarde para usar Arch Linux."
    ));
    
    adw_alert_dialog_add_response(dialog, "cancel", "Cancelar");
    adw_alert_dialog_add_response(dialog, "exit", "Salir");
    adw_alert_dialog_set_response_appearance(dialog, "exit", ADW_RESPONSE_SUGGESTED);
    
    // Obtener la ventana principal para mostrar el diálogo
    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(g_page9_data->main_content)));
    if (parent_window) {
        adw_alert_dialog_choose(dialog, GTK_WIDGET(parent_window), NULL, 
                               (GAsyncReadyCallback)on_exit_dialog_response, NULL);
    } else {
        adw_alert_dialog_choose(dialog, NULL, NULL, 
                               (GAsyncReadyCallback)on_exit_dialog_response, NULL);
    }
}

// Función llamada cuando se muestra la página 9
void page9_on_page_shown(void)
{
    LOG_INFO("Página 9 mostrada - Instalación completada");
    
    Page9Data *data = page9_get_data();
    if (data) {
        // Ocultar revealer en página de finalización
        if (data->revealer) {
            gtk_revealer_set_reveal_child(data->revealer, FALSE);
            LOG_INFO("Revealer ocultado en página de finalización");
        }
        
        // Cargar datos si es necesario
        page9_load_data(data);
        
        // Mostrar animación de finalización
        page9_show_completion_animation(data);
        
        LOG_INFO("Página de finalización lista - mostrando opciones al usuario");
    }
}

// Función llamada cuando se oculta la página 9
void page9_on_page_hidden(void)
{
    LOG_INFO("Página 9 ocultada");
    
    // Limpiar cualquier timer o animación si es necesario
}

// Ejecutar reinicio del sistema
void page9_execute_restart(void)
{
    LOG_INFO("Ejecutando reinicio del sistema...");
    
    // Sincronizar discos antes del reinicio
    LOG_INFO("Sincronizando discos...");
    system("sync");
    sleep(1);
    
    // Intentar varios métodos de reinicio
    LOG_INFO("Intentando reinicio con systemctl...");
    int result = system("systemctl reboot");
    
    if (result != 0) {
        LOG_WARNING("systemctl reboot falló, intentando con reboot...");
        result = system("reboot");
        
        if (result != 0) {
            LOG_WARNING("reboot falló, intentando con shutdown...");
            result = system("shutdown -r now");
            
            if (result != 0) {
                LOG_ERROR("Todos los métodos de reinicio fallaron");
                
                // Mostrar diálogo de error usando AdwAlertDialog
                AdwAlertDialog *error_dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
                    "Error al reiniciar el sistema",
                    "No se pudo ejecutar el comando de reinicio automático.\n\n"
                    "Por favor:\n"
                    "1. Cierre todas las aplicaciones\n"
                    "2. Abra una terminal como root\n"
                    "3. Ejecute: systemctl reboot\n\n"
                    "O use el botón de reinicio físico del equipo."
                ));
                
                adw_alert_dialog_add_response(error_dialog, "ok", "Entendido");
                adw_alert_dialog_set_response_appearance(error_dialog, "ok", ADW_RESPONSE_SUGGESTED);
                
                // Obtener la ventana principal para mostrar el diálogo
                GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(g_page9_data->main_content)));
                if (parent_window) {
                    adw_alert_dialog_choose(error_dialog, GTK_WIDGET(parent_window), NULL, NULL, NULL);
                } else {
                    adw_alert_dialog_choose(error_dialog, NULL, NULL, NULL, NULL);
                }
                return;
            }
        }
    }
    
    LOG_INFO("Comando de reinicio ejecutado exitosamente");
}

// Ejecutar salida del instalador
void page9_execute_exit(void)
{
    LOG_INFO("Saliendo del instalador...");
    
    // Sincronizar discos para asegurar que todos los datos estén escritos
    LOG_INFO("Sincronizando discos antes de salir...");
    system("sync");
    
    // Limpiar recursos de la página 9
    LOG_INFO("Limpiando recursos de página 9...");
    
    // Guardar cualquier configuración pendiente si es necesario
    // (esto podría expandirse en el futuro)
    
    // Obtener la aplicación actual
    GApplication *app = g_application_get_default();
    
    if (app) {
        LOG_INFO("Cerrando aplicación correctamente...");
        
        // Limpiar recursos globales
        page9_cleanup();
        
        // Intentar cerrar la aplicación de forma elegante
        if (G_IS_APPLICATION(app)) {
            // Usar g_timeout_add para permitir que el diálogo se cierre primero
            g_timeout_add(100, (GSourceFunc)g_application_quit, app);
        } else {
            LOG_WARNING("No se pudo obtener la aplicación, forzando salida...");
            exit(0);
        }
    } else {
        LOG_WARNING("No se pudo obtener la aplicación actual, forzando salida...");
        
        // Limpiar recursos
        page9_cleanup();
        
        // Salida forzada si no hay aplicación disponible
        exit(0);
    }
    
    LOG_INFO("Secuencia de salida iniciada");
}

// Verificar si se puede reiniciar
gboolean page9_can_restart(void)
{
    LOG_INFO("Verificando si se puede reiniciar el sistema...");
    
    // Verificar que no haya procesos críticos ejecutándose
    if (page9_check_critical_processes()) {
        LOG_WARNING("Hay procesos críticos ejecutándose, no se puede reiniciar");
        return FALSE;
    }
    
    // Verificar que tengamos permisos de root
    if (!page9_check_root_permissions()) {
        LOG_WARNING("No se tienen permisos de root para reiniciar");
        return FALSE;
    }
    
    // Verificar que estemos en un Live CD o sistema instalado
    if (!page9_check_system_state()) {
        LOG_WARNING("Estado del sistema no válido para reinicio");
        return FALSE;
    }
    
    LOG_INFO("Sistema listo para reiniciar");
    return TRUE;
}

// Verificar si se puede salir
gboolean page9_can_exit(void)
{
    LOG_INFO("Verificando si se puede salir del instalador...");
    
    // Verificar que no haya instalaciones en curso
    if (page9_check_installation_in_progress()) {
        LOG_WARNING("Hay una instalación en progreso, no se puede salir");
        return FALSE;
    }
    
    // Verificar que no haya procesos críticos del instalador
    if (page9_check_installer_processes()) {
        LOG_WARNING("Hay procesos críticos del instalador ejecutándose");
        return FALSE;
    }
    
    LOG_INFO("Instalador listo para salir");
    return TRUE;
}

// Callback para respuesta del diálogo de reinicio
static void on_restart_dialog_response(AdwAlertDialog *dialog, GAsyncResult *result, gpointer user_data)
{
    const char *response = adw_alert_dialog_choose_finish(dialog, result);
    
    if (g_strcmp0(response, "restart") == 0) {
        LOG_INFO("Usuario confirmó reinicio del sistema");
        page9_execute_restart();
    } else {
        LOG_INFO("Usuario canceló el reinicio");
    }
}

// Funciones auxiliares para verificaciones del sistema

// Verificar procesos críticos del sistema
gboolean page9_check_critical_processes(void)
{
    LOG_INFO("Verificando procesos críticos del sistema...");
    
    // Lista de procesos que no deberían estar ejecutándose durante reinicio
    const char *critical_processes[] = {
        "pacman",
        "pacstrap", 
        "mkfs",
        "parted",
        "gparted",
        NULL
    };
    
    for (int i = 0; critical_processes[i] != NULL; i++) {
        char command[256];
        snprintf(command, sizeof(command), "pgrep %s > /dev/null 2>&1", critical_processes[i]);
        
        if (system(command) == 0) {
            LOG_WARNING("Proceso crítico encontrado: %s", critical_processes[i]);
            return TRUE; // Hay procesos críticos
        }
    }
    
    LOG_INFO("No se encontraron procesos críticos");
    return FALSE;
}

// Verificar permisos de root
gboolean page9_check_root_permissions(void)
{
    LOG_INFO("Verificando permisos de root...");
    
    uid_t uid = getuid();
    uid_t euid = geteuid();
    
    if (uid == 0 || euid == 0) {
        LOG_INFO("Permisos de root confirmados");
        return TRUE;
    }
    
    // Verificar si podemos usar sudo
    int result = system("sudo -n true > /dev/null 2>&1");
    if (result == 0) {
        LOG_INFO("Permisos sudo disponibles");
        return TRUE;
    }
    
    LOG_WARNING("No se tienen permisos de root ni sudo");
    return FALSE;
}

// Verificar estado del sistema
gboolean page9_check_system_state(void)
{
    LOG_INFO("Verificando estado del sistema...");
    
    // Verificar si estamos en un Live CD
    if (access("/run/archiso", F_OK) == 0) {
        LOG_INFO("Ejecutándose desde Live CD de Arch Linux");
        return TRUE;
    }
    
    // Verificar si el sistema está instalado
    if (access("/etc/arch-release", F_OK) == 0) {
        LOG_INFO("Ejecutándose en sistema Arch Linux instalado");
        return TRUE;
    }
    
    // Verificar systemd
    if (access("/run/systemd/system", F_OK) == 0) {
        LOG_INFO("Sistema con systemd detectado");
        return TRUE;
    }
    
    LOG_WARNING("No se pudo determinar el estado del sistema");
    return FALSE;
}

// Verificar instalación en progreso
gboolean page9_check_installation_in_progress(void)
{
    LOG_INFO("Verificando si hay instalación en progreso...");
    
    // Verificar si hay montajes activos en /mnt
    FILE *mounts = fopen("/proc/mounts", "r");
    if (mounts) {
        char line[1024];
        while (fgets(line, sizeof(line), mounts)) {
            if (strstr(line, "/mnt") != NULL && strstr(line, "tmpfs") == NULL) {
                LOG_INFO("Encontrado montaje activo en /mnt: instalación posiblemente en progreso");
                fclose(mounts);
                return TRUE;
            }
        }
        fclose(mounts);
    }
    
    // Verificar archivos temporales de instalación
    if (access("/tmp/arcris_installation_lock", F_OK) == 0) {
        LOG_INFO("Archivo de bloqueo de instalación encontrado");
        return TRUE;
    }
    
    LOG_INFO("No se detectó instalación en progreso");
    return FALSE;
}

// Verificar procesos del instalador
gboolean page9_check_installer_processes(void)
{
    LOG_INFO("Verificando procesos del instalador...");
    
    // Lista de procesos del instalador que deberían terminar antes de salir
    const char *installer_processes[] = {
        "install.sh",
        "arch-chroot",
        "chroot",
        NULL
    };
    
    for (int i = 0; installer_processes[i] != NULL; i++) {
        char command[256];
        snprintf(command, sizeof(command), "pgrep -f %s > /dev/null 2>&1", installer_processes[i]);
        
        if (system(command) == 0) {
            LOG_WARNING("Proceso del instalador encontrado: %s", installer_processes[i]);
            return TRUE; // Hay procesos del instalador
        }
    }
    
    LOG_INFO("No se encontraron procesos críticos del instalador");
    return FALSE;
}

// Crear archivo de bloqueo de instalación
void page9_create_installation_lock(void)
{
    LOG_INFO("Creando archivo de bloqueo de instalación...");
    
    FILE *lock_file = fopen("/tmp/arcris_installation_lock", "w");
    if (lock_file) {
        fprintf(lock_file, "ARCRIS Installation in progress\n");
        fprintf(lock_file, "Started: %ld\n", time(NULL));
        fclose(lock_file);
        LOG_INFO("Archivo de bloqueo creado exitosamente");
    } else {
        LOG_WARNING("No se pudo crear archivo de bloqueo");
    }
}

// Remover archivo de bloqueo de instalación
void page9_remove_installation_lock(void)
{
    LOG_INFO("Removiendo archivo de bloqueo de instalación...");
    
    if (unlink("/tmp/arcris_installation_lock") == 0) {
        LOG_INFO("Archivo de bloqueo removido exitosamente");
    } else {
        LOG_WARNING("No se pudo remover archivo de bloqueo (puede que no exista)");
    }
}

// Callback para respuesta del diálogo de salida
static void on_exit_dialog_response(AdwAlertDialog *dialog, GAsyncResult *result, gpointer user_data)
{
    const char *response = adw_alert_dialog_choose_finish(dialog, result);
    
    if (g_strcmp0(response, "exit") == 0) {
        LOG_INFO("Usuario confirmó salida del instalador");
        page9_execute_exit();
    } else {
        LOG_INFO("Usuario canceló la salida");
    }
}