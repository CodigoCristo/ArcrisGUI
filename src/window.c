#include "window.h"
#include "close.h"

// Función para interceptar el cierre de la ventana
gboolean on_window_close_request(GtkWidget *window, gpointer user_data)
{
    GtkApplication *app = GTK_APPLICATION(user_data);

    // Crear el diálogo
    AdwDialog *dialog = adw_alert_dialog_new(("¿Cerrar aplicación?"), NULL);
    adw_alert_dialog_format_body(ADW_ALERT_DIALOG(dialog), ("¿Estás seguro de que quieres cerrar la aplicación?"));
    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog), "cancel", ("Cancelar"), "close", ("Cerrar"), NULL);
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "cancel");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");
    adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "close", ADW_RESPONSE_DESTRUCTIVE);

    // Conectar la señal de respuesta
    g_signal_connect(dialog, "response", G_CALLBACK(on_close_response), app);

    // Mostrar el diálogo
    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));

    // Detener la propagación del evento (para evitar que la ventana se cierre de inmediato)
    return TRUE;
}

