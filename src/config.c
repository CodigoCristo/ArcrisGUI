#include "config.h"
#include <glib.h>

// Definición de los nombres de las páginas del carousel
const char* CAROUSEL_PAGE_NAMES[] = {
    "Verificación de Internet",
    "Configuración del Sistema", 
    "Configuración Adicional",
    "Instalación/Finalización"
};

// Función para obtener el nombre de una página por su índice
const char* arcris_get_page_name(guint page_index)
{
    if (page_index < CAROUSEL_TOTAL_PAGES) {
        return CAROUSEL_PAGE_NAMES[page_index];
    }
    return "Página Desconocida";
}

// Función para validar si un índice de página es válido
gboolean arcris_is_valid_page_index(guint page_index)
{
    return (page_index < CAROUSEL_TOTAL_PAGES);
}

// Función para obtener el estado como string (útil para debugging)
const char* arcris_state_to_string(ArcrisState state)
{
    switch (state) {
        case ARCRIS_STATE_INITIALIZING:
            return "Inicializando";
        case ARCRIS_STATE_CHECKING_INTERNET:
            return "Verificando Internet";
        case ARCRIS_STATE_READY:
            return "Listo";
        case ARCRIS_STATE_CONFIGURING:
            return "Configurando";
        case ARCRIS_STATE_INSTALLING:
            return "Instalando";
        case ARCRIS_STATE_COMPLETED:
            return "Completado";
        case ARCRIS_STATE_ERROR:
            return "Error";
        default:
            return "Estado Desconocido";
    }
}

// Función para obtener el tipo de página como string
const char* arcris_page_type_to_string(CarouselPageType page_type)
{
    switch (page_type) {
        case PAGE_INTERNET_CHECK:
            return "Verificación de Internet";
        case PAGE_SYSTEM_CONFIG:
            return "Configuración del Sistema";
        case PAGE_ADDITIONAL_CONFIG:
            return "Configuración Adicional";
        case PAGE_INSTALLATION:
            return "Instalación";
        default:
            return "Página Desconocida";
    }
}

// Función para obtener la ruta de recurso de una página
const char* arcris_get_page_resource_path(CarouselPageType page_type)
{
    switch (page_type) {
        case PAGE_INTERNET_CHECK:
            return RESOURCE_PATH_PAGE1;
        case PAGE_SYSTEM_CONFIG:
            return RESOURCE_PATH_PAGE2;
        case PAGE_ADDITIONAL_CONFIG:
            return RESOURCE_PATH_PAGE3;
        case PAGE_INSTALLATION:
            return RESOURCE_PATH_PAGE4;
        default:
            return NULL;
    }
}

// Función para verificar si una página es la primera
gboolean arcris_is_first_page(guint page_index)
{
    return (page_index == CAROUSEL_FIRST_PAGE);
}

// Función para verificar si una página es la última
gboolean arcris_is_last_page(guint page_index)
{
    return (page_index == CAROUSEL_LAST_PAGE);
}

// Función para obtener la siguiente página
guint arcris_get_next_page(guint current_page)
{
    if (current_page < CAROUSEL_LAST_PAGE) {
        return current_page + 1;
    }
    return current_page; // Ya estamos en la última página
}

// Función para obtener la página anterior
guint arcris_get_previous_page(guint current_page)
{
    if (current_page > CAROUSEL_FIRST_PAGE) {
        return current_page - 1;
    }
    return current_page; // Ya estamos en la primera página
}

// Función para logging con formato consistente
void arcris_log_page_transition(guint from_page, guint to_page)
{
    if (arcris_is_debug_enabled()) {
        g_print("[CAROUSEL] Transición: %s -> %s (Página %u -> %u)\n",
                arcris_get_page_name(from_page),
                arcris_get_page_name(to_page),
                from_page, to_page);
    }
}

// Función para logging de estados de la aplicación
void arcris_log_state_change(ArcrisState old_state, ArcrisState new_state)
{
    if (arcris_is_debug_enabled()) {
        g_print("[STATE] Cambio de estado: %s -> %s\n",
                arcris_state_to_string(old_state),
                arcris_state_to_string(new_state));
    }
}

// Función para obtener información de la aplicación formateada
gchar* arcris_get_app_info_string(void)
{
    return g_strdup_printf("%s v%s - %s", 
                          ARCRIS_APP_NAME, 
                          ARCRIS_APP_VERSION, 
                          ARCRIS_APP_DESCRIPTION);
}

// Función para verificar si el entorno soporta una funcionalidad
gboolean arcris_check_environment_support(const char* feature)
{
    if (g_strcmp0(feature, "gnome") == 0) {
        return (g_find_program_in_path("gnome-control-center") != NULL);
    } else if (g_strcmp0(feature, "kde") == 0) {
        return (g_find_program_in_path("systemsettings5") != NULL);
    } else if (g_strcmp0(feature, "xfce") == 0) {
        return (g_find_program_in_path("xfce4-keyboard-settings") != NULL);
    }
    
    return FALSE;
}

// Función para obtener el comando de configuración de teclado apropiado
const char* arcris_get_keyboard_settings_command(void)
{
    if (arcris_check_environment_support("gnome")) {
        return APP_KEYBOARD_SETTINGS;
    } else if (arcris_check_environment_support("kde")) {
        return APP_KEYBOARD_SETTINGS_KDE;
    } else if (arcris_check_environment_support("xfce")) {
        return APP_KEYBOARD_SETTINGS_XFCE;
    }
    
    return APP_KEYBOARD_SETTINGS; // Fallback a GNOME
}

// Función de limpieza para strings dinámicos
void arcris_cleanup_string(gchar **string)
{
    if (string && *string) {
        g_free(*string);
        *string = NULL;
    }
}

// Función para validar configuraciones
gboolean arcris_validate_config(void)
{
    gboolean valid = TRUE;
    
    // Verificar que las constantes tienen valores válidos
    if (CAROUSEL_TOTAL_PAGES < 1) {
        LOG_ERROR("CAROUSEL_TOTAL_PAGES debe ser mayor que 0");
        valid = FALSE;
    }
    
    if (CAROUSEL_ANIMATION_DURATION < 0) {
        LOG_ERROR("CAROUSEL_ANIMATION_DURATION no puede ser negativo");
        valid = FALSE;
    }
    
    if (INTERNET_CHECK_TIMEOUT < 1) {
        LOG_ERROR("INTERNET_CHECK_TIMEOUT debe ser al menos 1 segundo");
        valid = FALSE;
    }
    
    if (TIME_UPDATE_INTERVAL < 100) {
        LOG_ERROR("TIME_UPDATE_INTERVAL debe ser al menos 100ms");
        valid = FALSE;
    }
    
    return valid;
}

// Función de inicialización de configuración
gboolean arcris_config_init(void)
{
    LOG_INFO("Inicializando configuración de %s", arcris_get_app_name());
    
    if (!arcris_validate_config()) {
        LOG_ERROR("Error en la validación de configuración");
        return FALSE;
    }
    
    if (arcris_is_debug_enabled()) {
        LOG_INFO("Modo debug habilitado");
        DEBUG_PRINT("Páginas del carousel: %u", CAROUSEL_TOTAL_PAGES);
        DEBUG_PRINT("Duración de animación: %ums", CAROUSEL_ANIMATION_DURATION);
        DEBUG_PRINT("Timeout de internet: %us", INTERNET_CHECK_TIMEOUT);
    }
    
    LOG_INFO("Configuración inicializada correctamente");
    return TRUE;
}