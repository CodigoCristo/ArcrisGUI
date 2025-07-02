#include "page1.h"
#include "config.h"
#include <stdlib.h>
#include <unistd.h>

// Variable global para datos de la página 1
static Page1Data *g_page1_data = NULL;
static guint g_internet_timer_id = 0;

// Función robusta para verificar internet con fallback automático
gboolean robust_internet_check_and_enable(gpointer user_data)
{
    Page1Data *data = (Page1Data *)user_data;
    
    if (!data) {
        g_print("❌ Error: datos de página nulos\n");
        return FALSE;
    }
    
    g_print("🌐 Verificando conexión a Internet...\n");
    
    // Intentar verificación rápida con múltiples métodos
    int result = -1;
    
    // Método 1: ping a 1.1.1.1 (Cloudflare)
    result = system("ping -c 1 -W 1 1.1.1.1 > /dev/null 2>&1");
    if (result != 0) {
        // Método 2: ping a 8.8.8.8 (Google)
        result = system("ping -c 1 -W 1 8.8.8.8 > /dev/null 2>&1");
    }
    
    // Limpiar timer
    g_internet_timer_id = 0;
    
    // Ocultar spinner siempre
    if (data->spinner) {
        gtk_widget_set_visible(data->spinner, FALSE);
    }
    
    if (result == 0) {
        // Conexión exitosa - ocultar labels y mostrar botón
        if (data->internet_label) {
            gtk_widget_set_visible(data->internet_label, FALSE);
        }
        if (data->no_internet_label) {
            gtk_widget_set_visible(data->no_internet_label, FALSE);
        }
        if (data->start_button) {
            gtk_widget_set_visible(data->start_button, TRUE);
            gtk_widget_set_sensitive(data->start_button, TRUE);
        }
        g_print("✅ Conexión a Internet establecida\n");
    } else {
        // Sin conexión - mostrar mensaje de error
        if (data->internet_label) {
            gtk_widget_set_visible(data->internet_label, FALSE);
        }
        if (data->no_internet_label) {
            gtk_label_set_text(GTK_LABEL(data->no_internet_label), "¡Conéctese primero a Internet!");
            gtk_widget_set_visible(data->no_internet_label, TRUE);
        }
        if (data->start_button) {
            gtk_widget_set_visible(data->start_button, FALSE);
        }
        g_print("⚠ Sin conexión a Internet\n");
    }
    
    return FALSE; // No repetir
}

// Función de fallback que garantiza que el spinner se oculte
gboolean fallback_enable_button(gpointer user_data)
{
    Page1Data *data = (Page1Data *)user_data;
    
    if (!data) return FALSE;
    
    g_print("🔄 Fallback: Verificación final\n");
    
    // Ocultar spinner siempre
    if (data->spinner) {
        gtk_widget_set_visible(data->spinner, FALSE);
    }
    
    // Hacer una verificación final de internet
    int result = system("ping -c 1 -W 1 1.1.1.1 > /dev/null 2>&1");
    if (result != 0) {
        result = system("ping -c 1 -W 1 8.8.8.8 > /dev/null 2>&1");
    }
    
    if (result == 0) {
        // Hay internet - mostrar botón
        if (data->internet_label) {
            gtk_widget_set_visible(data->internet_label, FALSE);
        }
        if (data->no_internet_label) {
            gtk_widget_set_visible(data->no_internet_label, FALSE);
        }
        if (data->start_button) {
            gtk_widget_set_visible(data->start_button, TRUE);
        }
    } else {
        // No hay internet - mostrar mensaje de error
        if (data->internet_label) {
            gtk_widget_set_visible(data->internet_label, FALSE);
        }
        if (data->no_internet_label) {
            gtk_label_set_text(GTK_LABEL(data->no_internet_label), "¡Conéctese primero a Internet!");
            gtk_widget_set_visible(data->no_internet_label, TRUE);
        }
        if (data->start_button) {
            gtk_widget_set_visible(data->start_button, FALSE);
        }
    }
    
    return FALSE;
}

static void page1_check_internet_connection(GtkWidget *internet_label, GtkWidget *spinner, 
                                          GtkWidget *no_internet_label, GtkWidget *start_button)
{
    if (!g_page1_data) {
        g_print("❌ Error: datos de página globales nulos\n");
        return;
    }
    
    // Cancelar timer anterior si existe
    if (g_internet_timer_id > 0) {
        g_source_remove(g_internet_timer_id);
        g_internet_timer_id = 0;
    }
    
    // Actualizar referencias en la estructura
    g_page1_data->internet_label = internet_label;
    g_page1_data->spinner = spinner;
    g_page1_data->no_internet_label = no_internet_label;
    g_page1_data->start_button = start_button;
    
    // Verificar que los widgets no sean NULL
    if (!internet_label || !spinner || !no_internet_label || !start_button) {
        g_print("❌ Error: algunos widgets son NULL\n");
        return;
    }
    
    // Configurar widgets iniciales
    gtk_label_set_text(GTK_LABEL(internet_label), "Probando conexión a internet");
    gtk_widget_set_visible(internet_label, TRUE);
    gtk_widget_set_visible(spinner, TRUE);
    gtk_widget_set_visible(no_internet_label, FALSE);
    gtk_widget_set_visible(start_button, FALSE);
    
    g_print("🚀 Iniciando verificación robusta de Internet...\n");
    
    // Programar verificación principal en 1.5 segundos
    g_timeout_add_seconds(2, robust_internet_check_and_enable, g_page1_data);
    
    // Programar fallback de seguridad en 4 segundos (garantiza que siempre se habilite)
    g_timeout_add_seconds(4, fallback_enable_button, g_page1_data);
}

static void page1_start_button_clicked(GtkButton *button, gpointer user_data)
{
    if (!g_page1_data) return;
    
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

void page1_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page1_data = g_malloc0(sizeof(Page1Data));
    
    // Guardar referencias importantes
    g_page1_data->carousel = carousel;
    g_page1_data->revealer = revealer;
    
    // Cargar la página 1 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page1.ui");
    GtkWidget *page1 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page1"));
    
    // Obtener widgets específicos de la página
    g_page1_data->internet_label = GTK_WIDGET(gtk_builder_get_object(page_builder, "internet"));
    g_page1_data->spinner = GTK_WIDGET(gtk_builder_get_object(page_builder, "spinner"));
    g_page1_data->no_internet_label = GTK_WIDGET(gtk_builder_get_object(page_builder, "no_internet"));
    g_page1_data->start_button = GTK_WIDGET(gtk_builder_get_object(page_builder, "start_button"));
    
    // Verificar conexión a Internet
    page1_check_internet_connection(g_page1_data->internet_label, g_page1_data->spinner, 
                                   g_page1_data->no_internet_label, g_page1_data->start_button);
    
    // Conectar señales del botón de inicio
    g_signal_connect(g_page1_data->start_button, "clicked", G_CALLBACK(page1_start_button_clicked), NULL);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page1);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
}

void page1_cleanup(Page1Data *data)
{
    if (g_page1_data) {
        g_free(g_page1_data);
        g_page1_data = NULL;
    }
}