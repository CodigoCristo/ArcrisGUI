#include "window_apps.h"
#include "config.h"
#include "variables_utils.h"
#include "i18n.h"
#include <glib/gstdio.h>
#include <string.h>
#include <gio/gio.h>

// Instancia global
static WindowAppsData *global_apps_data = NULL;

// Constantes
#define VARIABLES_FILE_PATH "./data/bash/variables.sh"

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
    window_apps_update_language(data);
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
    data->window_title = ADW_WINDOW_TITLE(gtk_builder_get_object(data->builder, "apps_window_title"));
    data->packages_group = ADW_PREFERENCES_GROUP(gtk_builder_get_object(data->builder, "packages_group"));

    // Cargar entrada de búsqueda
    data->search_entry = GTK_SEARCH_ENTRY(gtk_builder_get_object(data->builder, "searchApp"));

    // Cargar expanderes
    data->browsers_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "browsers_expander"));
    data->graphics_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "graphics_expander"));
    data->video_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "video_expander"));
    data->audio_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "audio_expander"));
    data->mail_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "mail_expander"));
    data->developers_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "developers_expander"));
    data->office_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "office_expander"));
    data->gamming_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "gamming_expander"));
    data->other_expander = ADW_EXPANDER_ROW(gtk_builder_get_object(data->builder, "other_expander"));

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
        {"mpv_info", "https://mpv.io/"},
        {"celluloid_info", "https://celluloid-player.github.io/"},
        {"showtime_info", "https://apps.gnome.org/es/Showtime/"},
        {"kooha_info", "https://github.com/SeaDve/Kooha"},
        {"vokoscreen_info", "https://linuxecke.volkoh.de/vokoscreen/vokoscreen.html"},

        // Audio
        {"audacious_info", "https://audacious-media-player.org/"},
        {"decibels_info", "https://apps.gnome.org/es/Decibels/"},
        {"clementine_info", "https://www.clementine-player.org/"},
        {"audacity_info", "https://www.audacityteam.org/"},
        {"ardour_info", "https://ardour.org/"},
        {"elisa_info", "https://apps.kde.org/elisa/"},
        {"euphonica_info", "https://github.com/htkhiem/euphonica/"},
        {"lmms_info", "https://lmms.io/"},
        {"spotify_info", "https://www.spotify.com"},

        // Communications
        {"telegram_info", "https://telegram.org/"},
        {"element_info", "https://element.io/"},
        {"discord_info", "https://discord.com/"},
        {"thunderbird_info", "https://www.thunderbird.net/"},
        {"signal_info", "https://signal.org/"},
        {"whatsapp_info", "https://rtosta.com/zapzap/"},
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
        {"android_studio_info", "https://developer.android.com/studio/"},
        {"netbeans_info", "https://netbeans.apache.org/"},

        // Office
        {"libreoffice_info", "https://www.libreoffice.org/"},
        {"onlyoffice_info", "https://www.onlyoffice.com/"},
        {"wps_info", "https://www.wps.com/"},
        {"abiword_info", "https://www.abisource.com/"},
        {"calibre_info", "https://calibre-ebook.com/"},
        {"papers_info", "https://apps.gnome.org/Papers/"},
        {"okular_info", "https://okular.kde.org/"},
        {"paperwork_info", "https://openpaper.work/"},

        // Gaming
        {"steam_info", "https://store.steampowered.com/"},
        {"lutris_info", "https://lutris.net/"},
        {"heroic_info", "https://heroicgameslauncher.com/"},
        {"bottles_info", "https://usebottles.com/"},
        {"proton_ge_info", "https://github.com/GloriousEggroll/proton-ge-custom"},
        {"winetricks_info", "https://github.com/Winetricks/winetricks"},
        {"gamemode_info", "https://github.com/FeralInteractive/gamemode"},
        {"mangohud_info", "https://github.com/flightlessmango/MangoHud"},
        {"gnome_games_info", "https://wiki.gnome.org/Apps/Games"},
        {"retroarch_info", "https://www.retroarch.com/"},
        {"ppsspp_info", "https://www.ppsspp.org/"},
        {"duckstation_info", "https://github.com/stenzek/duckstation"},
        {"pcsx2_info", "https://pcsx2.net/"},
        {"rpcs3_info", "https://rpcs3.net/"},
        {"shadps4_info", "https://github.com/shadps4-emu/shadPS4"},
        {"snes9x_gtk_info", "https://github.com/snes9xgit/snes9x"},
        {"zsnes_info", "http://www.zsnes.com/"},
        {"ryujinx_info", "https://ryujinx.org/"},
        {"citron_info", "https://github.com/emuplace/citron"},
        {"ryujinx_info", "https://ryujinx.app/"},
        {"dolphin_emu_info", "https://dolphin-emu.org/"},
        {"mesen2_info", "https://github.com/SourMesen/Mesen2"},
        {"fceux_info", "http://fceux.com/"},
        {"bsnes_qt5_info", "https://github.com/bsnes-emu/bsnes"},
        {"mgba_qt_info", "https://mgba.io/"},
        {"skyemu_info", "https://github.com/skylersaleh/SkyEmu"},
        {"azahar_info", "https://github.com/Xaviercreator/azahar"},
        {"melonds_info", "http://melonds.kuribo64.net/"},
        {"mame_info", "https://www.mamedev.org/"},
        {"wine_info", "https://www.winehq.org/"},
        {"playonlinux_info", "https://www.playonlinux.com/"},
        {"supertuxkart_info", "https://supertuxkart.net/"},
        {"supertux_info", "https://www.supertux.org/"},

        // Others
        {"octopi_info", "https://tintaescura.com/projects/octopi/"},
        {"pamac_info", "https://gitlab.manjaro.org/applications/pamac"},
        {"gnome_boxes_info", "https://wiki.gnome.org/Apps/Boxes"},
        {"virt_manager_info", "https://www.qemu.org/"},
        {"virtualbox_info", "https://www.virtualbox.org/"},
        {"genymotion_info", "https://www.genymotion.com/"},
        {"gufw_info", "https://gufw.org/"},
        {"brasero_info", "https://wiki.gnome.org/Apps/Brasero"},
        {"gparted_info", "https://gparted.org/"},
        {"gnome_disk_utility_info", "https://wiki.gnome.org/Apps/Disks"},
        {"transmission_info", "https://transmissionbt.com/"},
        {"filezilla_info", "https://filezilla-project.org/"},
        {"putty_info", "https://www.putty.org/"},
        {"stremio_info", "https://www.stremio.com/"},
        {"mission_center_info", "https://missioncenter.io/"},
        {"resources_info", "https://apps.gnome.org/Resources/"},
        {"htop_info", "https://htop.dev/"},
        {"bottom_info", "https://clementtsang.github.io/bottom/"},
        {"btop_info", "https://github.com/aristocratos/btop"},
        {"vim_info", "https://www.vim.org/"},
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
        {"mpv_check", "mpv", "reproductor multimedia mpv código abierto", "video_expander"},
        {"celluloid_check", "celluloid", "reproductor multimedia celluloid interfaz mpv gtk", "video_expander"},
        {"showtime_check", "showtime", "reproductor multimedia showtime gnome", "video_expander"},
        {"kooha_check", "kooha", "grabador pantalla kooha gtk elegante", "video_expander"},
        {"vokoscreen_check", "vokoscreen", "grabador pantalla vokoscreen múltiples formatos", "video_expander"},

        // Audio
        {"audacious_check", "audacious", "reproductor audio audacious", "audio_expander"},
        {"decibels_check", "decibels", "reproductor sonido decibels", "audio_expander"},
        {"clementine_check", "clementine-git", "reproductor música clementine", "audio_expander"},
        {"audacity_check", "audacity", "editor audio audacity", "audio_expander"},
        {"ardour_check", "ardour", "audio digital ardour profesional", "audio_expander"},
        {"elisa_check", "elisa", "reproductor música elisa kde", "audio_expander"},
        {"euphonica_check", "euphonica", "An MPD frontend with delusions of grandeur", "audio_expander"},
        {"lmms_check", "lmms", "audio digital lmms estación trabajo", "audio_expander"},
        {"spotify_check", "spotify", "spotify música streaming", "audio_expander"},

        // Communications
        {"whatsapp_check", "zapzap", "zapzap whatsapp linux", "mail_expander"},
        {"telegram_check", "telegram-desktop", "mensajería telegram", "mail_expander"},
        {"element_check", "element-desktop", "mensajería matrix element", "mail_expander"},
        {"discord_check", "discord", "comunicación discord gamers", "mail_expander"},
        {"thunderbird_check", "thunderbird", "correo thunderbird mozilla", "mail_expander"},
        {"signal_check", "signal-desktop", "mensajería signal privada", "mail_expander"},
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
        {"android_studio_check", "android-studio", "ide android studio google", "developers_expander"},
        {"netbeans_check", "netbeans", "ide netbeans apache", "developers_expander"},

        // Office
        {"libreoffice_check", "libreoffice-fresh", "suite ofimática libreoffice", "office_expander"},
        {"onlyoffice_check", "onlyoffice-bin", "suite ofimática onlyoffice", "office_expander"},
        {"wps_check", "wps-office", "suite ofimática wps", "office_expander"},
        {"abiword_check", "abiword", "procesador textos abiword", "office_expander"},
        {"calibre_check", "calibre", "libros electrónicos calibre", "office_expander"},
        {"papers_check", "papers", "visor documentos papers pdf gnome", "office_expander"},
        {"okular_check", "okular", "visor documentos okular kde", "office_expander"},
        {"paperwork_check", "paperwork", "gestor documentos paperwork ocr", "office_expander"},

        // Gaming
        {"steam_check", "steam", "gaming steam videojuegos", "gamming_expander"},
        {"lutris_check", "lutris", "gaming lutris linux", "gamming_expander"},
        {"heroic_check", "heroic-games-launcher-bin", "launcher heroic epic gog", "gamming_expander"},
        {"bottles_check", "bottles", "wine bottles gestor", "gamming_expander"},
        {"proton_ge_check", "proton-ge-custom-bin", "proton ge custom compatibilidad", "gamming_expander"},
        {"winetricks_check", "winetricks", "wine bibliotecas helper", "gamming_expander"},
        {"gamemode_check", "gamemode", "optimización rendimiento juegos", "gamming_expander"},
        {"mangohud_check", "mangohud", "overlay información sistema juegos", "gamming_expander"},
        {"gnome_games_check", "gnome-games", "juegos gnome colección", "gamming_expander"},
        {"retroarch_check", "retroarch", "emulador frontend retro", "gamming_expander"},
        {"ppsspp_check", "ppsspp", "emulador playstation portable psp", "gamming_expander"},
        {"duckstation_check", "duckstation", "emulador playstation 1 ps1", "gamming_expander"},
        {"pcsx2_check", "pcsx2", "emulador playstation 2 ps2", "gamming_expander"},
        {"rpcs3_check", "rpcs3", "emulador playstation 3 ps3", "gamming_expander"},
        {"shadps4_check", "shadps4", "emulador playstation 4 ps4", "gamming_expander"},
        {"snes9x_gtk_check", "snes9x-gtk", "emulador super nintendo snes", "gamming_expander"},
        {"zsnes_check", "zsnes", "emulador super nintendo clásico", "gamming_expander"},
        {"citron_check", "citron", "emulador nintendo switch basado yuzu", "gamming_expander"},
        {"ryujinx_check", "ryujinx", "emulador Switch 1 Emulator", "gamming_expander"},
        {"dolphin_emu_check", "dolphin-emu", "emulador gamecube wii", "gamming_expander"},
        {"mesen2_check", "mesen2-git", "emulador nes snes game boy", "gamming_expander"},
        {"fceux_check", "fceux", "emulador nintendo nes", "gamming_expander"},
        {"bsnes_qt5_check", "bsnes-qt5", "emulador super nintendo precisión", "gamming_expander"},
        {"mgba_qt_check", "mgba-qt", "emulador game boy advance", "gamming_expander"},
        {"skyemu_check", "skyemu-git", "emulador game boy advance", "gamming_expander"},
        {"azahar_check", "azahar", "emulador nintendo ds", "gamming_expander"},
        {"melonds_check", "melonds", "emulador nintendo ds precisión", "gamming_expander"},
        {"mame_check", "mame", "emulador arcade máquinas", "gamming_expander"},
        {"wine_check", "wine", "compatibilidad wine windows", "gamming_expander"},
        {"playonlinux_check", "playonlinux", "wine playonlinux frontend", "gamming_expander"},
        {"supertuxkart_check", "supertuxkart", "juego carreras karts 3d", "gamming_expander"},
        {"supertux_check", "supertux", "juego supertux plataformas", "gamming_expander"},

        // Others
        {"octopi_check", "octopi", "gestor paquetes octopi pacman", "other_expander"},
        {"pamac_check", "pamac-aur", "gestor paquetes pamac aur", "other_expander"},
        {"gnome_boxes_check", "gnome-boxes", "virtualización boxes gnome", "other_expander"},
        {"virt_manager_check", "qemu-full", "virtualización virt manager qemu", "other_expander"},
        {"virtualbox_check", "virtualbox", "virtualización virtualbox", "other_expander"},
        {"genymotion_check", "genymotion", "emulador android genymotion", "other_expander"},
        {"gufw_check", "gufw", "firewall gufw ufw", "other_expander"},
        {"brasero_check", "brasero", "grabación cd dvd brasero", "other_expander"},
        {"transmission_check", "transmission-gtk", "torrent transmission", "other_expander"},
        {"filezilla_check", "filezilla", "ftp filezilla cliente", "other_expander"},
        {"putty_check", "putty", "ssh putty cliente", "other_expander"},
        {"stremio_check", "stremio", "multimedia streaming stremio", "other_expander"},
        {"mission_center_check", "mission-center", "monitor sistema mission center", "other_expander"},
        {"resources_check", "resources", "monitor sistema resources gnome simple", "other_expander"},
        {"htop_check", "htop", "monitor procesos htop", "other_expander"},
        {"bottom_check", "bottom", "monitor sistema procesos bottom rust", "other_expander"},
        {"btop_check", "btop", "monitor recursos btop tui avanzado", "other_expander"},
        {"vim_check", "vim", "editor vim texto configurable", "other_expander"},
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

    // Si no se encontró, insertar justo después de UTILITIES_ENABLED=
    if (!found) {
        gchar **lines2 = g_strsplit(new_content->str, "\n", -1);
        GString *fixed = g_string_new("");
        gboolean inserted = FALSE;
        for (int i = 0; lines2[i] != NULL; i++) {
            g_string_append_printf(fixed, "%s\n", lines2[i]);
            if (!inserted && g_str_has_prefix(g_strstrip(lines2[i]), "UTILITIES_ENABLED=")) {
                g_string_append_printf(fixed, "\n# Utilities apps seleccionadas por el usuario\n%s\n",
                                       array_content->str);
                inserted = TRUE;
            }
        }
        if (!inserted)
            g_string_append_printf(fixed, "\n# Utilities apps seleccionadas por el usuario\n%s\n",
                                   array_content->str);
        g_string_assign(new_content, fixed->str);
        g_string_free(fixed, TRUE);
        g_strfreev(lines2);
    }

    // Guardar archivo actualizado
    vars_trim_trailing_newlines(new_content);
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
        {"mpv_check", "mpv"},
        {"celluloid_check", "celluloid"},
        {"showtime_check", "showtime"},
        {"kooha_check", "kooha"},
        {"vokoscreen_check", "vokoscreen"},

        // Audio
        {"audacious_check", "audacious"},
        {"decibels_check", "decibels"},
        {"clementine_check", "clementine-git"},
        {"audacity_check", "audacity"},
        {"ardour_check", "ardour"},
        {"elisa_check", "elisa"},
        {"euphonica_check", "euphonica"},
        {"lmms_check", "lmms"},
        {"spotify_check", "spotify"},

        // Communications
        {"whatsapp_check", "zapzap"},
        {"telegram_check", "telegram-desktop"},
        {"element_check", "element-desktop"},
        {"discord_check", "discord"},
        {"thunderbird_check", "thunderbird"},
        {"signal_check", "signal-desktop"},
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
        {"android_studio_check", "android-studio"},
        {"netbeans_check", "netbeans"},

        // Office
        {"libreoffice_check", "libreoffice-fresh"},
        {"onlyoffice_check", "onlyoffice-bin"},
        {"wps_check", "wps-office"},
        {"abiword_check", "abiword"},
        {"calibre_check", "calibre"},
        {"papers_check", "papers"},
        {"okular_check", "okular"},
        {"paperwork_check", "paperwork"},

        // Gaming
        {"steam_check", "steam"},
        {"lutris_check", "lutris"},
        {"heroic_check", "heroic-games-launcher-bin"},
        {"bottles_check", "bottles"},
        {"proton_ge_check", "proton-ge-custom-bin"},
        {"winetricks_check", "winetricks"},
        {"gamemode_check", "gamemode"},
        {"mangohud_check", "mangohud"},
        {"gnome_games_check", "gnome-games"},
        {"retroarch_check", "retroarch"},
        {"ppsspp_check", "ppsspp"},
        {"duckstation_check", "duckstation"},
        {"pcsx2_check", "pcsx2"},
        {"rpcs3_check", "rpcs3"},
        {"shadps4_check", "shadps4"},
        {"snes9x_gtk_check", "snes9x-gtk"},
        {"zsnes_check", "zsnes"},
        {"citron_check", "citron"},
        {"ryujinx_check", "ryujinx"},
        {"dolphin_emu_check", "dolphin-emu"},
        {"mesen2_check", "mesen2-git"},
        {"fceux_check", "fceux"},
        {"bsnes_qt5_check", "bsnes-qt5"},
        {"mgba_qt_check", "mgba-qt"},
        {"skyemu_check", "skyemu-git"},
        {"azahar_check", "azahar"},
        {"melonds_check", "melonds"},
        {"mame_check", "mame"},
        {"wine_check", "wine"},
        {"playonlinux_check", "playonlinux"},
        {"supertuxkart_check", "supertuxkart"},
        {"supertux_check", "supertux"},

        // Others
        {"octopi_check", "octopi"},
        {"pamac_check", "pamac-aur"},
        {"gnome_boxes_check", "gnome-boxes"},
        {"virt_manager_check", "qemu-full"},
        {"virtualbox_check", "virtualbox"},
        {"genymotion_check", "genymotion"},
        {"gufw_check", "gufw"},
        {"brasero_check", "brasero"},
        {"gparted_check", "gparted"},
        {"gnome_disk_utility_check", "gnome-disk-utility"},
        {"transmission_check", "transmission-gtk"},
        {"filezilla_check", "filezilla"},
        {"putty_check", "putty"},
        {"stremio_check", "stremio"},
        {"mission_center_check", "mission-center"},
        {"resources_check", "resources"},
        {"htop_check", "htop"},
        {"bottom_check", "bottom"},
        {"btop_check", "btop"},
        {"vim_check", "vim"},
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
        {"mpv_check", "mpv"},
        {"celluloid_check", "celluloid"},
        {"showtime_check", "showtime"},
        {"kooha_check", "kooha"},
        {"vokoscreen_check", "vokoscreen"},

        // Audio
        {"audacious_check", "audacious"},
        {"decibels_check", "decibels"},
        {"clementine_check", "clementine-git"},
        {"audacity_check", "audacity"},
        {"ardour_check", "ardour"},
        {"elisa_check", "elisa"},
        {"euphonica_check", "euphonica"},
        {"lmms_check", "lmms"},
        {"spotify_check", "spotify"},

        // Communications
        {"whatsapp_check", "zapzap"},
        {"telegram_check", "telegram-desktop"},
        {"element_check", "element-desktop"},
        {"discord_check", "discord"},
        {"thunderbird_check", "thunderbird"},
        {"signal_check", "signal-desktop"},
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
        {"android_studio_check", "android-studio"},
        {"netbeans_check", "netbeans"},

        // Office
        {"libreoffice_check", "libreoffice-fresh"},
        {"onlyoffice_check", "onlyoffice-bin"},
        {"wps_check", "wps-office"},
        {"abiword_check", "abiword"},
        {"calibre_check", "calibre"},
        {"papers_check", "papers"},
        {"okular_check", "okular"},
        {"paperwork_check", "paperwork"},

        // Gaming
        {"steam_check", "steam"},
        {"lutris_check", "lutris"},
        {"heroic_check", "heroic-games-launcher-bin"},
        {"bottles_check", "bottles"},
        {"proton_ge_check", "proton-ge-custom-bin"},
        {"winetricks_check", "winetricks"},
        {"gamemode_check", "gamemode"},
        {"mangohud_check", "mangohud"},
        {"gnome_games_check", "gnome-games"},
        {"retroarch_check", "retroarch"},
        {"ppsspp_check", "ppsspp"},
        {"duckstation_check", "duckstation"},
        {"pcsx2_check", "pcsx2"},
        {"rpcs3_check", "rpcs3"},
        {"shadps4_check", "shadps4"},
        {"snes9x_gtk_check", "snes9x-gtk"},
        {"zsnes_check", "zsnes"},
        {"citron_check", "citron"},
        {"ryujinx_check", "ryujinx"},
        {"dolphin_emu_check", "dolphin-emu"},
        {"mesen2_check", "mesen2-git"},
        {"fceux_check", "fceux"},
        {"bsnes_qt5_check", "bsnes-qt5"},
        {"mgba_qt_check", "mgba-qt"},
        {"skyemu_check", "skyemu-git"},
        {"azahar_check", "azahar"},
        {"melonds_check", "melonds"},
        {"mame_check", "mame"},
        {"wine_check", "wine"},
        {"playonlinux_check", "playonlinux"},
        {"supertuxkart_check", "supertuxkart"},
        {"supertux_check", "supertux"},

        // Others
        {"octopi_check", "octopi"},
        {"pamac_check", "pamac-aur"},
        {"gnome_boxes_check", "gnome-boxes"},
        {"virt_manager_check", "qemu-full"},
        {"virtualbox_check", "virtualbox"},
        {"genymotion_check", "genymotion"},
        {"gufw_check", "gufw"},
        {"brasero_check", "brasero"},
        {"gparted_check", "gparted"},
        {"gnome_disk_utility_check", "gnome-disk-utility"},
        {"transmission_check", "transmission-gtk"},
        {"filezilla_check", "filezilla"},
        {"putty_check", "putty"},
        {"stremio_check", "stremio"},
        {"mission_center_check", "mission-center"},
        {"resources_check", "resources"},
        {"htop_check", "htop"},
        {"bottom_check", "bottom"},
        {"btop_check", "btop"},
        {"vim_check", "vim"},
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

static void set_row_subtitle(WindowAppsData *data, const char *check_id,
                             const char *es, const char *en, const char *ru)
{
    GObject *check = gtk_builder_get_object(data->builder, check_id);
    if (!check) return;
    GtkWidget *w = gtk_widget_get_parent(GTK_WIDGET(check));
    while (w && !ADW_IS_ACTION_ROW(w))
        w = gtk_widget_get_parent(w);
    if (w)
        adw_action_row_set_subtitle(ADW_ACTION_ROW(w), i18n_t(es, en, ru));
}

void window_apps_update_language(WindowAppsData *data)
{
    if (!data || !data->builder) return;

    if (data->close_button)
        gtk_button_set_label(data->close_button,
            i18n_t("Cerrar"));
    if (data->save_button)
        gtk_button_set_label(data->save_button,
            i18n_t("Guardar"));
    if (data->window_title)
        adw_window_title_set_title(data->window_title,
            i18n_t("Utilidades"));
    if (data->packages_group)
        adw_preferences_group_set_title(data->packages_group,
            i18n_t("Categorías de Paquetes"));
    if (data->search_entry)
        gtk_search_entry_set_placeholder_text(data->search_entry,
            i18n_t("Busca tu aplicación"));

    // Expanders - títulos y subtítulos de categorías
    if (data->graphics_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->graphics_expander),
            i18n_t("Gráficos"));
        adw_expander_row_set_subtitle(data->graphics_expander,
            i18n_t("Herramientas de diseño y edición"));
    }
    if (data->video_expander) {
        adw_expander_row_set_subtitle(data->video_expander,
            i18n_t("Reproductores, editores, transcodificadores y grabadores de video"));
    }
    if (data->audio_expander) {
        adw_expander_row_set_subtitle(data->audio_expander,
            i18n_t("Editores y reproductores de Audio"));
    }
    if (data->mail_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->mail_expander),
            i18n_t("Comunicaciones"));
        adw_expander_row_set_subtitle(data->mail_expander,
            i18n_t("Clientes de correo electrónico y Chat"));
    }
    if (data->developers_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->developers_expander),
            i18n_t("Desarrollo"));
        adw_expander_row_set_subtitle(data->developers_expander,
            i18n_t("IDE's y Herramientas para developers"));
    }
    if (data->office_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->office_expander),
            i18n_t("Ofimática"));
        adw_expander_row_set_subtitle(data->office_expander,
            i18n_t("Productividad y documentos"));
    }
    if (data->gamming_expander) {
        adw_expander_row_set_subtitle(data->gamming_expander,
            i18n_t("Plataformas, emuladores, herramientas y juegos"));
    }
    if (data->other_expander) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(data->other_expander),
            i18n_t("Otros"));
        adw_expander_row_set_subtitle(data->other_expander,
            i18n_t("Aplicaciones diversas"));
    }

    // --- Subtítulos de filas individuales ---

    // Browsers
    set_row_subtitle(data, "chrome_check",
        "Navegador web rápido, seguro y gratuito de Google",
        "Fast, secure and free web browser by Google",
        "Быстрый, безопасный и бесплатный браузер от Google");
    set_row_subtitle(data, "brave_check",
        "Navegador web privado que bloquea anuncios y rastreadores por defecto",
        "Privacy-focused browser that blocks ads and trackers by default",
        "Браузер с защитой конфиденциальности, блокировкой рекламы и трекеров");
    set_row_subtitle(data, "chromium_check",
        "Proyecto de navegador web de código abierto que sirve como base para Google Chrome",
        "Open-source web browser project that serves as the base for Google Chrome",
        "Браузер с открытым исходным кодом, основа для Google Chrome");
    set_row_subtitle(data, "firefox_check",
        "Navegador web libre y de código abierto desarrollado por Mozilla",
        "Free and open-source web browser developed by Mozilla",
        "Свободный браузер с открытым кодом от Mozilla");
    set_row_subtitle(data, "opera_check",
        "Navegador web con VPN gratuita, bloqueador de anuncios y herramientas de productividad",
        "Web browser with free VPN, ad blocker and productivity tools",
        "Браузер с бесплатным VPN, блокировщиком рекламы и инструментами продуктивности");

    // Graphics
    set_row_subtitle(data, "gimp_check",
        "Programa de manipulación de imágenes GNU, editor de imágenes libre y de código abierto",
        "GNU Image Manipulation Program, free and open-source image editor",
        "Программа обработки изображений GNU, свободный редактор с открытым кодом");
    set_row_subtitle(data, "inkscape_check",
        "Editor de gráficos vectoriales libre y de código abierto",
        "Free and open-source vector graphics editor",
        "Свободный редактор векторной графики с открытым кодом");
    set_row_subtitle(data, "krita_check",
        "Programa de pintura digital libre y de código abierto",
        "Free and open-source digital painting program",
        "Свободная программа для цифровой живописи с открытым кодом");
    set_row_subtitle(data, "pinta_check",
        "Programa de edición y pintura de imágenes simple y fácil de usar",
        "Simple and easy-to-use image editing and painting program",
        "Простая программа для редактирования и рисования изображений");
    set_row_subtitle(data, "blender_check",
        "Suite de creación 3D libre que incluye modelado, animación, renderizado y más",
        "Free 3D creation suite including modeling, animation, rendering and more",
        "Свободный пакет для 3D: моделирование, анимация, рендеринг и многое другое");
    set_row_subtitle(data, "darktable_check",
        "Aplicación de fotografía y flujo de trabajo de imagen RAW de código abierto",
        "Open-source photography and RAW image workflow application",
        "Приложение для фотографии и обработки RAW с открытым кодом");
    set_row_subtitle(data, "freecad_check",
        "Modelador CAD 3D paramétrico libre y de código abierto",
        "Free and open-source parametric 3D CAD modeler",
        "Свободный параметрический 3D CAD-моделировщик");
    set_row_subtitle(data, "ristretto_check",
        "Visor de imágenes rápido y ligero para el entorno de escritorio Xfce",
        "Fast and lightweight image viewer for the Xfce desktop environment",
        "Быстрый и лёгкий просмотрщик изображений для Xfce");
    set_row_subtitle(data, "viewnior_check",
        "Visor de imágenes elegante y simple con interfaz de usuario minimalista",
        "Elegant and simple image viewer with minimalist user interface",
        "Элегантный и простой просмотрщик изображений с минималистичным интерфейсом");

    // Video
    set_row_subtitle(data, "baka_check",
        "Un reproductor multimedia libre, multiplataforma, basado en libmpv",
        "A free, cross-platform multimedia player based on libmpv",
        "Свободный кроссплатформенный медиаплеер на основе libmpv");
    set_row_subtitle(data, "dragon_check",
        "Un reproductor multimedia donde el foco está en la simplicidad, en lugar de características",
        "A multimedia player focused on simplicity over features",
        "Медиаплеер с упором на простоту, а не на функциональность");
    set_row_subtitle(data, "mpv_check",
        "Reproductor multimedia libre, de código abierto y multiplataforma",
        "Free, open-source and cross-platform multimedia player",
        "Свободный кроссплатформенный медиаплеер с открытым кодом");
    set_row_subtitle(data, "celluloid_check",
        "Interfaz GTK simple para el reproductor multimedia mpv",
        "Simple GTK frontend for the mpv media player",
        "Простой GTK-интерфейс для медиаплеера mpv");
    set_row_subtitle(data, "showtime_check",
        "Reproductor de vídeo para el escritorio GNOME",
        "Video player for the GNOME desktop",
        "Видеоплеер для рабочего стола GNOME");
    set_row_subtitle(data, "smplayer_check",
        "Reproductor multimedia con códecs incorporados que puede reproducir prácticamente todos los formatos de video y audio",
        "Multimedia player with built-in codecs that can play virtually all video and audio formats",
        "Медиаплеер со встроенными кодеками для воспроизведения практически всех форматов");
    set_row_subtitle(data, "vlc_check",
        "Reproductor multiplataforma MPEG, VCD/DVD y DivX",
        "Cross-platform MPEG, VCD/DVD and DivX player",
        "Кроссплатформенный MPEG, VCD/DVD и DivX плеер");
    set_row_subtitle(data, "kdenlive_check",
        "Un editor de video no lineal para Linux usando el framework de video MLT",
        "A non-linear video editor for Linux using the MLT video framework",
        "Нелинейный видеоредактор для Linux на основе MLT");
    set_row_subtitle(data, "openshot_check",
        "Un galardonado editor de video libre y de código abierto",
        "An award-winning free and open-source video editor",
        "Отмеченный наградами свободный видеоредактор с открытым кодом");
    set_row_subtitle(data, "pitivi_check",
        "Editor para proyectos de audio/video usando el framework GStreamer",
        "Editor for audio/video projects using the GStreamer framework",
        "Редактор аудио/видео проектов на основе GStreamer");
    set_row_subtitle(data, "shotcut_check",
        "Editor de video multiplataforma basado en Qt",
        "Cross-platform video editor based on Qt",
        "Кроссплатформенный видеоредактор на Qt");
    set_row_subtitle(data, "handbrake_check",
        "Transcodificador de video de código abierto",
        "Open-source video transcoder",
        "Видеотранскодер с открытым исходным кодом");
    set_row_subtitle(data, "obs_check",
        "Software libre y de código abierto para streaming en vivo y grabación",
        "Free and open-source software for live streaming and recording",
        "Свободное ПО для прямых трансляций и записи экрана");
    set_row_subtitle(data, "kooha_check",
        "Grabador de pantalla elegantemente diseñado construido con GTK",
        "Elegantly designed screen recorder built with GTK",
        "Элегантная программа записи экрана на GTK");
    set_row_subtitle(data, "vokoscreen_check",
        "Grabador de pantalla fácil de usar para Linux con soporte para múltiples formatos",
        "Easy-to-use screen recorder for Linux with support for multiple formats",
        "Простая программа записи экрана для Linux с поддержкой множества форматов");

    // Audio
    set_row_subtitle(data, "audacious_check",
        "Reproductor de audio libre y ligero con soporte para muchos formatos",
        "Free and lightweight audio player with support for many formats",
        "Свободный и лёгкий аудиоплеер с поддержкой множества форматов");
    set_row_subtitle(data, "decibels_check",
        "Reproductor de sonido simple que reproduce archivos de sonido sin bibliotecas",
        "Simple sound player that plays sound files without external libraries",
        "Простой аудиоплеер без внешних зависимостей");
    set_row_subtitle(data, "clementine_check",
        "Reproductor y organizador de música moderno multiplataforma",
        "Modern and cross-platform music player and organizer",
        "Современный кроссплатформенный музыкальный плеер и органайзер");
    set_row_subtitle(data, "audacity_check",
        "Editor de audio libre y multiplataforma",
        "Free and cross-platform audio editor",
        "Свободный кроссплатформенный аудиоредактор");
    set_row_subtitle(data, "ardour_check",
        "Estación de trabajo de audio digital profesional",
        "Professional digital audio workstation",
        "Профессиональная цифровая аудиостанция");
    set_row_subtitle(data, "lmms_check",
        "Estación de trabajo de audio digital libre y multiplataforma",
        "Free and cross-platform digital audio workstation",
        "Свободная кроссплатформенная цифровая аудиостанция");
    set_row_subtitle(data, "elisa_check",
        "Reproductor de música simple y elegante por KDE",
        "Simple and elegant music player by KDE",
        "Простой и элегантный музыкальный плеер от KDE");
    set_row_subtitle(data, "euphonica_check",
        "Reproductor de audio avanzado con funciones profesionales",
        "Advanced audio player with professional features",
        "Расширенный аудиоплеер с профессиональными функциями");
    set_row_subtitle(data, "spotify_check",
        "Lanzador de Spotify para Linux",
        "Spotify launcher for Linux",
        "Лаунчер Spotify для Linux");

    // Communications
    set_row_subtitle(data, "whatsapp_check",
        "WhatsApp nativo para Linux",
        "Native WhatsApp for Linux",
        "Нативный WhatsApp для Linux");
    set_row_subtitle(data, "telegram_check",
        "Aplicación de mensajería instantánea rápida y segura",
        "Fast and secure instant messaging application",
        "Быстрый и безопасный мессенджер");
    set_row_subtitle(data, "element_check",
        "Cliente seguro de mensajería y colaboración basado en Matrix",
        "Secure messaging and collaboration client based on Matrix",
        "Безопасный клиент для общения и совместной работы на Matrix");
    set_row_subtitle(data, "discord_check",
        "Plataforma de comunicación para comunidades y gamers",
        "Communication platform for communities and gamers",
        "Платформа общения для сообществ и геймеров");
    set_row_subtitle(data, "thunderbird_check",
        "Cliente de correo electrónico libre de Mozilla",
        "Free email client from Mozilla",
        "Свободный почтовый клиент от Mozilla");
    set_row_subtitle(data, "signal_check",
        "Mensajería privada con cifrado de extremo a extremo",
        "Private messaging with end-to-end encryption",
        "Приватный мессенджер со сквозным шифрованием");
    set_row_subtitle(data, "evolution_check",
        "Cliente de correo y organizador personal de GNOME",
        "GNOME email client and personal organizer",
        "Почтовый клиент и персональный органайзер GNOME");
    set_row_subtitle(data, "fractal_check",
        "Cliente Matrix para GNOME escrito en Rust",
        "Matrix client for GNOME written in Rust",
        "Клиент Matrix для GNOME, написанный на Rust");

    // Development
    set_row_subtitle(data, "vscode_check",
        "Editor de código fuente desarrollado por Microsoft",
        "Source code editor developed by Microsoft",
        "Редактор исходного кода от Microsoft");
    set_row_subtitle(data, "vscodium_check",
        "Versión libre de VS Code sin telemetría de Microsoft",
        "Free version of VS Code without Microsoft telemetry",
        "Свободная версия VS Code без телеметрии Microsoft");
    set_row_subtitle(data, "zed_check",
        "Editor de código colaborativo de alto rendimiento",
        "High-performance collaborative code editor",
        "Высокопроизводительный совместный редактор кода");
    set_row_subtitle(data, "geany_check",
        "IDE ligero usando GTK con características básicas",
        "Lightweight IDE using GTK with basic features",
        "Лёгкая IDE на GTK с базовыми функциями");
    set_row_subtitle(data, "sublime_check",
        "Editor de texto sofisticado para código, marcado y prosa",
        "Sophisticated text editor for code, markup and prose",
        "Функциональный текстовый редактор для кода, разметки и прозы");
    set_row_subtitle(data, "emacs_check",
        "Editor de texto extensible, personalizable y autodocumentado",
        "Extensible, customizable and self-documenting text editor",
        "Расширяемый настраиваемый самодокументирующий текстовый редактор");
    set_row_subtitle(data, "docker_check",
        "Plataforma de contenedores para desarrollar, enviar y ejecutar aplicaciones",
        "Container platform for developing, shipping and running applications",
        "Платформа контейнеризации для разработки и запуска приложений");
    set_row_subtitle(data, "pycharm_check",
        "IDE para desarrollo en Python por JetBrains",
        "IDE for Python development by JetBrains",
        "IDE для разработки на Python от JetBrains");
    set_row_subtitle(data, "intellij_check",
        "IDE para desarrollo en Java por JetBrains",
        "IDE for Java development by JetBrains",
        "IDE для разработки на Java от JetBrains");
    set_row_subtitle(data, "android_studio_check",
        "IDE oficial para desarrollo Android por Google",
        "Official IDE for Android development by Google",
        "Официальная IDE для разработки Android от Google");
    set_row_subtitle(data, "netbeans_check",
        "IDE de código abierto para Java, PHP, C++ y más",
        "Open-source IDE for Java, PHP, C++ and more",
        "IDE с открытым кодом для Java, PHP, C++ и других языков");

    // Office
    set_row_subtitle(data, "libreoffice_check",
        "Suite ofimática libre completa compatible con Microsoft Office",
        "Complete free office suite compatible with Microsoft Office",
        "Полный свободный офисный пакет, совместимый с Microsoft Office");
    set_row_subtitle(data, "onlyoffice_check",
        "Suite ofimática con alta compatibilidad con Microsoft Office",
        "Office suite with high compatibility with Microsoft Office",
        "Офисный пакет с высокой совместимостью с Microsoft Office");
    set_row_subtitle(data, "wps_check",
        "Suite ofimática con interfaz moderna y compatibilidad con MS Office",
        "Office suite with modern interface and MS Office compatibility",
        "Офисный пакет с современным интерфейсом и совместимостью с MS Office");
    set_row_subtitle(data, "abiword_check",
        "Procesador de textos libre y ligero",
        "Free and lightweight word processor",
        "Свободный и лёгкий текстовый процессор");
    set_row_subtitle(data, "calibre_check",
        "Gestor y lector de libros electrónicos completo",
        "Complete ebook manager and reader",
        "Полноценный менеджер и читалка электронных книг");
    set_row_subtitle(data, "papers_check",
        "Visor de documentos moderno para GNOME que soporta PDF y más",
        "Modern document viewer for GNOME supporting PDF and more",
        "Современный просмотрщик документов для GNOME с поддержкой PDF");
    set_row_subtitle(data, "okular_check",
        "Visor universal de documentos de KDE",
        "KDE universal document viewer",
        "Универсальный просмотрщик документов KDE");
    set_row_subtitle(data, "paperwork_check",
        "Gestor de documentos personales con OCR",
        "Personal document manager with OCR",
        "Персональный менеджер документов с OCR");

    // Gaming
    set_row_subtitle(data, "steam_check",
        "Plataforma de distribución digital de videojuegos",
        "Digital video game distribution platform",
        "Платформа цифрового распространения видеоигр");
    set_row_subtitle(data, "lutris_check",
        "Plataforma de gaming libre para Linux",
        "Free gaming platform for Linux",
        "Свободная игровая платформа для Linux");
    set_row_subtitle(data, "heroic_check",
        "Launcher alternativo para Epic Games Store y GOG",
        "Alternative launcher for Epic Games Store and GOG",
        "Альтернативный лаунчер для Epic Games Store и GOG");
    set_row_subtitle(data, "bottles_check",
        "Gestor de prefijos de Wine fácil de usar",
        "Easy-to-use Wine prefix manager",
        "Удобный менеджер префиксов Wine");
    set_row_subtitle(data, "wine_check",
        "Capa de compatibilidad para ejecutar aplicaciones Windows en Linux",
        "Compatibility layer for running Windows applications on Linux",
        "Слой совместимости для запуска Windows-приложений в Linux");
    set_row_subtitle(data, "playonlinux_check",
        "Frontend gráfico para Wine con scripts automáticos",
        "Graphical frontend for Wine with automatic scripts",
        "Графический интерфейс для Wine с автоматическими скриптами");
    set_row_subtitle(data, "supertuxkart_check",
        "Juego de carreras de karts 3D de código abierto",
        "Open-source 3D kart racing game",
        "Гонки на картах 3D с открытым кодом");
    set_row_subtitle(data, "supertux_check",
        "Juego de plataformas 2D inspirado en Super Mario Bros",
        "2D platform game inspired by Super Mario Bros",
        "2D-платформер, вдохновлённый Super Mario Bros");
    set_row_subtitle(data, "proton_ge_check",
        "Versión personalizada de Proton con parches adicionales para mejor compatibilidad",
        "Custom Proton version with additional patches for better compatibility",
        "Кастомная версия Proton с дополнительными патчами для совместимости");
    set_row_subtitle(data, "winetricks_check",
        "Script helper para instalar bibliotecas necesarias en Wine",
        "Helper script to install libraries needed in Wine",
        "Вспомогательный скрипт для установки библиотек в Wine");
    set_row_subtitle(data, "gamemode_check",
        "Optimización temporal del sistema para mejorar el rendimiento en juegos",
        "Temporary system optimization to improve gaming performance",
        "Временная оптимизация системы для улучшения производительности в играх");
    set_row_subtitle(data, "mangohud_check",
        "Overlay de información del sistema para juegos basado en Vulkan y OpenGL",
        "System information overlay for games based on Vulkan and OpenGL",
        "Оверлей системной информации для игр на Vulkan и OpenGL");
    set_row_subtitle(data, "gnome_games_check",
        "Colección de juegos simples para el escritorio GNOME",
        "Collection of simple games for the GNOME desktop",
        "Коллекция простых игр для рабочего стола GNOME");
    set_row_subtitle(data, "retroarch_check",
        "Frontend para emuladores, motores de juego y reproductores multimedia",
        "Frontend for emulators, game engines and media players",
        "Фронтенд для эмуляторов, игровых движков и медиаплееров");
    set_row_subtitle(data, "ppsspp_check",
        "Emulador de PlayStation Portable multiplataforma",
        "Cross-platform PlayStation Portable emulator",
        "Кроссплатформенный эмулятор PlayStation Portable");
    set_row_subtitle(data, "duckstation_check",
        "Emulador de PlayStation 1 con precisión y mejoras gráficas",
        "PlayStation 1 emulator with accuracy and graphical enhancements",
        "Эмулятор PlayStation 1 с точностью и графическими улучшениями");
    set_row_subtitle(data, "pcsx2_check",
        "Emulador de PlayStation 2 de código abierto",
        "Open-source PlayStation 2 emulator",
        "Эмулятор PlayStation 2 с открытым кодом");
    set_row_subtitle(data, "rpcs3_check",
        "Emulador experimental de PlayStation 3 de código abierto",
        "Experimental open-source PlayStation 3 emulator",
        "Экспериментальный эмулятор PlayStation 3 с открытым кодом");
    set_row_subtitle(data, "shadps4_check",
        "Emulador experimental de PlayStation 4 en desarrollo",
        "Experimental PlayStation 4 emulator in development",
        "Экспериментальный эмулятор PlayStation 4 в разработке");
    set_row_subtitle(data, "snes9x_gtk_check",
        "Emulador de Super Nintendo con interfaz GTK",
        "Super Nintendo emulator with GTK interface",
        "Эмулятор Super Nintendo с интерфейсом GTK");
    set_row_subtitle(data, "zsnes_check",
        "Emulador clásico de Super Nintendo Entertainment System",
        "Classic Super Nintendo Entertainment System emulator",
        "Классический эмулятор Super Nintendo");
    set_row_subtitle(data, "citron_check",
        "Emulador de Nintendo Switch basado en yuzu",
        "Nintendo Switch emulator based on yuzu",
        "Эмулятор Nintendo Switch на основе yuzu");
    set_row_subtitle(data, "ryujinx_check",
        "Emulador experimental de Nintendo Switch de código abierto",
        "Experimental open-source Nintendo Switch emulator",
        "Экспериментальный эмулятор Nintendo Switch с открытым кодом");
    set_row_subtitle(data, "dolphin_emu_check",
        "Emulador de Nintendo GameCube y Wii",
        "Nintendo GameCube and Wii emulator",
        "Эмулятор Nintendo GameCube и Wii");
    set_row_subtitle(data, "mesen2_check",
        "Emulador multi-sistema para NES, SNES, Game Boy y PC Engine",
        "Multi-system emulator for NES, SNES, Game Boy and PC Engine",
        "Многосистемный эмулятор для NES, SNES, Game Boy и PC Engine");
    set_row_subtitle(data, "fceux_check",
        "Emulador de Nintendo Entertainment System (NES)",
        "Nintendo Entertainment System (NES) emulator",
        "Эмулятор Nintendo Entertainment System (NES)");
    set_row_subtitle(data, "bsnes_qt5_check",
        "Emulador de Super Nintendo con alta precisión",
        "High accuracy Super Nintendo emulator",
        "Высокоточный эмулятор Super Nintendo");
    set_row_subtitle(data, "mgba_qt_check",
        "Emulador de Game Boy Advance con interfaz Qt",
        "Game Boy Advance emulator with Qt interface",
        "Эмулятор Game Boy Advance с интерфейсом Qt");
    set_row_subtitle(data, "skyemu_check",
        "Emulador de Game Boy Advance con funciones avanzadas",
        "Game Boy Advance emulator with advanced features",
        "Эмулятор Game Boy Advance с расширенными функциями");
    set_row_subtitle(data, "azahar_check",
        "Emulador de Nintendo DS desarrollado en Rust",
        "Nintendo DS emulator developed in Rust",
        "Эмулятор Nintendo DS, написанный на Rust");
    set_row_subtitle(data, "melonds_check",
        "Emulador de Nintendo DS con alta precisión",
        "High accuracy Nintendo DS emulator",
        "Высокоточный эмулятор Nintendo DS");
    set_row_subtitle(data, "mame_check",
        "Emulador de máquinas arcade y sistemas de computadora vintage",
        "Arcade machine and vintage computer system emulator",
        "Эмулятор аркадных автоматов и ретро-компьютеров");

    // Others
    set_row_subtitle(data, "octopi_check",
        "Frontend gráfico para pacman con notificaciones",
        "Graphical frontend for pacman with notifications",
        "Графический интерфейс для pacman с уведомлениями");
    set_row_subtitle(data, "pamac_check",
        "Gestor de paquetes gráfico para Arch Linux con soporte AUR",
        "Graphical package manager for Arch Linux with AUR support",
        "Графический менеджер пакетов Arch Linux с поддержкой AUR");
    set_row_subtitle(data, "gnome_boxes_check",
        "Aplicación simple de virtualización para GNOME",
        "Simple virtualization application for GNOME",
        "Простое приложение виртуализации для GNOME");
    set_row_subtitle(data, "virt_manager_check",
        "Gestor de máquinas virtuales para KVM/QEMU",
        "Virtual machine manager for KVM/QEMU",
        "Менеджер виртуальных машин для KVM/QEMU");
    set_row_subtitle(data, "virtualbox_check",
        "Hipervisor de virtualización multiplataforma",
        "Cross-platform virtualization hypervisor",
        "Кроссплатформенный гипервизор виртуализации");
    set_row_subtitle(data, "genymotion_check",
        "Emulador de Android rápido y fácil de usar",
        "Fast and easy-to-use Android emulator",
        "Быстрый и удобный эмулятор Android");
    set_row_subtitle(data, "gufw_check",
        "Frontend gráfico para el firewall UFW",
        "Graphical frontend for the UFW firewall",
        "Графический интерфейс для брандмауэра UFW");
    set_row_subtitle(data, "brasero_check",
        "Aplicación de grabación de CD/DVD para GNOME",
        "CD/DVD burning application for GNOME",
        "Приложение записи CD/DVD для GNOME");
    set_row_subtitle(data, "gparted_check",
        "Editor de particiones gráfico libre y de código abierto",
        "Free and open-source graphical partition editor",
        "Свободный графический редактор разделов");
    set_row_subtitle(data, "gnome_disk_utility_check",
        "Utilidad para gestión y configuración de discos para GNOME",
        "Disk management and configuration utility for GNOME",
        "Утилита управления дисками для GNOME");
    set_row_subtitle(data, "transmission_check",
        "Cliente BitTorrent ligero y fácil de usar",
        "Lightweight and easy-to-use BitTorrent client",
        "Лёгкий и удобный BitTorrent-клиент");
    set_row_subtitle(data, "filezilla_check",
        "Cliente FTP, FTPS y SFTP multiplataforma",
        "Cross-platform FTP, FTPS and SFTP client",
        "Кроссплатформенный FTP, FTPS и SFTP клиент");
    set_row_subtitle(data, "putty_check",
        "Cliente SSH, Telnet y rlogin",
        "SSH, Telnet and rlogin client",
        "Клиент SSH, Telnet и rlogin");
    set_row_subtitle(data, "stremio_check",
        "Centro multimedia moderno para video streaming",
        "Modern media center for video streaming",
        "Современный медиацентр для потокового видео");
    set_row_subtitle(data, "mission_center_check",
        "Monitor del sistema nativo para GNOME",
        "Native system monitor for GNOME",
        "Нативный системный монитор для GNOME");
    set_row_subtitle(data, "resources_check",
        "Monitor del sistema simple y limpio para GNOME",
        "Simple and clean system monitor for GNOME",
        "Простой и чистый системный монитор для GNOME");
    set_row_subtitle(data, "htop_check",
        "Visor de procesos interactivo para sistemas Unix",
        "Interactive process viewer for Unix systems",
        "Интерактивный просмотрщик процессов для Unix");
    set_row_subtitle(data, "bottom_check",
        "Monitor de sistema y procesos multiplataforma escrito en Rust",
        "Cross-platform system and process monitor written in Rust",
        "Кроссплатформенный монитор системы и процессов на Rust");
    set_row_subtitle(data, "btop_check",
        "Monitor de recursos con interfaz TUI avanzada",
        "Resource monitor with advanced TUI interface",
        "Монитор ресурсов с расширенным TUI-интерфейсом");
    set_row_subtitle(data, "vim_check",
        "Editor de texto altamente configurable para edición eficiente",
        "Highly configurable text editor for efficient editing",
        "Высоконастраиваемый текстовый редактор для эффективного редактирования");
    set_row_subtitle(data, "neovim_check",
        "Fork moderno de Vim centrado en extensibilidad",
        "Modern fork of Vim focused on extensibility",
        "Современный форк Vim с упором на расширяемость");
    set_row_subtitle(data, "timeshift_check",
        "Herramienta de backup del sistema tipo Time Machine",
        "Time Machine-like system backup tool",
        "Инструмент резервного копирования системы в стиле Time Machine");
    set_row_subtitle(data, "keepassxc_check",
        "Gestor de contraseñas multiplataforma y de código abierto",
        "Cross-platform and open-source password manager",
        "Кроссплатформенный менеджер паролей с открытым кодом");
}
