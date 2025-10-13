#include "page1.h"
#include "page2.h"
#include <stdlib.h>
#include <unistd.h>

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
        
        // Mostrar y habilitar botón
        if (g_page1_data->start_button) {
            gtk_widget_set_visible(g_page1_data->start_button, TRUE);
            gtk_widget_set_sensitive(g_page1_data->start_button, TRUE);
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
    if (g_page1_data && g_page1_data->internet_monitor_id > 0) {
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
    g_page1_data->start_button = GTK_WIDGET(gtk_builder_get_object(page_builder, "start_button"));
    
    // Verificar que se cargaron todos los widgets correctamente
    if (!g_page1_data->internet_label || !g_page1_data->spinner || 
        !g_page1_data->no_internet_label || !g_page1_data->start_button) {
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
    
    // Iniciar monitoreo de internet después de 1 segundo (dar tiempo a la UI)
    g_timeout_add_seconds(1, page1_start_internet_monitoring_callback, NULL);
    
    g_print("✅ Página 1 inicializada correctamente\n");
}

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