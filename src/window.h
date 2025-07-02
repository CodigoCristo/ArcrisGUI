#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Declaración de la función para manejar la solicitud de cierre de la ventana
gboolean on_window_close_request(GtkWidget *window, gpointer user_data);

#endif

