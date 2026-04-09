#include "page2.h"
#include "variables_utils.h"
#include "config.h"
#include "internet.h"
#include "i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libsoup/soup.h>
#include <gio/gio.h>

// Variable global para datos de la página 2
static Page2Data *g_page2_data = NULL;

// Tabla de mapeo país → layout de teclado
typedef struct {
    const gchar *country_code;
    const gchar *x11_layout;
    const gchar *tty_keymap;
} CountryKeyboardMap;

static const CountryKeyboardMap country_keyboard_map[] = {
    // === LATINOAMÉRICA HISPANOHABLANTE (latam) ===
    {"MX", "latam", "la-latin1"}, // México
    {"GT", "latam", "la-latin1"}, // Guatemala
    {"BZ", "latam", "la-latin1"}, // Belice
    {"HN", "latam", "la-latin1"}, // Honduras
    {"SV", "latam", "la-latin1"}, // El Salvador
    {"NI", "latam", "la-latin1"}, // Nicaragua
    {"CR", "latam", "la-latin1"}, // Costa Rica
    {"PA", "latam", "la-latin1"}, // Panamá
    {"CU", "latam", "la-latin1"}, // Cuba
    {"DO", "latam", "la-latin1"}, // República Dominicana
    {"PR", "latam", "la-latin1"}, // Puerto Rico
    {"CO", "latam", "la-latin1"}, // Colombia
    {"VE", "latam", "la-latin1"}, // Venezuela
    {"EC", "latam", "la-latin1"}, // Ecuador
    {"PE", "latam", "la-latin1"}, // Perú
    {"BO", "latam", "la-latin1"}, // Bolivia
    {"PY", "latam", "la-latin1"}, // Paraguay
    {"CL", "latam", "la-latin1"}, // Chile
    {"AR", "latam", "la-latin1"}, // Argentina
    {"UY", "latam", "la-latin1"}, // Uruguay
    {"GQ", "latam", "la-latin1"}, // Guinea Ecuatorial

    // === BRASIL ===
    {"BR", "br",   "br-abnt2"},

    // === ESPAÑA ===
    {"ES", "es",   "es"},

    // === PORTUGAL Y LUSÓFONOS ===
    {"PT", "pt",   "pt-latin1"}, // Portugal
    {"AO", "pt",   "pt-latin1"}, // Angola
    {"MZ", "pt",   "pt-latin1"}, // Mozambique
    {"CV", "pt",   "pt-latin1"}, // Cabo Verde
    {"ST", "pt",   "pt-latin1"}, // Santo Tomé y Príncipe
    {"GW", "pt",   "pt-latin1"}, // Guinea-Bisáu

    // === ANGLOSAJONES ===
    {"US", "us",   "us"}, // Estados Unidos
    {"CA", "us",   "us"}, // Canadá
    {"AU", "us",   "us"}, // Australia
    {"NZ", "us",   "us"}, // Nueva Zelanda
    {"GB", "gb",   "uk"}, // Reino Unido
    {"IE", "gb",   "uk"}, // Irlanda
    {"JM", "us",   "us"}, // Jamaica
    {"TT", "us",   "us"}, // Trinidad y Tobago
    {"BB", "us",   "us"}, // Barbados
    {"BS", "us",   "us"}, // Bahamas
    {"GY", "us",   "us"}, // Guyana
    {"AG", "us",   "us"}, // Antigua y Barbuda
    {"LC", "us",   "us"}, // Santa Lucía
    {"VC", "us",   "us"}, // San Vicente
    {"GD", "us",   "us"}, // Granada
    {"DM", "us",   "us"}, // Dominica
    {"KN", "us",   "us"}, // San Cristóbal
    {"PH", "us",   "us"}, // Filipinas
    {"SG", "us",   "us"}, // Singapur
    {"IN", "us",   "us"}, // India
    {"PK", "us",   "us"}, // Pakistán
    {"BD", "us",   "us"}, // Bangladesh
    {"LK", "us",   "us"}, // Sri Lanka
    {"ZA", "za",   "us"}, // Sudáfrica
    {"NG", "us",   "us"}, // Nigeria
    {"GH", "us",   "us"}, // Ghana
    {"KE", "us",   "us"}, // Kenia
    {"TZ", "us",   "us"}, // Tanzania
    {"UG", "us",   "us"}, // Uganda
    {"ZW", "us",   "us"}, // Zimbabue
    {"ZM", "us",   "us"}, // Zambia
    {"MW", "us",   "us"}, // Malaui
    {"BW", "us",   "us"}, // Botsuana
    {"NA", "us",   "us"}, // Namibia
    {"SZ", "us",   "us"}, // Esuatini
    {"LS", "us",   "us"}, // Lesoto
    {"SL", "us",   "us"}, // Sierra Leona
    {"LR", "us",   "us"}, // Liberia
    {"GM", "us",   "us"}, // Gambia
    {"SS", "us",   "us"}, // Sudán del Sur
    {"ER", "us",   "us"}, // Eritrea

    // === FRANCÓFONOS ===
    {"FR", "fr",   "fr"}, // Francia
    {"MC", "fr",   "fr"}, // Mónaco
    {"LU", "fr",   "fr"}, // Luxemburgo
    {"HT", "fr",   "fr"}, // Haití
    {"CI", "fr",   "fr"}, // Costa de Marfil
    {"SN", "fr",   "fr"}, // Senegal
    {"ML", "fr",   "fr"}, // Mali
    {"BF", "fr",   "fr"}, // Burkina Faso
    {"GN", "fr",   "fr"}, // Guinea
    {"NE", "fr",   "fr"}, // Níger
    {"BJ", "fr",   "fr"}, // Benín
    {"TG", "fr",   "fr"}, // Togo
    {"CM", "fr",   "fr"}, // Camerún
    {"CD", "fr",   "fr"}, // Congo (RDC)
    {"CG", "fr",   "fr"}, // Congo
    {"GA", "fr",   "fr"}, // Gabón
    {"MG", "fr",   "fr"}, // Madagascar
    {"KM", "fr",   "fr"}, // Comoras
    {"MU", "fr",   "fr"}, // Mauricio
    {"SC", "fr",   "fr"}, // Seychelles
    {"BI", "fr",   "fr"}, // Burundi
    {"RW", "fr",   "fr"}, // Ruanda
    {"CF", "fr",   "fr"}, // Rep. Centroafricana
    {"TD", "fr",   "fr"}, // Chad
    {"DJ", "fr",   "fr"}, // Yibuti

    // === BÉLGICA (AZERTY) ===
    {"BE", "be",   "be-latin1"},

    // === GERMANOS ===
    {"DE", "de",   "de"}, // Alemania
    {"AT", "de",   "de"}, // Austria
    {"CH", "ch",   "sg-latin1"}, // Suiza
    {"LI", "ch",   "sg-latin1"}, // Liechtenstein

    // === ITALIANOS ===
    {"IT", "it",   "it"}, // Italia
    {"SM", "it",   "it"}, // San Marino
    {"VA", "it",   "it"}, // Vaticano

    // === NEERLANDESES ===
    {"NL", "nl",   "nl"}, // Países Bajos
    {"SR", "nl",   "nl"}, // Surinam

    // === NÓRDICOS ===
    {"SE", "se",   "se-lat6"},   // Suecia
    {"FI", "fi",   "fi-latin1"}, // Finlandia
    {"NO", "no",   "no-latin1"}, // Noruega
    {"DK", "dk",   "dk-latin1"}, // Dinamarca
    {"IS", "is",   "is-latin1"}, // Islandia

    // === EUROPA DEL ESTE ===
    {"RU", "ru",    "ru"},         // Rusia
    {"UA", "ua",    "ua"},         // Ucrania
    {"BY", "by",    "by"},         // Bielorrusia
    {"PL", "pl",    "pl2"},        // Polonia
    {"CZ", "cz",    "cz-lat2"},    // República Checa
    {"SK", "sk",    "sk-qwerty"},  // Eslovaquia
    {"HU", "hu",    "hu"},         // Hungría
    {"RO", "ro",    "ro"},         // Rumanía
    {"MD", "ro",    "ro"},         // Moldavia
    {"BG", "bg",    "bg_pho-utf8"},// Bulgaria
    {"RS", "rs",    "sr-cy"},      // Serbia
    {"BA", "ba",    "us"},         // Bosnia
    {"HR", "hr",    "croat"},      // Croacia
    {"SI", "si",    "slovene"},    // Eslovenia
    {"MK", "mk",    "mk"},         // Macedonia del Norte
    {"AL", "al",    "al"},         // Albania
    {"XK", "rs",    "sr-cy"},      // Kosovo
    {"ME", "rs",    "sr-cy"},      // Montenegro

    // === GRECIA ===
    {"GR", "gr",   "gr"}, // Grecia
    {"CY", "gr",   "gr"}, // Chipre

    // === PAÍSES BÁLTICOS ===
    {"LV", "lv",   "lv"},       // Letonia
    {"LT", "lt",   "lt.iso773"},// Lituania
    {"EE", "ee",   "et"},       // Estonia

    // === CÁUCASO ===
    {"GE", "ge",   "ge"}, // Georgia
    {"AM", "am",   "am"}, // Armenia
    {"AZ", "az",   "az"}, // Azerbaiyán

    // === ASIA CENTRAL ===
    {"KZ", "kz",   "ru"}, // Kazajistán
    {"UZ", "uz",   "ru"}, // Uzbekistán
    {"TM", "tm",   "ru"}, // Turkmenistán
    {"TJ", "tj",   "ru"}, // Tayikistán
    {"KG", "kg",   "ru"}, // Kirguistán

    // === TURQUÍA ===
    {"TR", "tr",   "trq"},

    // === ORIENTE MEDIO ===
    {"IL", "il",   "il"},     // Israel
    {"SA", "ara",  "arabic"}, // Arabia Saudita
    {"AE", "ara",  "arabic"}, // Emiratos Árabes
    {"EG", "ara",  "arabic"}, // Egipto
    {"IQ", "ara",  "arabic"}, // Iraq
    {"JO", "ara",  "arabic"}, // Jordania
    {"KW", "ara",  "arabic"}, // Kuwait
    {"LB", "ara",  "arabic"}, // Líbano
    {"SY", "ara",  "arabic"}, // Siria
    {"YE", "ara",  "arabic"}, // Yemen
    {"OM", "ara",  "arabic"}, // Omán
    {"BH", "ara",  "arabic"}, // Baréin
    {"QA", "ara",  "arabic"}, // Catar
    {"PS", "ara",  "arabic"}, // Palestina
    {"LY", "ara",  "arabic"}, // Libia
    {"TN", "ara",  "arabic"}, // Túnez
    {"DZ", "ara",  "arabic"}, // Argelia
    {"MA", "ara",  "arabic"}, // Marruecos
    {"MR", "ara",  "arabic"}, // Mauritania
    {"SD", "ara",  "arabic"}, // Sudán
    {"SO", "ara",  "arabic"}, // Somalia
    {"IR", "ir",   "ir-ltrans"}, // Irán
    {"AF", "af",   "us"},     // Afganistán

    // === ASIA DEL SUR ===
    {"NP", "us",   "us"}, // Nepal
    {"BT", "us",   "us"}, // Bután
    {"MV", "us",   "us"}, // Maldivas

    // === ASIA DEL ESTE ===
    {"JP", "jp",   "jp"}, // Japón
    {"CN", "cn",   "us"}, // China
    {"TW", "us",   "us"}, // Taiwán
    {"KR", "kr",   "us"}, // Corea del Sur
    {"MN", "us",   "us"}, // Mongolia

    // === SURESTE ASIÁTICO ===
    {"TH", "th",   "th-tis"}, // Tailandia
    {"VN", "vn",   "us"},     // Vietnam
    {"ID", "us",   "us"},     // Indonesia
    {"MY", "us",   "us"},     // Malasia
    {"KH", "us",   "us"},     // Camboya
    {"LA", "us",   "us"},     // Laos
    {"MM", "us",   "us"},     // Myanmar
    {"TL", "us",   "us"},     // Timor-Leste
    {"BN", "us",   "us"},     // Brunéi

    // === OCEANÍA ===
    {"FJ", "us",   "us"}, // Fiyi
    {"PG", "us",   "us"}, // Papúa Nueva Guinea
    {"SB", "us",   "us"}, // Islas Salomón
    {"VU", "us",   "us"}, // Vanuatu
    {"WS", "us",   "us"}, // Samoa
    {"TO", "us",   "us"}, // Tonga
    {"KI", "us",   "us"}, // Kiribati

    {NULL, NULL,   NULL}
};

// Devuelve el layout X11 y keymap TTY para un código de país
static void country_to_keyboard(const gchar *country_code,
                                 const gchar **out_x11,
                                 const gchar **out_tty)
{
    *out_x11 = "us";
    *out_tty = "us";
    if (!country_code) return;

    for (int i = 0; country_keyboard_map[i].country_code != NULL; i++) {
        if (g_strcmp0(country_code, country_keyboard_map[i].country_code) == 0) {
            *out_x11 = country_keyboard_map[i].x11_layout;
            *out_tty = country_keyboard_map[i].tty_keymap;
            return;
        }
    }
}

// Obtiene el idioma desde la API (ej: "es-PE,qu,ay")
static gchar* page2_get_language_from_api(void)
{
    SoupSession *session = soup_session_new();
    SoupMessage *msg = soup_message_new("GET", "https://ipapi.co/languages");
    gchar *language_code = NULL;

    if (msg) {
        GBytes *response_body = soup_session_send_and_read(session, msg, NULL, NULL);
        if (response_body) {
            const char *body_data = g_bytes_get_data(response_body, NULL);
            if (body_data) {
                gchar **languages = g_strsplit(body_data, ",", -1);
                if (languages && languages[0])
                    language_code = g_strstrip(g_strdup(languages[0]));
                g_strfreev(languages);
            }
            g_bytes_unref(response_body);
        }
        g_object_unref(msg);
    }
    g_object_unref(session);
    return language_code;
}

// Obtiene la zona horaria desde la API
static gchar* page2_get_timezone_from_api(void)
{
    SoupSession *session = soup_session_new();
    SoupMessage *msg = soup_message_new("GET", "https://ipapi.co/timezone");
    gchar *timezone_code = NULL;

    if (msg) {
        GBytes *response_body = soup_session_send_and_read(session, msg, NULL, NULL);
        if (response_body) {
            const char *body_data = g_bytes_get_data(response_body, NULL);
            if (body_data) {
                gchar *clean = g_strstrip(g_strdup(body_data));
                if (clean && strlen(clean) > 0)
                    timezone_code = clean;
                else
                    g_free(clean);
            }
            g_bytes_unref(response_body);
        }
        g_object_unref(msg);
    }
    g_object_unref(session);
    return timezone_code;
}

// Función para encontrar y seleccionar automáticamente un elemento en ComboRow
static void auto_select_in_combo_row(AdwComboRow *combo_row, const gchar *search_text)
{
    if (!combo_row || !search_text) return;

    GListModel *model = adw_combo_row_get_model(combo_row);
    if (!model) return;

    guint n_items = g_list_model_get_n_items(model);

    for (guint i = 0; i < n_items; i++) {
        GtkStringObject *item = GTK_STRING_OBJECT(g_list_model_get_item(model, i));
        if (item) {
            const gchar *item_text = gtk_string_object_get_string(item);
            if (item_text && g_str_has_prefix(item_text, search_text)) {
                adw_combo_row_set_selected(combo_row, i);
                g_print("✅ Auto-seleccionado en ComboRow: %s\n", item_text);
                g_object_unref(item);
                return;
            }
            g_object_unref(item);
        }
    }
    g_print("⚠ No se encontró '%s' en ComboRow\n", search_text);
}

// Estructura para pasar datos al hilo de configuración automática
typedef struct {
    gchar *detected_language;
    gchar *detected_timezone;
} AutoConfigData;

// Función que se ejecuta en el hilo principal para actualizar la UI
static gboolean apply_auto_config_to_ui(gpointer user_data)
{
    AutoConfigData *config_data = (AutoConfigData *)user_data;

    if (!g_page2_data) {
        g_free(config_data->detected_language);
        g_free(config_data->detected_timezone);
        g_free(config_data);
        return FALSE;
    }

    // Configurar teclado basándose en el país del idioma detectado
    // ej: "es-PE" → extraer "PE" → buscar en tabla → latam / la-latin1
    if (config_data->detected_language) {
        gchar **parts = g_strsplit(config_data->detected_language, "-", 2);
        if (parts && parts[0] && parts[1]) {
            gchar *country = g_strdup(parts[1]);
            const gchar *x11 = NULL, *tty = NULL;
            country_to_keyboard(country, &x11, &tty);
            LOG_INFO("Idioma: %s → país: %s → teclado: %s / keymap: %s",
                     config_data->detected_language, country, x11, tty);
            auto_select_in_combo_row(g_page2_data->combo_keyboard, x11);
            auto_select_in_combo_row(g_page2_data->combo_keymap, tty);
            g_free(country);
        }
        g_strfreev(parts);

        // Configurar locale: "es-PE" → "es_PE"
        gchar *locale_search = g_strdup(config_data->detected_language);
        for (gchar *p = locale_search; *p; p++) {
            if (*p == '-') *p = '_';
        }
        auto_select_in_combo_row(g_page2_data->combo_locale, locale_search);
        g_free(locale_search);
    }

    // Configurar zona horaria
    if (config_data->detected_timezone) {
        auto_select_in_combo_row(g_page2_data->combo_timezone, config_data->detected_timezone);
        setenv("TZ", config_data->detected_timezone, 1);
        tzset();
        if (g_page2_data->time_label)
            update_time_display(NULL);
    }

    save_combo_selections_to_file();

    g_free(config_data->detected_language);
    g_free(config_data->detected_timezone);
    g_free(config_data);

    return FALSE;
}

// Función que se ejecuta en un hilo separado para obtener configuración automática
static gpointer auto_config_worker_thread(gpointer user_data)
{
    AutoConfigData *config_data = g_malloc0(sizeof(AutoConfigData));

    config_data->detected_language = page2_get_language_from_api();
    config_data->detected_timezone = page2_get_timezone_from_api();

    if (!config_data->detected_language)
        LOG_WARNING("No se pudo detectar el idioma desde API");
    if (!config_data->detected_timezone)
        LOG_WARNING("No se pudo detectar la zona horaria");

    g_idle_add(apply_auto_config_to_ui, config_data);
    return NULL;
}

// Función para configurar automáticamente los ComboRows basándose en el idioma y zona horaria detectados
void auto_configure_combo_rows(void)
{
    if (!g_page2_data) return;

    g_print("🌐 Iniciando configuración automática (modo asíncrono)...\n");

    // Ejecutar la configuración automática en un hilo separado
    GThread *config_thread = g_thread_new("auto-config-thread", auto_config_worker_thread, NULL);

    // Liberar la referencia al hilo (se limpiará automáticamente cuando termine)
    g_thread_unref(config_thread);
}

// Función helper para ejecutar comandos del sistema y llenar listas
gboolean execute_system_command_to_list(const char *command, GtkStringList *list)
{
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        g_warning("Failed to execute command: %s", command);
        return FALSE;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Eliminar el salto de línea al final
        buffer[strcspn(buffer, "\n")] = '\0';
        gtk_string_list_append(list, buffer);
    }

    pclose(fp);
    return TRUE;
}

// Funciones de carga de datos
void page2_load_keyboards(GtkStringList *keyboard_list)
{
    execute_system_command_to_list("localectl list-x11-keymap-layouts", keyboard_list);
}

void page2_load_keymaps(GtkStringList *keymap_list)
{
    execute_system_command_to_list("localectl list-keymaps", keymap_list);
}

void page2_load_timezones(GtkStringList *timezone_list)
{
    execute_system_command_to_list("timedatectl --no-pager list-timezones", timezone_list);
}

void page2_load_locales(GtkStringList *locale_list)
{
    GBytes *resource_data;
    const gchar *resource_content;
    gsize content_length;

    // Cargar el recurso embebido locale.gen
    resource_data = g_resources_lookup_data("/org/gtk/arcris/locale.gen",
                                          G_RESOURCE_LOOKUP_FLAGS_NONE,
                                          NULL);

    if (!resource_data) {
        g_print("❌ Error: No se pudo cargar el recurso locale.gen\n");
        return;
    }

    resource_content = g_bytes_get_data(resource_data, &content_length);

    if (!resource_content) {
        g_bytes_unref(resource_data);
        return;
    }

    // Procesar el contenido línea por línea
    gchar **lines = g_strsplit(resource_content, "\n", -1);

    for (gint i = 0; lines[i] != NULL; i++) {
        gchar *line = g_strstrip(lines[i]);

        // Filtrar líneas que empiecen con '#  ' (comentarios con doble espacio)
        if (g_str_has_prefix(line, "#  ")) {
            continue;
        }

        // Quitar el '#' del inicio si existe
        if (g_str_has_prefix(line, "#")) {
            line = line + 1;
        }

        // Buscar líneas que contengan '.UTF-8 UTF-8'
        if (g_strstr_len(line, -1, ".UTF-8 UTF-8")) {
            // Extraer la primera parte (antes del espacio) - equivalente a awk '{print $1}'
            gchar **parts = g_strsplit(line, " ", 2);
            if (parts && parts[0] && strlen(parts[0]) > 0) {
                gtk_string_list_append(locale_list, parts[0]);
            }
            g_strfreev(parts);
        }
    }

    g_strfreev(lines);
    g_bytes_unref(resource_data);

    g_print("✅ Locales cargados desde recurso embebido\n");
}

// Función helper para configurar ComboRows
void page2_setup_combo_row(AdwComboRow *combo_row, GtkStringList *model,
                           GCallback callback, gpointer user_data)
{
    // Crear expresión para mostrar las cadenas
    GtkExpression *exp = gtk_property_expression_new(
        GTK_TYPE_STRING_OBJECT,
        NULL,
        "string"
    );

    // Configurar el ComboRow
    adw_combo_row_set_model(combo_row, G_LIST_MODEL(model));
    adw_combo_row_set_expression(combo_row, exp);
    adw_combo_row_set_search_match_mode(combo_row, GTK_STRING_FILTER_MATCH_MODE_SUBSTRING);

    // Conectar callback si se proporciona
    if (callback) {
        g_signal_connect(combo_row, "notify::selected-item", callback, user_data);
    }
}

// Callbacks para los ComboRows
void on_keyboard_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *keyboard = gtk_string_object_get_string(selected_item);
        g_print("Teclado seleccionado: %s\n", keyboard);

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

void on_keymap_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *keymap = gtk_string_object_get_string(selected_item);
        g_print("Keymap TTY seleccionado: %s\n", keymap);

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

void on_timezone_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *timezone = gtk_string_object_get_string(selected_item);
        g_print("Zona horaria seleccionada: %s\n", timezone);

        // Aplicar inmediatamente la zona horaria
        setenv("TZ", timezone, 1);
        tzset();

        // Forzar actualización inmediata del tiempo
        if (g_page2_data && g_page2_data->time_label) {
            update_time_display(NULL);
        }

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

void on_locale_selection_changed(AdwComboRow *combo_row, GParamSpec *pspec, gpointer user_data)
{
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(combo_row);

    if (selected_item) {
        const gchar *locale = gtk_string_object_get_string(selected_item);
        g_print("Locale seleccionado: %s\n", locale);

        // Guardar la selección en archivo bash
        save_combo_selections_to_file();
    }
}

// Función para actualizar la hora con zona horaria actual
// Función para guardar las selecciones de ComboRow en archivo bash
static void apply_page2_vars(GString *content, gpointer user_data)
{
    (void)user_data;
    GtkStringObject *item;

    item = adw_combo_row_get_selected_item(g_page2_data->combo_keyboard);
    if (item) vars_upsert(content, "KEYBOARD_LAYOUT", gtk_string_object_get_string(item));

    item = adw_combo_row_get_selected_item(g_page2_data->combo_keymap);
    if (item) vars_upsert(content, "KEYMAP_TTY", gtk_string_object_get_string(item));

    item = adw_combo_row_get_selected_item(g_page2_data->combo_timezone);
    if (item) vars_upsert(content, "TIMEZONE", gtk_string_object_get_string(item));

    item = adw_combo_row_get_selected_item(g_page2_data->combo_locale);
    if (item) vars_upsert(content, "LOCALE", gtk_string_object_get_string(item));
}

void save_combo_selections_to_file(void)
{
    if (!g_page2_data) return;
    LOG_INFO("=== save_combo_selections_to_file INICIADO ===");
    if (!vars_update(apply_page2_vars, NULL))
        LOG_WARNING("No se pudo guardar selecciones de página 2");
    else
        LOG_INFO("=== save_combo_selections_to_file FINALIZADO ===");
}

gboolean update_time_display(gpointer user_data)
{
    if (!g_page2_data || !g_page2_data->time_label || !g_page2_data->combo_timezone) {
        return FALSE; // Detener el timer si no hay datos
    }

    // Obtener la zona horaria seleccionada del ComboRow
    GtkStringObject *selected_item = adw_combo_row_get_selected_item(g_page2_data->combo_timezone);
    const gchar *timezone = NULL;

    if (selected_item) {
        timezone = gtk_string_object_get_string(selected_item);
    }

    // Configurar la zona horaria si está disponible
    if (timezone && strlen(timezone) > 0) {
        setenv("TZ", timezone, 1);
        tzset();
    }

    // Obtener la hora actual
    time_t raw_time;
    struct tm *time_info;
    char time_string[64];

    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_string, sizeof(time_string), "%H:%M:%S - %d/%m/%Y", time_info);

    // Actualizar el label
    gtk_label_set_text(g_page2_data->time_label, time_string);

    return TRUE; // Continuar actualizando
}

// Función para abrir configuración de teclado
void open_keyboard_settings(GtkButton *button, gpointer user_data)
{
    // Abrir la aplicación de configuración de teclado del sistema
    const char* cmd = arcris_get_keyboard_settings_command();
    gchar *full_command = g_strdup_printf("%s &", cmd);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    system(full_command);
#pragma GCC diagnostic pop
    g_free(full_command);
}

// Función auxiliar para abrir la aplicación de visualización de teclado
static gpointer open_tecla_task(gpointer data) {
    if (!g_page2_data || !g_page2_data->combo_keyboard || !g_page2_data->combo_keymap) {
        return NULL;
    }

    // Obtener el String del Row1 (teclado X11 seleccionado)
    GtkStringObject *keyboard_item = adw_combo_row_get_selected_item(g_page2_data->combo_keyboard);
    const gchar *keyboard_layout = NULL;

    // Obtener el String del Row2 (keymap TTY seleccionado)
    GtkStringObject *keymap_item = adw_combo_row_get_selected_item(g_page2_data->combo_keymap);
    const gchar *keymap_tty = NULL;

    if (keyboard_item) {
        keyboard_layout = gtk_string_object_get_string(keyboard_item);
    }

    if (keymap_item) {
        keymap_tty = gtk_string_object_get_string(keymap_item);
    }

    if (keyboard_layout && keymap_tty) {
        // Abrir kbd-layout-viewer5 con el layout seleccionado
        gchar *viewer_command = g_strdup_printf("kbd-layout-viewer5 -l %s &", keyboard_layout);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        system(viewer_command);
#pragma GCC diagnostic pop
        g_free(viewer_command);
        g_print("Abriendo visualización de teclado para: %s\n", keyboard_layout);

        // Ejecutar comandos para cambiar el idioma en el sistema
        gchar *x11_command = g_strdup_printf("sudo setxkbmap %s 2>/dev/null || true", keyboard_layout);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        system(x11_command);
#pragma GCC diagnostic pop
        g_free(x11_command);
        g_print("Configurando teclado X11: %s\n", keyboard_layout);
    }

    return NULL;
}

// Función para abrir la aplicación de visualización de teclado
void open_tecla(GtkButton *button, gpointer user_data)
{
    // Ejecutar en un hilo separado para no bloquear la interfaz
    GThread *thread = g_thread_new("open-tecla-thread", open_tecla_task, NULL);
    g_thread_unref(thread);
}

// Función principal de inicialización de la página 2
void page2_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page2_data = g_malloc0(sizeof(Page2Data));

    // Guardar referencias importantes
    g_page2_data->carousel = carousel;
    g_page2_data->revealer = revealer;

    // Cargar la página 2 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page2.ui");
    GtkWidget *page2 = GTK_WIDGET(gtk_builder_get_object(page_builder, "page2"));

    // Obtener widgets específicos de la página
    g_page2_data->combo_keyboard = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row1"));
    g_page2_data->combo_keymap = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row2"));
    g_page2_data->combo_timezone = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row3"));
    g_page2_data->combo_locale = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "combo2_row4"));
    g_page2_data->tecla_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "tecla"));
    g_page2_data->tecla_button_content = ADW_BUTTON_CONTENT(gtk_builder_get_object(page_builder, "tecla_button_content"));
    g_page2_data->time_label = GTK_LABEL(gtk_builder_get_object(page_builder, "locale_time_label"));
    g_page2_data->status_page = ADW_STATUS_PAGE(gtk_builder_get_object(page_builder, "page2"));
    g_page2_data->group_keyboard = ADW_PREFERENCES_GROUP(gtk_builder_get_object(page_builder, "group_keyboard"));
    g_page2_data->group_timezone = ADW_PREFERENCES_GROUP(gtk_builder_get_object(page_builder, "group_timezone"));
    g_page2_data->group_ubicacion = ADW_PREFERENCES_GROUP(gtk_builder_get_object(page_builder, "group_ubicacion"));

    // Obtener modelos de datos
    g_page2_data->keyboard_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "main_keyboard"));
    g_page2_data->keymap_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "tty_keyboard"));
    g_page2_data->timezone_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "string_timezones"));
    g_page2_data->locale_list = GTK_STRING_LIST(gtk_builder_get_object(page_builder, "locale_list"));

    // Cargar datos en las listas
    page2_load_keyboards(g_page2_data->keyboard_list);
    page2_load_keymaps(g_page2_data->keymap_list);
    page2_load_timezones(g_page2_data->timezone_list);
    page2_load_locales(g_page2_data->locale_list);

    // Configurar ComboRows
    page2_setup_combo_row(g_page2_data->combo_keyboard, g_page2_data->keyboard_list,
                          G_CALLBACK(on_keyboard_selection_changed), g_page2_data);

    page2_setup_combo_row(g_page2_data->combo_keymap, g_page2_data->keymap_list,
                          G_CALLBACK(on_keymap_selection_changed), g_page2_data);

    page2_setup_combo_row(g_page2_data->combo_timezone, g_page2_data->timezone_list,
                          G_CALLBACK(on_timezone_selection_changed), g_page2_data);

    page2_setup_combo_row(g_page2_data->combo_locale, g_page2_data->locale_list,
                          G_CALLBACK(on_locale_selection_changed), g_page2_data);

    // Configuración especial para timezone (seleccionar el segundo elemento por defecto temporalmente)
    adw_combo_row_set_selected(g_page2_data->combo_timezone, 1);

    // Configurar automáticamente los ComboRows basándose en el idioma detectado (asíncrono)
    auto_configure_combo_rows();

    // Conectar señales adicionales
    g_signal_connect(g_page2_data->tecla_button, "clicked", G_CALLBACK(open_tecla), NULL);

    // Iniciar actualización de tiempo cada segundo
    g_timeout_add_seconds(1, update_time_display, NULL);

    // Añadir la página al carousel
    adw_carousel_append(carousel, page2);

    // Liberar el builder de la página
    g_object_unref(page_builder);
}

// Función de limpieza
void page2_cleanup(Page2Data *data)
{
    if (g_page2_data) {
        g_free(g_page2_data);
        g_page2_data = NULL;
    }
}

void page2_update_language(void)
{
    if (!g_page2_data) return;

    if (g_page2_data->status_page) {
        adw_status_page_set_title(g_page2_data->status_page,
            i18n_t("Sistema local", "Local System", "Система"));
        adw_status_page_set_description(g_page2_data->status_page,
            i18n_t("Ingrese una distribución del teclado, Zona Horaria y Localidad.",
                   "Enter a keyboard layout, Timezone and Locale.",
                   "Введите раскладку клавиатуры, часовой пояс и локаль."));
    }
    if (g_page2_data->group_keyboard)
        adw_preferences_group_set_title(g_page2_data->group_keyboard,
            i18n_t("Teclado", "Keyboard", "Клавиатура"));
    if (g_page2_data->group_timezone)
        adw_preferences_group_set_title(g_page2_data->group_timezone,
            i18n_t("Zona Horaria", "Timezone", "Часовой пояс"));
    if (g_page2_data->group_ubicacion)
        adw_preferences_group_set_title(g_page2_data->group_ubicacion,
            i18n_t("Ubicación", "Location", "Местоположение"));
    if (g_page2_data->combo_keyboard)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page2_data->combo_keyboard),
            i18n_t("Idioma del teclado", "Keyboard Language", "Язык клавиатуры"));
    if (g_page2_data->combo_keymap)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page2_data->combo_keymap),
            i18n_t("Teclado en terminal", "TTY Keyboard", "Клавиатура в терминале"));
    if (g_page2_data->combo_timezone) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page2_data->combo_timezone),
            i18n_t("Región", "Region", "Регион"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(g_page2_data->combo_timezone),
            i18n_t("Selecciona tu región para actualizar la hora",
                   "Select your region to update the time",
                   "Выберите регион для обновления времени"));
    }
    if (g_page2_data->combo_locale) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(g_page2_data->combo_locale),
            i18n_t("País", "Country", "Страна"));
        adw_action_row_set_subtitle(ADW_ACTION_ROW(g_page2_data->combo_locale),
            i18n_t("Selecciona tu idioma y País",
                   "Select your language and Country",
                   "Выберите язык и страну"));
    }
    if (g_page2_data->tecla_button_content)
        adw_button_content_set_label(g_page2_data->tecla_button_content,
            i18n_t("_Probar", "_Test", "_Проверить"));
    if (g_page2_data->tecla_button)
        gtk_widget_set_tooltip_text(GTK_WIDGET(g_page2_data->tecla_button),
            i18n_t("Prueba el teclado si es el correcto",
                   "Test if the keyboard layout is correct",
                   "Проверьте правильность раскладки клавиатуры"));
}
