#ifndef PAGE1_H
#define PAGE1_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Estructura para datos de la página 1
typedef struct _Page1Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    GtkWidget *internet_label;
    GtkWidget *spinner;
    GtkWidget *no_internet_label;
    GtkWidget *update_check_label;
    GtkWidget *start_button;
    guint internet_monitor_id;
    guint internet_monitor_initial_id;  // timer inicial de 1 seg (debe cancelarse en update mode)
    gboolean has_internet;
    gboolean auto_configured;
    gboolean is_update_mode;            // TRUE mientras se busca/instala actualización
} Page1Data;

// Funciones principales de la página 1
void page1_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page1_cleanup(Page1Data *data);

// Funciones de monitoreo de internet
void page1_start_internet_monitoring(void);
void page1_stop_internet_monitoring(void);
gboolean page1_check_internet_status(gpointer user_data);

// Función de búsqueda de actualizaciones
void page1_start_update_check(void);

// Funciones de conexión de internet (privadas - implementadas internamente)
// Las funciones específicas de conexión a internet son privadas del módulo

#endif