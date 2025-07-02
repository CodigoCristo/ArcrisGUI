#ifndef INTERNET_H
#define INTERNET_H

#include <adwaita.h>
#include <libsoup/soup.h>
#include <string.h>
#include <glib.h>

// Estructura global para almacenar datos
typedef struct {
    char *language;
    char *timezone;
    char *locale;
} CharPage1;

// Estructura para pasar parámetros a la función de verificación
typedef struct {
    GtkWidget *internet_label;
    GtkWidget *spinner;
    GtkWidget *no_internet_label;
    GtkWidget *start_button;
} InternetWidgets;

// Note: ComboRow variables are now encapsulated within page2.c module

// Declaraciones de funciones
gchar* read_output_from_stream(GInputStream *stream, GError **error);
// Note: Functions for internet connection checking moved to page1.c
// Note: Functions for ComboRow handling and time updates moved to page2.c
// Note: API functions (get_*_from_api) moved to page2.c as static functions


#endif
