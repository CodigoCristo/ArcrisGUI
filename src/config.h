#ifndef CONFIG_H
#define CONFIG_H

#include <gtk/gtk.h>

// Información de la aplicación
#define ARCRIS_APP_NAME "Arcris"
#define ARCRIS_APP_ID "org.gtk.arcris"
#define ARCRIS_APP_VERSION "2.0"
#define ARCRIS_APP_DESCRIPTION "Instalador y configurador del sistema Arcris"

// Configuraciones del Carousel
#define CAROUSEL_ANIMATION_DURATION 300  // milisegundos
#define CAROUSEL_TOTAL_PAGES 5
#define CAROUSEL_FIRST_PAGE 0
#define CAROUSEL_LAST_PAGE (CAROUSEL_TOTAL_PAGES - 1)

// Configuraciones de red e internet
#define INTERNET_CHECK_TIMEOUT 2         // segundos para timeout de ping
#define INTERNET_CHECK_INTERVAL 3        // segundos entre verificaciones
#define INTERNET_CHECK_HOST "8.8.8.8"   // servidor para verificar conectividad
#define INTERNET_INITIAL_CHECK_DELAY 1   // segundos antes del primer chequeo



// Comandos del sistema
#define CMD_LIST_KEYBOARDS "localectl list-x11-keymap-layouts"
#define CMD_LIST_KEYMAPS "localectl list-keymaps"
#define CMD_LIST_TIMEZONES "timedatectl --no-pager list-timezones"
#define CMD_LIST_LOCALES "cat ./data/locale.gen | grep -v '#  ' | sed 's/#//g' | grep '.UTF-8 UTF-8' | awk '{print $1}'"
#define CMD_CHECK_INTERNET "ping -c 1 -W " G_STRINGIFY(INTERNET_CHECK_TIMEOUT) " " INTERNET_CHECK_HOST " > /dev/null 2>&1"

// Aplicaciones externas
#define APP_KEYBOARD_SETTINGS "gnome-control-center keyboard"
#define APP_KEYBOARD_SETTINGS_KDE "systemsettings5 kcm_keyboard"
#define APP_KEYBOARD_SETTINGS_XFCE "xfce4-keyboard-settings"

// Configuraciones de tiempo
#define TIME_UPDATE_INTERVAL 1000        // milisegundos para actualizar la hora
#define TIME_FORMAT "%H:%M:%S - %d/%m/%Y"

// Configuraciones de la interfaz
#define WINDOW_DEFAULT_WIDTH 950
#define WINDOW_DEFAULT_HEIGHT 659
#define REVEALER_TRANSITION_DURATION 250  // milisegundos

// Configuraciones de debug
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) g_print("[DEBUG] " fmt "\n", ##__VA_ARGS__)
    #define DEBUG_ENABLED TRUE
#else
    #define DEBUG_PRINT(fmt, ...)
    #define DEBUG_ENABLED FALSE
#endif

// Configuraciones de logging
#define LOG_DOMAIN "Arcris"
#define LOG_INFO(fmt, ...) g_message("[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) g_warning("[WARNING] " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) g_critical("[ERROR] " fmt, ##__VA_ARGS__)

// Tamaños de buffer
#define BUFFER_SIZE_SMALL 64
#define BUFFER_SIZE_MEDIUM 256
#define BUFFER_SIZE_LARGE 1024

// Configuraciones por defecto de ComboRow
#define COMBO_ROW_SEARCH_MODE GTK_STRING_FILTER_MATCH_MODE_SUBSTRING
#define COMBO_ROW_DEFAULT_SELECTION 0
#define TIMEZONE_DEFAULT_SELECTION 1    // Seleccionar el segundo elemento por defecto

// Estados de la aplicación
typedef enum {
    ARCRIS_STATE_INITIALIZING,
    ARCRIS_STATE_CHECKING_INTERNET,
    ARCRIS_STATE_READY,
    ARCRIS_STATE_CONFIGURING,
    ARCRIS_STATE_INSTALLING,
    ARCRIS_STATE_COMPLETED,
    ARCRIS_STATE_ERROR
} ArcrisState;

// Tipos de página del carousel
typedef enum {
    PAGE_INTERNET_CHECK = 0,
    PAGE_SYSTEM_CONFIG = 1,
    PAGE_ADDITIONAL_CONFIG = 2,
    PAGE_INSTALLATION = 3
} CarouselPageType;

// Nombres de las páginas (para debugging y logs)
extern const char* CAROUSEL_PAGE_NAMES[];

// Configuraciones de recursos
#define RESOURCE_PATH_WINDOW "/org/gtk/arcris/window.ui"
#define RESOURCE_PATH_PAGE1 "/org/gtk/arcris/page1.ui"
#define RESOURCE_PATH_PAGE2 "/org/gtk/arcris/page2.ui"
#define RESOURCE_PATH_PAGE3 "/org/gtk/arcris/page3.ui"
#define RESOURCE_PATH_PAGE4 "/org/gtk/arcris/page4.ui"
#define RESOURCE_PATH_ABOUT "/org/gtk/arcris/about.ui"

// Configuraciones de estilos CSS
#define CSS_CLASS_SUGGESTED "suggested-action"
#define CSS_CLASS_DESTRUCTIVE "destructive-action"
#define CSS_CLASS_FLAT "flat"

// Mensajes de usuario
#define MSG_INTERNET_CHECKING "Verificando conexión a Internet..."
#define MSG_INTERNET_CONNECTED "Conexión a Internet establecida"
#define MSG_INTERNET_DISCONNECTED "Sin conexión a Internet"
#define MSG_READY_TO_START "Listo para comenzar"
#define MSG_CONFIGURING_SYSTEM "Configurando sistema..."
#define MSG_INSTALLATION_COMPLETE "¡Instalación completada!"

// Configuraciones de diálogos
#define DIALOG_CLOSE_TITLE "¿Cerrar aplicación?"
#define DIALOG_CLOSE_BODY "¿Estás seguro de que quieres cerrar la aplicación?"
#define DIALOG_CLOSE_CANCEL "Cancelar"
#define DIALOG_CLOSE_CONFIRM "Cerrar"

// Tooltips
#define TOOLTIP_BACK_BUTTON "Ir a la página anterior"
#define TOOLTIP_NEXT_BUTTON "Ir a la página siguiente"
#define TOOLTIP_START_BUTTON "Comenzar configuración"
#define TOOLTIP_KEYBOARD_SETTINGS "Abrir configuración de teclado"

// Funciones helper para configuración
static inline gboolean arcris_is_debug_enabled(void) {
    return DEBUG_ENABLED;
}

static inline const char* arcris_get_app_name(void) {
    return ARCRIS_APP_NAME;
}

static inline const char* arcris_get_app_id(void) {
    return ARCRIS_APP_ID;
}

static inline const char* arcris_get_app_version(void) {
    return ARCRIS_APP_VERSION;
}

// Declaraciones de funciones de configuración
gboolean arcris_config_init(void);
gboolean arcris_validate_config(void);

// Declaraciones de funciones de utilidad
const char* arcris_get_page_name(guint page_index);
gboolean arcris_is_valid_page_index(guint page_index);
const char* arcris_state_to_string(ArcrisState state);
const char* arcris_page_type_to_string(CarouselPageType page_type);
const char* arcris_get_page_resource_path(CarouselPageType page_type);

// Declaraciones de funciones de navegación
gboolean arcris_is_first_page(guint page_index);
gboolean arcris_is_last_page(guint page_index);
guint arcris_get_next_page(guint current_page);
guint arcris_get_previous_page(guint current_page);

// Declaraciones de funciones de logging
void arcris_log_page_transition(guint from_page, guint to_page);
void arcris_log_state_change(ArcrisState old_state, ArcrisState new_state);

// Declaraciones de funciones de información
gchar* arcris_get_app_info_string(void);
gboolean arcris_check_environment_support(const char* feature);
const char* arcris_get_keyboard_settings_command(void);
void arcris_cleanup_string(gchar **string);

#endif /* CONFIG_H */
