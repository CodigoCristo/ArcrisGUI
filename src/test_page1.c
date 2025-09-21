#include "page1.h"
#include "config.h"
#include <stdlib.h>
#include <unistd.h>

// Variable global para datos de la página 1 (versión de prueba)
static Page1Data *g_test_page1_data = NULL;

// Función de prueba que habilita inmediatamente el botón
gboolean test_enable_start_button(gpointer user_data)
{
    Page1Data *data = (Page1Data *)user_data;
    
    if (!data) return FALSE;
    
    g_print("🧪 Modo de prueba: habilitando botón inmediatamente\n");
    
    // Ocultar spinner
    gtk_widget_set_visible(data->spinner, FALSE);
    
    // Ocultar labels y mostrar solo el botón (simular conexión exitosa)
    gtk_widget_set_visible(data->internet_label, FALSE);
    gtk_widget_set_visible(data->no_internet_label, FALSE);
    
    // Mostrar y habilitar botón inmediatamente
    gtk_widget_set_visible(data->start_button, TRUE);
    gtk_widget_set_sensitive(data->start_button, TRUE);
    
    return FALSE; // No repetir
}

static void test_check_internet_connection(GtkWidget *internet_label, GtkWidget *spinner, 
                                         GtkWidget *no_internet_label, GtkWidget *start_button)
{
    if (!g_test_page1_data) return;
    
    // Actualizar referencias en la estructura
    g_test_page1_data->internet_label = internet_label;
    g_test_page1_data->spinner = spinner;
    g_test_page1_data->no_internet_label = no_internet_label;
    g_test_page1_data->start_button = start_button;
    
    // Configurar widgets iniciales
    gtk_label_set_text(GTK_LABEL(internet_label), "Probando conexión a internet");
    gtk_widget_set_visible(internet_label, TRUE);
    gtk_widget_set_visible(spinner, TRUE);
    gtk_widget_set_visible(no_internet_label, FALSE);
    gtk_widget_set_visible(start_button, FALSE);
    
    g_print("🧪 MODO PRUEBA: Iniciando verificación simulada...\n");
    
    // Habilitar inmediatamente después de 0.5 segundos (solo para mostrar el spinner brevemente)
    g_timeout_add(500, test_enable_start_button, g_test_page1_data);
}

static void test_start_button_clicked(GtkButton *button, gpointer user_data)
{
    if (!g_test_page1_data) return;
    
    AdwCarousel *carousel = g_test_page1_data->carousel;
    GtkRevealer *revealer = g_test_page1_data->revealer;
    
    g_print("🧪 MODO PRUEBA: Botón de inicio presionado\n");
    
    // Mostrar los controles de navegación
    gtk_revealer_set_reveal_child(revealer, TRUE);
    
    // Mover a la siguiente página del carousel
    guint current_page = adw_carousel_get_position(carousel);
    guint total_pages = adw_carousel_get_n_pages(carousel);
    
    if (current_page + 1 < total_pages) {
        GtkWidget *next_page = adw_carousel_get_nth_page(carousel, current_page + 1);
        adw_carousel_scroll_to(carousel, next_page, CAROUSEL_ANIMATION_DURATION);
        g_print("🧪 Navegando a la página %u\n", current_page + 1);
    }
}

void test_page1_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    g_print("🧪 INICIANDO MODO DE PRUEBA PARA PÁGINA 1\n");
    
    // Allocar memoria para los datos de la página
    g_test_page1_data = g_malloc0(sizeof(Page1Data));
    
    // Guardar referencias importantes
    g_test_page1_data->carousel = carousel;
    g_test_page1_data->revealer = revealer;
    
    // Cargar la página 1 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page1.ui");
    GtkWidget *page1 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page1"));
    
    // Obtener widgets específicos de la página
    g_test_page1_data->internet_label = GTK_WIDGET(gtk_builder_get_object(page_builder, "internet"));
    g_test_page1_data->spinner = GTK_WIDGET(gtk_builder_get_object(page_builder, "spinner"));
    g_test_page1_data->no_internet_label = GTK_WIDGET(gtk_builder_get_object(page_builder, "no_internet"));
    g_test_page1_data->start_button = GTK_WIDGET(gtk_builder_get_object(page_builder, "start_button"));
    
    // Verificar que los widgets se cargaron correctamente
    if (!g_test_page1_data->internet_label || !g_test_page1_data->spinner || 
        !g_test_page1_data->no_internet_label || !g_test_page1_data->start_button) {
        g_print("🚨 ERROR: No se pudieron cargar todos los widgets de page1\n");
        if (!g_test_page1_data->internet_label) g_print("   - internet_label es NULL\n");
        if (!g_test_page1_data->spinner) g_print("   - spinner es NULL\n");
        if (!g_test_page1_data->no_internet_label) g_print("   - no_internet_label es NULL\n");
        if (!g_test_page1_data->start_button) g_print("   - start_button es NULL\n");
    } else {
        g_print("✅ Todos los widgets de page1 cargados correctamente\n");
    }
    
    // Realizar "verificación" de conexión a Internet (modo prueba)
    test_check_internet_connection(g_test_page1_data->internet_label, g_test_page1_data->spinner, 
                                  g_test_page1_data->no_internet_label, g_test_page1_data->start_button);
    
    // Conectar señales del botón de inicio
    g_signal_connect(g_test_page1_data->start_button, "clicked", G_CALLBACK(test_start_button_clicked), NULL);
    
    // Añadir la página al carousel
    adw_carousel_append(carousel, page1);
    
    // Liberar el builder de la página
    g_object_unref(page_builder);
    
    g_print("🧪 Página 1 (MODO PRUEBA) inicializada correctamente\n");
}

void test_page1_cleanup(Page1Data *data)
{
    if (g_test_page1_data) {
        g_print("🧪 Limpiando página 1 (modo prueba)\n");
        g_free(g_test_page1_data);
        g_test_page1_data = NULL;
    }
}