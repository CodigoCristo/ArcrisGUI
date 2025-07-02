#include "close.h"
#include <glib.h>

gboolean on_close_response(AdwAlertDialog *dialog, const char *response, AdwApplication *app)
{
    if (g_strcmp0(response, "close") == 0) {
        // El usuario confirmó el cierre
        g_application_quit(G_APPLICATION(app));
        g_print("Finalizo Programa\n");
    }
    return G_SOURCE_REMOVE;
}

