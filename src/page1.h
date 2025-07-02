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
    GtkWidget *start_button;
} Page1Data;

// Funciones principales de la página 1
void page1_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page1_cleanup(Page1Data *data);

// Funciones de conexión de internet (privadas - implementadas internamente)
// Las funciones específicas de conexión a internet son privadas del módulo

#endif