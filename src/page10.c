#include "page10.h"
#include "config.h"
#include "i18n.h"

static Page10Data *g_page10_data = NULL;

void page10_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    (void)builder;

    g_page10_data = g_new0(Page10Data, 1);
    g_page10_data->carousel = carousel;
    g_page10_data->revealer = revealer;

    LOG_INFO("=== Inicializando página 10 (error) ===");

    GtkBuilder *page_builder = gtk_builder_new();
    GError *error = NULL;

    if (!gtk_builder_add_from_resource(page_builder, "/org/gtk/arcris/page10.ui", &error)) {
        LOG_ERROR("Error cargando page10.ui: %s", error ? error->message : "desconocido");
        if (error) g_error_free(error);
        g_object_unref(page_builder);
        return;
    }

    g_page10_data->main_content = GTK_WIDGET(gtk_builder_get_object(page_builder, "main_bin"));
    if (!g_page10_data->main_content) {
        LOG_ERROR("No se pudo obtener main_bin de page10.ui");
        g_object_unref(page_builder);
        return;
    }

    g_page10_data->error_title    = GTK_LABEL(gtk_builder_get_object(page_builder, "error_title"));
    g_page10_data->error_message  = GTK_LABEL(gtk_builder_get_object(page_builder, "error_message"));
    g_page10_data->view_log_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(page_builder, "view_log_button"));
    g_page10_data->log_revealer   = GTK_REVEALER(gtk_builder_get_object(page_builder, "log_revealer"));
    g_page10_data->log_text_view  = GTK_TEXT_VIEW(gtk_builder_get_object(page_builder, "log_text_view"));

    adw_carousel_append(carousel, g_page10_data->main_content);

    if (g_page10_data->view_log_button) {
        g_signal_connect(g_page10_data->view_log_button, "toggled",
                         G_CALLBACK(on_view_log_button_toggled), g_page10_data);
    }

    g_object_unref(page_builder);
    LOG_INFO("Página 10 (error) inicializada correctamente");
}

void page10_load_log(Page10Data *data)
{
    if (!data || !data->log_text_view) return;

    gchar *log_path = g_build_filename(g_get_home_dir(), "install.log", NULL);
    gchar *log_content = NULL;
    GError *error = NULL;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(data->log_text_view);

    if (g_file_get_contents(log_path, &log_content, NULL, &error)) {
        gtk_text_buffer_set_text(buffer, log_content, -1);
        g_free(log_content);
    } else {
        gtk_text_buffer_set_text(buffer, "(No se pudo leer el archivo de registro.)", -1);
        if (error) g_error_free(error);
    }

    g_free(log_path);
}

void on_view_log_button_toggled(GtkToggleButton *button, gpointer user_data)
{
    Page10Data *data = (Page10Data*)user_data;
    if (!data || !data->log_revealer) return;

    gboolean active = gtk_toggle_button_get_active(button);
    gtk_revealer_set_reveal_child(data->log_revealer, active);

    if (active) {
        page10_load_log(data);
    }
}

GtkWidget* page10_get_widget(void)
{
    if (!g_page10_data) return NULL;
    return g_page10_data->main_content;
}

Page10Data* page10_get_data(void)
{
    return g_page10_data;
}

void page10_on_page_shown(void)
{
    LOG_INFO("Página 10 (error) mostrada");
    if (g_page10_data && g_page10_data->revealer) {
        gtk_revealer_set_reveal_child(g_page10_data->revealer, FALSE);
    }
}

void page10_on_page_hidden(void)
{
    LOG_INFO("Página 10 (error) oculta");
}

void page10_update_language(void)
{
    if (!g_page10_data) return;

    if (g_page10_data->error_title)
        gtk_label_set_text(g_page10_data->error_title,
            i18n_t("Error en la instalación"));
    if (g_page10_data->error_message)
        gtk_label_set_text(g_page10_data->error_message,
            i18n_t("La instalación no pudo completarse. Revisa el registro para más información."));
    if (g_page10_data->view_log_button)
        gtk_button_set_label(GTK_BUTTON(g_page10_data->view_log_button),
            i18n_t("Ver registro log"));
}
