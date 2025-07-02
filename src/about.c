#include "about.h"
#include <adwaita.h>
#include <gtk/gtk.h>

void about_action(GSimpleAction *action, GVariant *parameter, gpointer app)
{
    // Cargar el archivo about.ui
    GtkBuilder *builder_about = gtk_builder_new_from_resource("/org/gtk/arcris/about.ui");


    // Obtener el objeto AdwAboutDialog
    AdwAboutDialog *about_dialog = ADW_ABOUT_DIALOG(gtk_builder_get_object(builder_about, "aboutWindow"));

    // Mostrar el diálogo
    adw_dialog_present(ADW_DIALOG(about_dialog), GTK_WIDGET(gtk_application_get_active_window(GTK_APPLICATION(app))));

    // Liberar el builder
    g_object_unref(builder_about);
}
