#include "window_apps.h"
#include "config.h"
#include <glib/gstdio.h>
#include <string.h>
#include <gio/gio.h>

// Instancia global
static WindowAppsData *global_apps_data = NULL;

// Constantes
#define VARIABLES_FILE_PATH "./data/variables.sh"

WindowAppsData* window_apps_new(void)
{
    if (global_apps_data) {
        return global_apps_data;
    }

    WindowAppsData *data = g_new0(WindowAppsData, 1);
    data->is_initialized = FALSE;
    data->selected_apps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    global_apps_data = data;
    return data;
}

void window_apps_init(WindowAppsData *data)
{
    if (!data || data->is_initialized) return;

    LOG_INFO("Inicializando ventana de utilities apps");

    // Crear builder y cargar UI
    data->builder = gtk_builder_new();
    GError *error = NULL;

    if (!gtk_builder_add_from_resource(data->builder, "/org/gtk/arcris/window_apps.ui", &error)) {
        LOG_ERROR("Error cargando UI de utilities apps: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    }

    // Cargar widgets
    window_apps_load_widgets_from_builder(data);

    // Configurar widgets
    window_apps_setup_widgets(data);

    // Configurar búsqueda
    window_apps_setup_search(data);

    // Conectar señales
    window_apps_connect_signals(data);

    // Conectar botones de información
    window_apps_connect_info_buttons(data);

    // Cargar aplicaciones guardadas
    window_apps_load_selected_apps_from_file(data);

    data->is_initialized = TRUE;
    LOG_INFO("Ventana de utilities apps inicializada correctamente");
}

void window_apps_cleanup(WindowAppsData *data)
{
    if (!data) return;

    LOG_INFO("Limpiando ventana de utilities apps");

    if (data->selected_apps) {
        g_hash_table_destroy(data->selected_apps);
        data->selected_apps = NULL;
    }

    if (data->builder) {
        g_object_unref(data->builder);
        data->builder = NULL;
    }

    if (data->window) {
        gtk_window_destroy(data->window);
        data->window = NULL;
    }

    data->is_initialized = FALSE;
}

void window_apps_show(WindowAppsData *data, GtkWindow *parent)
{
    if (!data) return;

    if (!data->is_initialized) {
        window_apps_init(data);
    }

    if (!data->window) {
        LOG_ERROR("No se pudo mostrar la ventana de utilities apps: ventana no inicializada");
        return;
    }

    // Configurar ventana padre
    if (parent) {
        gtk_window_set_transient_for(data->window, parent);
        gtk_window_set_modal(data->window, TRUE);
    }

    // Cargar aplicaciones seleccionadas actuales
    window_apps_load_selected_apps_from_file(data);

    // Aplicar selecciones a los checkboxes
    window_apps_apply_selections_to_checkboxes(data);

    // Mostrar la ventana
    gtk_window_present(data->window);

    LOG_INFO("Ventana de utilities apps mostrada");
}

void window_apps_hide(WindowAppsData *data)
{
    if (!data || !data->window) return;

    gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
    LOG_INFO("Ventana de utilities apps ocultada");
}

void window_apps_load_widgets_from_builder(WindowAppsData *data)
{
    if (!data || !data->builder) return;

    // Cargar ventana principal
    data->window = GTK_WINDOW(gtk_builder_get_object(data->builder, "ProgramExtraWindow"));
    if (!data->window) {
        LOG_ERROR("No se pudo cargar la ventana principal de utilities apps");
        return;
    }

    // Cargar botones
    data->close_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));

    // Cargar entrada de búsqueda
    data->search_entry = GTK_SEARCH_ENTRY(gtk_builder_get_object(data->builder, "searchApp"));

    // Cargar expanderes
    data->browsers_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "browsers_expander"));
    data->graphics_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "graphics_expander"));
    data->video_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "video_expander"));

    if (!data->close_button) LOG_WARNING("No se pudo cargar close_button");
    if (!data->save_button) LOG_WARNING("No se pudo cargar save_button");
    if (!data->search_entry) LOG_WARNING("No se pudo cargar searchApp");
    if (!data->browsers_expander) LOG_WARNING("No se pudo cargar browsers_expander");
    if (!data->graphics_expander) LOG_WARNING("No se pudo cargar graphics_expander");
    if (!data->video_expander) LOG_WARNING("No se pudo cargar video_expander");

    LOG_INFO("Widgets de ventana de utilities apps cargados desde builder");
}

void window_apps_setup_widgets(WindowAppsData *data)
{
    if (!data) return;

    // Configurar ventana
    if (data->window) {
        gtk_window_set_title(data->window, "Utilities Apps");
        gtk_window_set_default_size(data->window, 900, 630);
        gtk_window_set_resizable(data->window, FALSE);
    }

    LOG_INFO("Widgets de ventana de utilities apps configurados");
}

void window_apps_setup_search(WindowAppsData *data)
{
    if (!data || !data->search_entry) return;

    // Configurar propiedades de la entrada de búsqueda
    gtk_search_entry_set_placeholder_text(data->search_entry, "Busca tu aplicación");

    LOG_INFO("Búsqueda de utilities apps configurada");
}

void window_apps_connect_signals(WindowAppsData *data)
{
    if (!data) return;

    // Conectar señales de botones
    if (data->close_button) {
        g_signal_connect(data->close_button, "clicked",
                        G_CALLBACK(on_apps_close_button_clicked), data);
    }

    if (data->save_button) {
        g_signal_connect(data->save_button, "clicked",
                        G_CALLBACK(on_apps_save_button_clicked), data);
    }

    // Conectar señales de búsqueda
    if (data->search_entry) {
        g_signal_connect(data->search_entry, "search-changed",
                        G_CALLBACK(on_apps_search_changed), data);
    }

    LOG_INFO("Señales de ventana de utilities apps conectadas");
}

void window_apps_connect_info_buttons(WindowAppsData *data)
{
    if (!data || !data->builder) return;

    // Definir todos los botones de información con sus URLs
    struct {
        const char *button_id;
        const char *url;
    } info_buttons[] = {
        // Browsers
        {"chrome_info", "https://www.google.com/chrome/"},
        {"brave_info", "https://brave.com/"},
        {"chromium_info", "https://www.chromium.org/"},
        {"firefox_info", "https://www.mozilla.org/firefox/"},
        {"opera_info", "https://www.opera.com/"},

        // Graphics
        {"gimp_info", "https://www.gimp.org/"},
        {"inkscape_info", "https://inkscape.org/"},
        {"krita_info", "https://krita.org/"},
        {"pinta_info", "https://www.pinta-project.com/"},
        {"blender_info", "https://www.blender.org/"},
        {"darktable_info", "https://www.darktable.org/"},
        {"freecad_info", "https://www.freecadweb.org/"},
        {"ristretto_info", "https://docs.xfce.org/apps/ristretto/start"},
        {"viewnior_info", "http://siyanpanayotov.com/project/viewnior/"},

        // Video
        {"baka_info", "https://github.com/u8sand/Baka-MPlayer"},
        {"dragon_info", "https://github.com/mwh/dragon"},
        {"kdenlive_info", "https://kdenlive.org/"},
        {"obs_info", "https://obsproject.com/"},
        {"openshot_info", "https://www.openshot.org/"},
        {"pitivi_info", "http://www.pitivi.org/"},
        {"shotcut_info", "https://shotcut.org/"},
        {"smplayer_info", "https://www.smplayer.info/"},
        {"vlc_info", "https://www.videolan.org/vlc/"},
        {"handbrake_info", "https://handbrake.fr/"},
        {"clapper_info", "https://rafostar.github.io/clapper/"},
        {"lmms_info", "https://lmms.io/"},
        {"simplescreenrecorder_info", "https://www.maartenbaert.be/simplescreenrecorder/"},

        // Audio
        {"audacious_info", "https://audacious-media-player.org/"},
        {"clementine_info", "https://www.clementine-player.org/"},
        {"audacity_info", "https://www.audacityteam.org/"},
        {"ardour_info", "https://ardour.org/"},
        {"elisa_info", "https://apps.kde.org/elisa/"},
        {"euphonica_info", "https://github.com/htkhiem/euphonica/"},

        // Communications
        {"telegram_info", "https://telegram.org/"},
        {"element_info", "https://element.io/"},
        {"discord_info", "https://discord.com/"},
        {"thunderbird_info", "https://www.thunderbird.net/"},
        {"signal_info", "https://signal.org/"},
        {"whatsapp_info", "https://github.com/eneshecan/whatsapp-for-linux"},
        {"evolution_info", "https://wiki.gnome.org/Apps/Evolution"},
        {"fractal_info", "https://wiki.gnome.org/Apps/Fractal"},

        // Development
        {"vscode_info", "https://code.visualstudio.com/"},
        {"vscodium_info", "https://vscodium.com/"},
        {"zed_info", "https://zed.dev/"},
        {"geany_info", "https://www.geany.org/"},
        {"sublime_info", "https://www.sublimetext.com/"},
        {"emacs_info", "https://www.gnu.org/software/emacs/"},
        {"docker_info", "https://www.docker.com/"},
        {"pycharm_info", "https://www.jetbrains.com/pycharm/"},
        {"intellij_info", "https://www.jetbrains.com/idea/"},
        {"netbeans_info", "https://netbeans.apache.org/"},

        // Office
        {"libreoffice_info", "https://www.libreoffice.org/"},
        {"onlyoffice_info", "https://www.onlyoffice.com/"},
        {"wps_info", "https://www.wps.com/"},
        {"abiword_info", "https://www.abisource.com/"},
        {"calibre_info", "https://calibre-ebook.com/"},
        {"evince_info", "https://wiki.gnome.org/Apps/Evince"},
        {"okular_info", "https://okular.kde.org/"},
        {"paperwork_info", "https://openpaper.work/"},

        // Gaming
        {"steam_info", "https://store.steampowered.com/"},
        {"lutris_info", "https://lutris.net/"},
        {"heroic_info", "https://heroicgameslauncher.com/"},
        {"bottles_info", "https://usebottles.com/"},
        {"wine_info", "https://www.winehq.org/"},
        {"playonlinux_info", "https://www.playonlinux.com/"},
        {"supertux_info", "https://www.supertux.org/"},

        // Others
        {"octopi_info", "https://tintaescura.com/projects/octopi/"},
        {"pamac_info", "https://gitlab.manjaro.org/applications/pamac"},
        {"gnome_boxes_info", "https://wiki.gnome.org/Apps/Boxes"},
        {"virt_manager_info", "https://www.qemu.org/"},
        {"virtualbox_info", "https://www.virtualbox.org/"},
        {"gufw_info", "https://gufw.org/"},
        {"brasero_info", "https://wiki.gnome.org/Apps/Brasero"},
        {"transmission_info", "https://transmissionbt.com/"},
        {"filezilla_info", "https://filezilla-project.org/"},
        {"putty_info", "https://www.putty.org/"},
        {"stremio_info", "https://www.stremio.com/"},
        {"mission_center_info", "https://missioncenter.io/"},
        {"htop_info", "https://htop.dev/"},
        {"neovim_info", "https://neovim.io/"},
        {"timeshift_info", "https://github.com/teejee2008/timeshift"},
        {"keepassxc_info", "https://keepassxc.org/"},

        {NULL, NULL} // Terminador
    };

    // Conectar cada botón de información
    for (int i = 0; info_buttons[i].button_id != NULL; i++) {
        GtkButton *button = GTK_BUTTON(gtk_builder_get_object(data->builder, info_buttons[i].button_id));
        if (button) {
            // Crear datos para el callback
            InfoButtonData *button_data = g_new0(InfoButtonData, 1);
            button_data->url = g_strdup(info_buttons[i].url);
            button_data->window = data->window;

            g_signal_connect_data(button, "clicked",
                                G_CALLBACK(on_info_button_clicked),
                                button_data,
                                (GClosureNotify)window_apps_free_info_button_data,
                                0);

            LOG_INFO("Conectado botón de información: %s -> %s", info_buttons[i].button_id, info_buttons[i].url);
        } else {
            LOG_WARNING("No se pudo encontrar el botón: %s", info_buttons[i].button_id);
        }
    }

    LOG_INFO("Botones de información conectados");
}

void window_apps_filter_apps(WindowAppsData *data, const gchar *search_text)
{
    if (!data || !data->builder) return;

    // Lista de todas las filas de aplicaciones con sus nombres para buscar
    struct {
        const char *row_id;
        const char *app_name;
        const char *description;
        const char *category_expander;
    } app_rows[] = {
        // Browsers
        {"chrome_check", "google-chrome", "navegador web google", "browsers_expander"},
        {"brave_check", "brave-bin", "navegador privado brave", "browsers_expander"},
        {"chromium_check", "chromium", "navegador chromium", "browsers_expander"},
        {"firefox_check", "firefox", "navegador firefox mozilla", "browsers_expander"},
        {"opera_check", "opera", "navegador opera", "browsers_expander"},

        // Graphics
        {"gimp_check", "gimp", "editor de imágenes gimp gnu", "graphics_expander"},
        {"inkscape_check", "inkscape", "editor vectorial inkscape", "graphics_expander"},
        {"krita_check", "krita", "pintura digital krita", "graphics_expander"},
        {"pinta_check", "pinta", "editor imágenes pinta", "graphics_expander"},
        {"blender_check", "blender", "3d modelado animación blender", "graphics_expander"},
        {"darktable_check", "darktable", "fotografía raw darktable", "graphics_expander"},
        {"freecad_check", "freecad", "cad 3d freecad", "graphics_expander"},
        {"ristretto_check", "ristretto", "visor imágenes ristretto xfce", "graphics_expander"},
        {"viewnior_check", "viewnior", "visor imágenes viewnior", "graphics_expander"},

        // Video
        {"baka_check", "baka-mplayer", "reproductor multimedia baka", "video_expander"},
        {"dragon_check", "dragon", "reproductor dragon simple", "video_expander"},
        {"kdenlive_check", "kdenlive", "editor video kdenlive", "video_expander"},
        {"obs_check", "obs-studio", "grabación streaming obs", "video_expander"},
        {"openshot_check", "openshot", "editor video openshot", "video_expander"},
        {"pitivi_check", "pitivi", "editor video pitivi", "video_expander"},
        {"shotcut_check", "shotcut", "editor video shotcut", "video_expander"},
        {"smplayer_check", "smplayer", "reproductor multimedia smplayer", "video_expander"},
        {"vlc_check", "vlc", "reproductor multimedia vlc", "video_expander"},
        {"handbrake_check", "handbrake", "transcodificador video handbrake", "video_expander"},
        {"clapper_check", "clapper", "reproductor multimedia clapper gnome", "video_expander"},
        {"lmms_check", "lmms", "audio digital lmms", "video_expander"},
        {"simplescreenrecorder_check", "simplescreenrecorder", "grabador pantalla simple", "video_expander"},

        // Audio
        {"audacious_check", "audacious", "reproductor audio audacious", "audio_expander"},
        {"clementine_check", "clementine-git", "reproductor música clementine", "audio_expander"},
        {"audacity_check", "audacity", "editor audio audacity", "audio_expander"},
        {"ardour_check", "ardour", "audio digital ardour profesional", "audio_expander"},
        {"elisa_check", "elisa", "reproductor música elisa kde", "audio_expander"},
        {"euphonica_check", "euphonica", "An MPD frontend with delusions of grandeur", "audio_expander"},

        // Communications
        {"telegram_check", "telegram-desktop", "mensajería telegram", "mail_expander"},
        {"element_check", "element-desktop", "mensajería matrix element", "mail_expander"},
        {"discord_check", "discord", "comunicación discord gamers", "mail_expander"},
        {"thunderbird_check", "thunderbird", "correo thunderbird mozilla", "mail_expander"},
        {"signal_check", "signal-desktop", "mensajería signal privada", "mail_expander"},
        {"whatsapp_check", "whatsapp-for-linux-git", "whatsapp linux", "mail_expander"},
        {"evolution_check", "evolution", "correo evolution gnome", "mail_expander"},
        {"fractal_check", "fractal", "matrix fractal gnome", "mail_expander"},

        // Development
        {"vscode_check", "visual-studio-code-bin", "editor código vscode microsoft", "developers_expander"},
        {"vscodium_check", "vscodium-bin", "editor código vscodium libre", "developers_expander"},
        {"zed_check", "zed", "editor código zed colaborativo", "developers_expander"},
        {"geany_check", "geany", "ide geany ligero", "developers_expander"},
        {"sublime_check", "sublime-text-4", "editor sublime text", "developers_expander"},
        {"emacs_check", "emacs", "editor emacs extensible", "developers_expander"},
        {"docker_check", "docker", "contenedores docker", "developers_expander"},
        {"pycharm_check", "pycharm-community-edition-bin", "ide python pycharm", "developers_expander"},
        {"intellij_check", "intellij-idea-community-edition-bin", "ide java intellij", "developers_expander"},
        {"netbeans_check", "netbeans", "ide netbeans apache", "developers_expander"},

        // Office
        {"libreoffice_check", "libreoffice-fresh", "suite ofimática libreoffice", "office_expander"},
        {"onlyoffice_check", "onlyoffice-bin", "suite ofimática onlyoffice", "office_expander"},
        {"wps_check", "wps-office", "suite ofimática wps", "office_expander"},
        {"abiword_check", "abiword", "procesador textos abiword", "office_expander"},
        {"calibre_check", "calibre", "libros electrónicos calibre", "office_expander"},
        {"evince_check", "evince", "visor documentos evince pdf", "office_expander"},
        {"okular_check", "okular", "visor documentos okular kde", "office_expander"},
        {"paperwork_check", "paperwork", "gestor documentos paperwork ocr", "office_expander"},

        // Gaming
        {"steam_check", "steam", "gaming steam videojuegos", "gamming_expander"},
        {"lutris_check", "lutris", "gaming lutris linux", "gamming_expander"},
        {"heroic_check", "heroic-games-launcher-bin", "launcher heroic epic gog", "gamming_expander"},
        {"bottles_check", "bottles", "wine bottles gestor", "gamming_expander"},
        {"wine_check", "wine", "compatibilidad wine windows", "gamming_expander"},
        {"playonlinux_check", "playonlinux", "wine playonlinux frontend", "gamming_expander"},
        {"supertux_check", "supertux", "juego supertux plataformas", "gamming_expander"},

        // Others
        {"octopi_check", "octopi", "gestor paquetes octopi pacman", "other_expander"},
        {"pamac_check", "pamac-aur", "gestor paquetes pamac aur", "other_expander"},
        {"gnome_boxes_check", "gnome-boxes", "virtualización boxes gnome", "other_expander"},
        {"virt_manager_check", "qemu-full", "virtualización virt manager qemu", "other_expander"},
        {"virtualbox_check", "virtualbox", "virtualización virtualbox", "other_expander"},
        {"gufw_check", "gufw", "firewall gufw ufw", "other_expander"},
        {"brasero_check", "brasero", "grabación cd dvd brasero", "other_expander"},
        {"transmission_check", "transmission-gtk", "torrent transmission", "other_expander"},
        {"filezilla_check", "filezilla", "ftp filezilla cliente", "other_expander"},
        {"putty_check", "putty", "ssh putty cliente", "other_expander"},
        {"stremio_check", "stremio", "multimedia streaming stremio", "other_expander"},
        {"mission_center_check", "mission-center", "monitor sistema mission center", "other_expander"},
        {"htop_check", "htop", "monitor procesos htop", "other_expander"},
        {"neovim_check", "neovim", "editor neovim vim", "other_expander"},
        {"timeshift_check", "timeshift", "backup timeshift sistema", "other_expander"},
        {"keepassxc_check", "keepassxc", "gestor contraseñas keepassxc", "other_expander"},

        {NULL, NULL, NULL, NULL} // Terminador
    };

    gboolean has_search = (search_text && strlen(search_text) > 0);
    gchar *search_lower = has_search ? g_utf8_strdown(search_text, -1) : NULL;

    // Rastrear qué categorías tienen coincidencias
    GHashTable *categories_with_matches = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);

    // Filtrar cada fila de aplicación
    for (int i = 0; app_rows[i].row_id != NULL; i++) {
        GObject *check_obj = gtk_builder_get_object(data->builder, app_rows[i].row_id);
        if (check_obj) {
            GtkWidget *check_widget = GTK_WIDGET(check_obj);
            GtkWidget *row_widget = gtk_widget_get_parent(check_widget);

            // Buscar el AdwActionRow que contiene el checkbox
            while (row_widget && !ADW_IS_ACTION_ROW(row_widget)) {
                row_widget = gtk_widget_get_parent(row_widget);
            }

            if (row_widget) {
                gboolean should_show = TRUE;

                if (has_search) {
                    // Buscar en nombre de app y descripción
                    gchar *app_name_lower = g_utf8_strdown(app_rows[i].app_name, -1);
                    gchar *description_lower = g_utf8_strdown(app_rows[i].description, -1);

                    should_show = (strstr(app_name_lower, search_lower) != NULL ||
                                 strstr(description_lower, search_lower) != NULL);

                    // Si hay coincidencia, marcar la categoría
                    if (should_show && app_rows[i].category_expander) {
                        g_hash_table_add(categories_with_matches, (gpointer)app_rows[i].category_expander);
                    }

                    g_free(app_name_lower);
                    g_free(description_lower);
                }

                gtk_widget_set_visible(row_widget, should_show);
            }
        }
    }

    // Expandir/contraer categorías según coincidencias
    const char *expander_ids[] = {
        "browsers_expander", "graphics_expander", "video_expander",
        "audio_expander", "mail_expander", "developers_expander",
        "office_expander", "gamming_expander", "other_expander", NULL
    };

    for (int i = 0; expander_ids[i] != NULL; i++) {
        AdwExpanderRow *expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, expander_ids[i]));
        if (expander) {
            gboolean should_expand = has_search ?
                g_hash_table_contains(categories_with_matches, expander_ids[i]) : FALSE;
            adw_expander_row_set_expanded(expander, should_expand);
        }
    }

    // Limpiar
    g_hash_table_destroy(categories_with_matches);

    if (search_lower) {
        g_free(search_lower);
    }

    if (has_search && strlen(search_text) > 0) {
        LOG_INFO("Filtrando apps con texto: %s", search_text);
    } else {
        LOG_INFO("Mostrando todas las apps");
    }
}

void on_info_button_clicked(GtkButton *button, gpointer user_data)
{
    InfoButtonData *data = (InfoButtonData*)user_data;
    if (!data || !data->url) return;

    LOG_INFO("Abriendo URL: %s", data->url);

    GError *error = NULL;

    // Intentar abrir la URL con el navegador predeterminado
    if (!g_app_info_launch_default_for_uri(data->url, NULL, &error)) {
        LOG_ERROR("Error abriendo URL %s: %s", data->url, error ? error->message : "Unknown error");

        // Mostrar diálogo de error
        if (data->window) {
            gchar *message = g_strdup_printf(
                "No se pudo abrir la URL: %s\n\nError: %s",
                data->url,
                error ? error->message : "Navegador no encontrado"
            );

            AdwDialog *dialog = adw_alert_dialog_new(
                "Error al abrir URL",
                message
            );

            adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "ok", "Aceptar");
            adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "ok");

            adw_dialog_present(dialog, GTK_WIDGET(data->window));

            g_free(message);
        }

        if (error) g_error_free(error);
    } else {
        LOG_INFO("URL abierta exitosamente: %s", data->url);
    }
}

void window_apps_free_info_button_data(InfoButtonData *data)
{
    if (!data) return;

    if (data->url) {
        g_free(data->url);
        data->url = NULL;
    }

    g_free(data);
}

gboolean window_apps_load_selected_apps_from_file(WindowAppsData *data)
{
    if (!data || !data->selected_apps) return FALSE;

    GError *error = NULL;
    gchar *content = NULL;
    gsize length;

    if (g_file_get_contents(VARIABLES_FILE_PATH, &content, &length, &error)) {
        // Buscar la línea UTILITIES_APPS=
        gchar **lines = g_strsplit(content, "\n", -1);

        for (int i = 0; lines[i] != NULL; i++) {
            gchar *line = g_strstrip(lines[i]);
            if (g_str_has_prefix(line, "UTILITIES_APPS=(")) {
                // Extraer contenido del array
                gchar *start = strchr(line, '(');
                gchar *end = strrchr(line, ')');
                if (start && end && end > start) {
                    start++; // Saltar el '('
                    *end = '\0'; // Terminar en ')'

                    // Convertir array bash a hash table
                    gchar **apps = g_strsplit(start, " ", -1);

                    // Limpiar hash table anterior
                    g_hash_table_remove_all(data->selected_apps);

                    for (int j = 0; apps[j] != NULL; j++) {
                        gchar *app = g_strstrip(apps[j]);
                        // Remover comillas si las hay
                        if (g_str_has_prefix(app, "\"") && g_str_has_suffix(app, "\"")) {
                            app[strlen(app)-1] = '\0';
                            app++;
                        }
                        if (strlen(app) > 0) {
                            g_hash_table_insert(data->selected_apps, g_strdup(app), g_strdup("selected"));
                        }
                    }

                    g_strfreev(apps);
                }
                break;
            }
        }

        g_strfreev(lines);
        g_free(content);
        LOG_INFO("Utilities apps cargadas desde variables.sh");
        return TRUE;
    } else {
        if (error) {
            LOG_INFO("No se pudo cargar archivo variables.sh: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }
}

gboolean window_apps_save_selected_apps_to_file(WindowAppsData *data)
{
    if (!data || !data->selected_apps) return FALSE;

    // Leer archivo variables.sh actual
    GError *error = NULL;
    gchar *content = NULL;
    gsize length;

    if (!g_file_get_contents(VARIABLES_FILE_PATH, &content, &length, &error)) {
        LOG_ERROR("No se pudo leer variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }

    // Crear contenido del array
    GString *array_content = g_string_new("UTILITIES_APPS=(");

    if (g_hash_table_size(data->selected_apps) > 0) {
        GHashTableIter iter;
        gpointer key, value;
        gboolean first = TRUE;

        g_hash_table_iter_init(&iter, data->selected_apps);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            if (!first) g_string_append(array_content, " ");
            g_string_append_printf(array_content, "\"%s\"", (gchar*)key);
            first = FALSE;
        }
    }

    g_string_append(array_content, ")");

    // Buscar y reemplazar línea UTILITIES_APPS o agregarla
    gchar **lines = g_strsplit(content, "\n", -1);
    GString *new_content = g_string_new("");
    gboolean found = FALSE;

    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(g_strstrip(lines[i]), "UTILITIES_APPS=")) {
            g_string_append_printf(new_content, "%s\n", array_content->str);
            found = TRUE;
        } else {
            g_string_append_printf(new_content, "%s\n", lines[i]);
        }
    }

    // Si no se encontró, agregar al final
    if (!found) {
        g_string_append_printf(new_content, "\n# Utilities apps seleccionadas por el usuario\n%s\n", array_content->str);
    }

    // Guardar archivo actualizado
    gboolean success = g_file_set_contents(VARIABLES_FILE_PATH, new_content->str, -1, &error);

    if (success) {
        LOG_INFO("Utilities apps guardadas como array en variables.sh");
    } else {
        LOG_ERROR("Error guardando en variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    }

    // Limpiar memoria
    g_string_free(array_content, TRUE);
    g_string_free(new_content, TRUE);
    g_strfreev(lines);
    g_free(content);

    return success;
}

GHashTable* window_apps_get_selected_apps(WindowAppsData *data)
{
    if (!data) return NULL;
    return data->selected_apps;
}

void window_apps_set_selected_apps(WindowAppsData *data, GHashTable *apps)
{
    if (!data || !apps) return;

    if (data->selected_apps) {
        g_hash_table_destroy(data->selected_apps);
    }

    data->selected_apps = g_hash_table_ref(apps);
}

// Callbacks

void on_apps_close_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowAppsData *data = (WindowAppsData*)user_data;
    if (!data) return;

    LOG_INFO("Cerrando ventana de utilities apps");
    window_apps_hide(data);
}

void on_apps_save_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowAppsData *data = (WindowAppsData*)user_data;
    if (!data) return;

    LOG_INFO("Guardando utilities apps seleccionadas");

    // Recopilar aplicaciones seleccionadas de los checkboxes
    window_apps_collect_selected_apps_from_checkboxes(data);

    if (window_apps_save_selected_apps_to_file(data)) {
        LOG_INFO("Utilities apps guardadas exitosamente");
        // Cerrar ventana después de guardar
        window_apps_hide(data);
    } else {
        LOG_ERROR("Error al guardar utilities apps");
    }
}

void on_apps_search_changed(GtkSearchEntry *entry, gpointer user_data)
{
    WindowAppsData *data = (WindowAppsData*)user_data;
    if (!data) return;

    const gchar *search_text = gtk_editable_get_text(GTK_EDITABLE(entry));
    window_apps_filter_apps(data, search_text);
}

void window_apps_collect_selected_apps_from_checkboxes(WindowAppsData *data)
{
    if (!data || !data->builder || !data->selected_apps) return;

    // Lista de todos los checkboxes con sus nombres de paquete
    struct {
        const char *checkbox_id;
        const char *package_name;
    } checkboxes[] = {
        // Browsers
        {"chrome_check", "google-chrome"},
        {"brave_check", "brave-bin"},
        {"chromium_check", "chromium"},
        {"firefox_check", "firefox"},
        {"opera_check", "opera"},

        // Graphics
        {"gimp_check", "gimp"},
        {"inkscape_check", "inkscape"},
        {"krita_check", "krita"},
        {"pinta_check", "pinta"},
        {"blender_check", "blender"},
        {"darktable_check", "darktable"},
        {"freecad_check", "freecad"},
        {"ristretto_check", "ristretto"},
        {"viewnior_check", "viewnior"},

        // Video
        {"baka_check", "baka-mplayer"},
        {"dragon_check", "dragon"},
        {"kdenlive_check", "kdenlive"},
        {"obs_check", "obs-studio"},
        {"openshot_check", "openshot"},
        {"pitivi_check", "pitivi"},
        {"shotcut_check", "shotcut"},
        {"smplayer_check", "smplayer"},
        {"vlc_check", "vlc"},
        {"handbrake_check", "handbrake"},
        {"clapper_check", "clapper"},
        {"lmms_check", "lmms"},
        {"simplescreenrecorder_check", "simplescreenrecorder"},

        // Audio
        {"audacious_check", "audacious"},
        {"clementine_check", "clementine-git"},
        {"audacity_check", "audacity"},
        {"ardour_check", "ardour"},
        {"elisa_check", "elisa"},
        {"euphonica_check", "euphonica"},

        // Communications
        {"telegram_check", "telegram-desktop"},
        {"element_check", "element-desktop"},
        {"discord_check", "discord"},
        {"thunderbird_check", "thunderbird"},
        {"signal_check", "signal-desktop"},
        {"whatsapp_check", "whatsapp-for-linux-git"},
        {"evolution_check", "evolution"},
        {"fractal_check", "fractal"},

        // Development
        {"vscode_check", "visual-studio-code-bin"},
        {"vscodium_check", "vscodium-bin"},
        {"zed_check", "zed"},
        {"geany_check", "geany"},
        {"sublime_check", "sublime-text-4"},
        {"emacs_check", "emacs"},
        {"docker_check", "docker"},
        {"pycharm_check", "pycharm-community-edition-bin"},
        {"intellij_check", "intellij-idea-community-edition-bin"},
        {"netbeans_check", "netbeans"},

        // Office
        {"libreoffice_check", "libreoffice-fresh"},
        {"onlyoffice_check", "onlyoffice-bin"},
        {"wps_check", "wps-office"},
        {"abiword_check", "abiword"},
        {"calibre_check", "calibre"},
        {"evince_check", "evince"},
        {"okular_check", "okular"},
        {"paperwork_check", "paperwork"},

        // Gaming
        {"steam_check", "steam"},
        {"lutris_check", "lutris"},
        {"heroic_check", "heroic-games-launcher-bin"},
        {"bottles_check", "bottles"},
        {"wine_check", "wine"},
        {"playonlinux_check", "playonlinux"},
        {"supertux_check", "supertux"},

        // Others
        {"octopi_check", "octopi"},
        {"pamac_check", "pamac-aur"},
        {"gnome_boxes_check", "gnome-boxes"},
        {"virt_manager_check", "qemu-full"},
        {"virtualbox_check", "virtualbox"},
        {"gufw_check", "gufw"},
        {"brasero_check", "brasero"},
        {"transmission_check", "transmission-gtk"},
        {"filezilla_check", "filezilla"},
        {"putty_check", "putty"},
        {"stremio_check", "stremio"},
        {"mission_center_check", "mission-center"},
        {"htop_check", "htop"},
        {"neovim_check", "neovim"},
        {"timeshift_check", "timeshift"},
        {"keepassxc_check", "keepassxc"},

        {NULL, NULL} // Terminador
    };

    // Limpiar hash table anterior
    g_hash_table_remove_all(data->selected_apps);

    // Recorrer todos los checkboxes y verificar cuáles están seleccionados
    for (int i = 0; checkboxes[i].checkbox_id != NULL; i++) {
        GtkCheckButton *checkbox = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, checkboxes[i].checkbox_id));
        if (checkbox) {
            gboolean is_active = gtk_check_button_get_active(checkbox);
            if (is_active) {
                g_hash_table_insert(data->selected_apps,
                                  g_strdup(checkboxes[i].package_name),
                                  g_strdup("selected"));
                LOG_INFO("App seleccionada: %s", checkboxes[i].package_name);
            }
        } else {
            LOG_WARNING("No se pudo encontrar checkbox: %s", checkboxes[i].checkbox_id);
        }
    }

    LOG_INFO("Total de apps seleccionadas: %d", g_hash_table_size(data->selected_apps));
}

void window_apps_apply_selections_to_checkboxes(WindowAppsData *data)
{
    if (!data || !data->builder || !data->selected_apps) return;

    // Lista de todos los checkboxes con sus nombres de paquete
    struct {
        const char *checkbox_id;
        const char *package_name;
    } checkboxes[] = {
        // Browsers
        {"chrome_check", "google-chrome"},
        {"brave_check", "brave-bin"},
        {"chromium_check", "chromium"},
        {"firefox_check", "firefox"},
        {"opera_check", "opera"},

        // Graphics
        {"gimp_check", "gimp"},
        {"inkscape_check", "inkscape"},
        {"krita_check", "krita"},
        {"pinta_check", "pinta"},
        {"blender_check", "blender"},
        {"darktable_check", "darktable"},
        {"freecad_check", "freecad"},
        {"ristretto_check", "ristretto"},
        {"viewnior_check", "viewnior"},

        // Video
        {"baka_check", "baka-mplayer"},
        {"dragon_check", "dragon"},
        {"kdenlive_check", "kdenlive"},
        {"obs_check", "obs-studio"},
        {"openshot_check", "openshot"},
        {"pitivi_check", "pitivi"},
        {"shotcut_check", "shotcut"},
        {"smplayer_check", "smplayer"},
        {"vlc_check", "vlc"},
        {"handbrake_check", "handbrake"},
        {"clapper_check", "clapper"},
        {"lmms_check", "lmms"},
        {"simplescreenrecorder_check", "simplescreenrecorder"},

        // Audio
        {"audacious_check", "audacious"},
        {"clementine_check", "clementine-git"},
        {"audacity_check", "audacity"},
        {"ardour_check", "ardour"},
        {"elisa_check", "elisa"},
        {"euphonica_check", "euphonica"},

        // Communications
        {"telegram_check", "telegram-desktop"},
        {"element_check", "element-desktop"},
        {"discord_check", "discord"},
        {"thunderbird_check", "thunderbird"},
        {"signal_check", "signal-desktop"},
        {"whatsapp_check", "whatsapp-for-linux-git"},
        {"evolution_check", "evolution"},
        {"fractal_check", "fractal"},

        // Development
        {"vscode_check", "visual-studio-code-bin"},
        {"vscodium_check", "vscodium-bin"},
        {"zed_check", "zed"},
        {"geany_check", "geany"},
        {"sublime_check", "sublime-text-4"},
        {"emacs_check", "emacs"},
        {"docker_check", "docker"},
        {"pycharm_check", "pycharm-community-edition-bin"},
        {"intellij_check", "intellij-idea-community-edition-bin"},
        {"netbeans_check", "netbeans"},

        // Office
        {"libreoffice_check", "libreoffice-fresh"},
        {"onlyoffice_check", "onlyoffice-bin"},
        {"wps_check", "wps-office"},
        {"abiword_check", "abiword"},
        {"calibre_check", "calibre"},
        {"evince_check", "evince"},
        {"okular_check", "okular"},
        {"paperwork_check", "paperwork"},

        // Gaming
        {"steam_check", "steam"},
        {"lutris_check", "lutris"},
        {"heroic_check", "heroic-games-launcher-bin"},
        {"bottles_check", "bottles"},
        {"wine_check", "wine"},
        {"playonlinux_check", "playonlinux"},
        {"supertux_check", "supertux"},

        // Others
        {"octopi_check", "octopi"},
        {"pamac_check", "pamac-aur"},
        {"gnome_boxes_check", "gnome-boxes"},
        {"virt_manager_check", "qemu-full"},
        {"virtualbox_check", "virtualbox"},
        {"gufw_check", "gufw"},
        {"brasero_check", "brasero"},
        {"transmission_check", "transmission-gtk"},
        {"filezilla_check", "filezilla"},
        {"putty_check", "putty"},
        {"stremio_check", "stremio"},
        {"mission_center_check", "mission-center"},
        {"htop_check", "htop"},
        {"neovim_check", "neovim"},
        {"timeshift_check", "timeshift"},
        {"keepassxc_check", "keepassxc"},

        {NULL, NULL} // Terminador
    };

    // Recorrer todos los checkboxes y activar los que están en la selección
    int applied_count = 0;
    for (int i = 0; checkboxes[i].checkbox_id != NULL; i++) {
        GtkCheckButton *checkbox = GTK_CHECK_BUTTON(gtk_builder_get_object(data->builder, checkboxes[i].checkbox_id));
        if (checkbox) {
            gboolean should_be_active = g_hash_table_contains(data->selected_apps, checkboxes[i].package_name);
            gtk_check_button_set_active(checkbox, should_be_active);
            if (should_be_active) {
                applied_count++;
                LOG_INFO("Aplicando selección: %s", checkboxes[i].package_name);
            }
        } else {
            LOG_WARNING("No se pudo encontrar checkbox: %s", checkboxes[i].checkbox_id);
        }
    }

    LOG_INFO("Selecciones aplicadas a checkboxes: %d", applied_count);
}

// Funciones de utilidad

void window_apps_reset_to_defaults(WindowAppsData *data)
{
    if (!data) return;

    if (data->selected_apps) {
        g_hash_table_remove_all(data->selected_apps);
    }

    LOG_INFO("Ventana de utilities apps reiniciada a valores por defecto");
}

WindowAppsData* window_apps_get_instance(void)
{
    if (!global_apps_data) {
        global_apps_data = window_apps_new();
    }
    return global_apps_data;
}
