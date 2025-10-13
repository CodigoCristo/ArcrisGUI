#include "window_hardware.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

// Variable global para la instancia
static WindowHardwareData *g_hardware_instance = NULL;

// Using LOG macros from config.h

WindowHardwareData* window_hardware_new(void)
{
    WindowHardwareData *data = g_new0(WindowHardwareData, 1);
    if (!data) {
        LOG_ERROR("No se pudo asignar memoria para WindowHardwareData");
        return NULL;
    }

    // Inicializar valores por defecto
    data->current_video_driver = VIDEO_DRIVER_OPEN_SOURCE;
    data->current_audio_driver = AUDIO_DRIVER_ALSA;
    data->current_wifi_driver = WIFI_DRIVER_NONE;
    data->current_bluetooth_driver = BLUETOOTH_DRIVER_NONE;
    data->is_initialized = FALSE;
    data->hardware_info = NULL;

    LOG_INFO("Nueva instancia de WindowHardwareData creada");
    return data;
}

void window_hardware_init(WindowHardwareData *data)
{
    if (!data) {
        LOG_ERROR("WindowHardwareData es NULL en init");
        return;
    }

    // Cargar el builder desde recursos
    data->builder = gtk_builder_new_from_resource("/org/gtk/arcris/window_hardware.ui");
    if (!data->builder) {
        LOG_ERROR("No se pudo cargar el builder de window_hardware.ui");
        return;
    }

    // Cargar widgets desde el builder
    window_hardware_load_widgets_from_builder(data);

    // Configurar widgets
    window_hardware_setup_widgets(data);

    // Conectar señales
    window_hardware_connect_signals(data);

    // Detectar hardware
    data->hardware_info = window_hardware_detect_hardware();

    // Actualizar descripciones con información de hardware
    window_hardware_update_hardware_descriptions(data);

    // Cargar configuración desde variables
    window_hardware_load_from_variables(data);

    // Auto-seleccionar siempre el índice 0 en driver_video_combo
    window_hardware_auto_select_video_driver_index_0(data);
    LOG_INFO("Auto-selección de driver de video (índice 0) configurada durante inicialización");

    data->is_initialized = TRUE;
    LOG_INFO("WindowHardwareData inicializada correctamente");
}

void window_hardware_load_widgets_from_builder(WindowHardwareData *data)
{
    if (!data || !data->builder) return;

    // Obtener la ventana principal
    data->window = GTK_WINDOW(gtk_builder_get_object(data->builder, "HardwareListWindow"));
    if (!data->window) {
        LOG_WARNING("No se pudo obtener la ventana principal");
    }

    // Obtener botones
    data->close_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "close_button"));
    data->save_button = GTK_BUTTON(gtk_builder_get_object(data->builder, "save_button"));

    // Obtener combo rows
    data->driver_video_combo = ADW_COMBO_ROW(gtk_builder_get_object(data->builder, "driver_video_combo"));
    data->driver_sonido_combo = ADW_COMBO_ROW(gtk_builder_get_object(data->builder, "driver_sonido_combo"));
    data->driver_wifi_combo = ADW_COMBO_ROW(gtk_builder_get_object(data->builder, "driver_wifi_combo"));
    data->driver_bluetooth_combo = ADW_COMBO_ROW(gtk_builder_get_object(data->builder, "driver_bluetooth_combo"));

    // Verificar que se obtuvieron correctamente
    if (!data->close_button) LOG_WARNING("No se pudo obtener close_button");
    if (!data->save_button) LOG_WARNING("No se pudo obtener save_button");
    if (!data->driver_video_combo) LOG_WARNING("No se pudo obtener driver_video_combo");
    if (!data->driver_sonido_combo) LOG_WARNING("No se pudo obtener driver_sonido_combo");
    if (!data->driver_wifi_combo) LOG_WARNING("No se pudo obtener driver_wifi_combo");
    if (!data->driver_bluetooth_combo) LOG_WARNING("No se pudo obtener driver_bluetooth_combo");

    LOG_INFO("Widgets cargados desde builder");
}

void window_hardware_setup_widgets(WindowHardwareData *data)
{
    if (!data) return;

    // Configurar la ventana
    if (data->window) {
        gtk_window_set_title(data->window, "Configuración de Hardware");
        gtk_window_set_modal(data->window, TRUE);
        gtk_window_set_resizable(data->window, FALSE);
    }

    LOG_INFO("Widgets configurados");
}

void window_hardware_connect_signals(WindowHardwareData *data)
{
    if (!data) return;

    // Conectar señales de botones
    if (data->close_button) {
        g_signal_connect(data->close_button, "clicked",
                        G_CALLBACK(on_hardware_close_button_clicked), data);
        LOG_INFO("Señal del botón cerrar conectada");
    }

    if (data->save_button) {
        g_signal_connect(data->save_button, "clicked",
                        G_CALLBACK(on_hardware_save_button_clicked), data);
        LOG_INFO("Señal del botón guardar conectada");
    }

    // Conectar señales de combo boxes
    if (data->driver_video_combo) {
        g_signal_connect(data->driver_video_combo, "notify::selected",
                        G_CALLBACK(on_video_driver_combo_changed), data);
    }

    if (data->driver_sonido_combo) {
        g_signal_connect(data->driver_sonido_combo, "notify::selected",
                        G_CALLBACK(on_audio_driver_combo_changed), data);
    }

    if (data->driver_wifi_combo) {
        g_signal_connect(data->driver_wifi_combo, "notify::selected",
                        G_CALLBACK(on_wifi_driver_combo_changed), data);
    }

    if (data->driver_bluetooth_combo) {
        g_signal_connect(data->driver_bluetooth_combo, "notify::selected",
                        G_CALLBACK(on_bluetooth_driver_combo_changed), data);
    }

    LOG_INFO("Señales de hardware conectadas");
}

void window_hardware_show(WindowHardwareData *data, GtkWindow *parent)
{
    if (!data || !data->window) {
        LOG_ERROR("No se puede mostrar la ventana - datos inválidos");
        return;
    }

    if (parent) {
        gtk_window_set_transient_for(data->window, parent);
    }

    // Actualizar información de hardware antes de mostrar
    if (data->hardware_info) {
        window_hardware_update_hardware_descriptions(data);
    }

    // Auto-seleccionar siempre el índice 0 en driver_video_combo antes de mostrar
    window_hardware_auto_select_video_driver_index_0(data);

    gtk_window_present(data->window);
    LOG_INFO("Ventana de hardware mostrada");
}

void window_hardware_hide(WindowHardwareData *data)
{
    if (!data || !data->window) return;

    gtk_widget_set_visible(GTK_WIDGET(data->window), FALSE);
    LOG_INFO("Ventana de hardware ocultada");
}

HardwareInfo* window_hardware_detect_hardware(void)
{
    HardwareInfo *info = g_new0(HardwareInfo, 1);
    if (!info) {
        LOG_ERROR("No se pudo asignar memoria para HardwareInfo");
        return NULL;
    }

    // Detectar tarjeta gráfica
    info->graphics_card_name = window_hardware_get_graphics_card_info();

    // Detectar tarjeta de audio
    info->audio_card_name = window_hardware_get_audio_card_info();

    // Detectar wifi
    info->wifi_card_name = window_hardware_get_wifi_card_info();

    // Detectar bluetooth
    info->bluetooth_card_name = window_hardware_get_bluetooth_card_info();

    LOG_INFO("Hardware detectado correctamente");
    return info;
}

char* window_hardware_get_graphics_card_info(void)
{
    char *result = NULL;
    FILE *fp;
    char buffer[512];

    // Intentar obtener información con lspci
    fp = popen("lspci | grep -i 'vga' | sed -E 's/.*\\[([^][]+)\\].*/\\1/' | cut -d':' -f3 | sed -e 's/^ //' -e 's/ (rev.*)//'", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            // Remover el salto de línea
            char *newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';
            // El comando sed ya extrae solo el nombre que necesitamos
            if (strlen(buffer) > 0) {
                result = g_strdup(buffer);
            }
        }
        pclose(fp);
    }

    // Si no se pudo obtener información, usar un valor por defecto
    if (!result || strlen(result) == 0) {
        if (result) g_free(result);
        result = g_strdup("Tarjeta gráfica genérica detectada");
    }

    LOG_INFO("Tarjeta gráfica detectada: %s", result);
    return result;
}

char* window_hardware_get_audio_card_info(void)
{
    char *result = NULL;
    FILE *fp;
    char buffer[512];

    // Intentar obtener información con lspci
    fp = popen("lspci | grep -i audio | head -1 | sed -E 's/^[0-9a-f:.]+ Audio device: //; s/ \\(rev [^)]+\\)//'", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            // Remover el salto de línea
            char *newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';
            // El comando sed ya extrae solo el nombre que necesitamos
            if (strlen(buffer) > 0) {
                result = g_strdup(buffer);
            }
        }
        pclose(fp);
    }

    if (!result || strlen(result) == 0) {
        if (result) g_free(result);
        result = g_strdup("Tarjeta de audio genérica detectada");
    }

    LOG_INFO("Tarjeta de audio detectada: %s", result);
    return result;
}

char* window_hardware_get_wifi_card_info(void)
{
    char *result = NULL;
    FILE *fp;
    char buffer[512];

    fp = popen("lspci | grep -i 'network controller' | sed -E 's/^[0-9a-f:.]+ Network controller: //; s/ \\(rev [^)]+\\)//'", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            // Remover el salto de línea
            char *newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';
            // El comando sed ya extrae solo el nombre que necesitamos
            if (strlen(buffer) > 0) {
                result = g_strdup(buffer);
            }
        }
        pclose(fp);
    }

    if (!result || strlen(result) == 0) {
        if (result) g_free(result);
        result = g_strdup("No se detectó tarjeta WiFi");
    }

    return result;
}

char* window_hardware_get_bluetooth_card_info(void)
{
    char *result = NULL;
    FILE *fp;
    char buffer[512];

    fp = popen("lsusb | grep -i bluetooth | sed -E 's/^.*ID [0-9a-f]+:[0-9a-f]+ //'", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            // Remover el salto de línea
            char *newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';
            // El comando sed ya extrae solo el nombre que necesitamos
            if (strlen(buffer) > 0) {
                result = g_strdup(buffer);
            }
        }
        pclose(fp);
    }

    if (!result || strlen(result) == 0) {
        if (result) g_free(result);
        result = g_strdup("No se detectó dispositivo Bluetooth");
    }

    return result;
}

void window_hardware_update_hardware_descriptions(WindowHardwareData *data)
{
    if (!data || !data->hardware_info) return;

    // Actualizar descripción de video
    if (data->hardware_info->graphics_card_name) {
        window_hardware_update_video_description(data, data->hardware_info->graphics_card_name);
    }

    // Actualizar descripción de audio
    if (data->hardware_info->audio_card_name) {
        window_hardware_update_audio_description(data, data->hardware_info->audio_card_name);
    }

    // Actualizar subtítulo de WiFi
    if (data->hardware_info->wifi_card_name && data->driver_wifi_combo) {
        g_object_set(data->driver_wifi_combo, "subtitle", data->hardware_info->wifi_card_name, NULL);
        // Verificar si WiFi no está disponible y deshabilitar
        if (strstr(data->hardware_info->wifi_card_name, "No se detectó") != NULL) {
            gtk_widget_set_sensitive(GTK_WIDGET(data->driver_wifi_combo), FALSE);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(data->driver_wifi_combo), TRUE);
        }
        LOG_INFO("Subtítulo WiFi actualizado: %s", data->hardware_info->wifi_card_name);
    }

    // Actualizar subtítulo de Bluetooth
    if (data->hardware_info->bluetooth_card_name && data->driver_bluetooth_combo) {
        g_object_set(data->driver_bluetooth_combo, "subtitle", data->hardware_info->bluetooth_card_name, NULL);
        // Verificar si Bluetooth no está disponible y deshabilitar
        if (strstr(data->hardware_info->bluetooth_card_name, "No se detectó") != NULL) {
            gtk_widget_set_sensitive(GTK_WIDGET(data->driver_bluetooth_combo), FALSE);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(data->driver_bluetooth_combo), TRUE);
        }
        LOG_INFO("Subtítulo Bluetooth actualizado: %s", data->hardware_info->bluetooth_card_name);
    }

    LOG_INFO("Descripciones de hardware actualizadas");
}

void window_hardware_update_video_description(WindowHardwareData *data, const char *graphics_card)
{
    if (!data || !graphics_card) return;

    // Buscar el grupo de video en el builder
    GObject *video_group = gtk_builder_get_object(data->builder, "video_group");
    if (!video_group) {
        LOG_WARNING("No se pudo encontrar el grupo de video");
        return;
    }

    // Crear el texto con formato y color verde negrita con salto de línea
    char *description_markup = g_strdup_printf(
        "Tu tarjeta Gráfica es:\n<span color='#28b463' weight='bold'>%s</span>",
        graphics_card
    );

    // Actualizar la descripción del grupo
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(video_group), description_markup);

    g_free(description_markup);
    LOG_INFO("Descripción de video actualizada con: %s", graphics_card);
}

void window_hardware_update_audio_description(WindowHardwareData *data, const char *audio_card)
{
    if (!data || !audio_card) return;

    // Buscar el grupo de audio en el builder
    GObject *audio_group = gtk_builder_get_object(data->builder, "audio_group");
    if (!audio_group) {
        LOG_WARNING("No se pudo encontrar el grupo de audio");
        return;
    }

    // Crear el texto con formato con salto de línea
    char *description_markup = g_strdup_printf(
        "Tu tarjeta Audio es:\n<span color='#28b463' weight='bold'>%s</span>",
        audio_card
    );

    // Actualizar la descripción del grupo
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(audio_group), description_markup);

    g_free(description_markup);
    LOG_INFO("Descripción de audio actualizada con: %s", audio_card);
}

// Callbacks de botones
void on_hardware_close_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowHardwareData *data = (WindowHardwareData*)user_data;
    if (!data) return;

    window_hardware_hide(data);
    LOG_INFO("Ventana de hardware cerrada por el usuario");
}

void on_hardware_save_button_clicked(GtkButton *button, gpointer user_data)
{
    WindowHardwareData *data = (WindowHardwareData*)user_data;
    if (!data) return;

    // Guardar configuración
    if (window_hardware_save_to_variables(data)) {
        LOG_INFO("Configuración de hardware guardada correctamente");
    } else {
        LOG_ERROR("Error al guardar configuración de hardware");
    }

    // Cerrar ventana
    window_hardware_hide(data);
}

// Callbacks de combo boxes
void on_video_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    WindowHardwareData *data = (WindowHardwareData*)user_data;
    if (!data) return;

    guint selected = adw_combo_row_get_selected(combo);
    VideoDriverType old_driver = data->current_video_driver;
    data->current_video_driver = (VideoDriverType)selected;

    const char *old_name = window_hardware_get_video_driver_name(old_driver);
    const char *new_name = window_hardware_get_video_driver_name(data->current_video_driver);

    window_hardware_log_driver_change("Video", old_name, new_name);

    // Guardar automáticamente en variables.sh
    window_hardware_save_driver_variables(data);
}

void on_audio_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    WindowHardwareData *data = (WindowHardwareData*)user_data;
    if (!data) return;

    guint selected = adw_combo_row_get_selected(combo);
    AudioDriverType old_driver = data->current_audio_driver;
    data->current_audio_driver = (AudioDriverType)selected;

    const char *old_name = window_hardware_get_audio_driver_name(old_driver);
    const char *new_name = window_hardware_get_audio_driver_name(data->current_audio_driver);

    window_hardware_log_driver_change("Audio", old_name, new_name);

    // Guardar automáticamente en variables.sh
    window_hardware_save_driver_variables(data);
}

void on_wifi_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    WindowHardwareData *data = (WindowHardwareData*)user_data;
    if (!data) return;

    guint selected = adw_combo_row_get_selected(combo);
    WifiDriverType old_driver = data->current_wifi_driver;
    data->current_wifi_driver = (WifiDriverType)selected;

    const char *old_name = window_hardware_get_wifi_driver_name(old_driver);
    const char *new_name = window_hardware_get_wifi_driver_name(data->current_wifi_driver);

    window_hardware_log_driver_change("WiFi", old_name, new_name);

    // Guardar automáticamente en variables.sh
    window_hardware_save_driver_variables(data);
}

void on_bluetooth_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    WindowHardwareData *data = (WindowHardwareData*)user_data;
    if (!data) return;

    guint selected = adw_combo_row_get_selected(combo);
    BluetoothDriverType old_driver = data->current_bluetooth_driver;
    data->current_bluetooth_driver = (BluetoothDriverType)selected;

    const char *old_name = window_hardware_get_bluetooth_driver_name(old_driver);
    const char *new_name = window_hardware_get_bluetooth_driver_name(data->current_bluetooth_driver);

    window_hardware_log_driver_change("Bluetooth", old_name, new_name);

    // Guardar automáticamente en variables.sh
    window_hardware_save_driver_variables(data);
}

// Funciones de nombres de drivers
const char* window_hardware_get_video_driver_name(VideoDriverType driver)
{
    switch (driver) {
        case VIDEO_DRIVER_OPEN_SOURCE: return "Open Source";
        case VIDEO_DRIVER_NVIDIA_LINUX: return "nvidia";
        case VIDEO_DRIVER_NVIDIA_LTS: return "nvidia-lts";
        case VIDEO_DRIVER_NVIDIA_DKMS: return "nvidia-dkms";
        case VIDEO_DRIVER_NVIDIA_470: return "nvidia-470xx-dkms";
        case VIDEO_DRIVER_NVIDIA_390: return "nvidia-390xx-dkms";
        case VIDEO_DRIVER_AMD_PRIVATE: return "AMD Private";
        case VIDEO_DRIVER_INTEL_NEW: return "Intel (Gen 8+)";
        case VIDEO_DRIVER_INTEL_OLD: return "Intel (Gen 2-7)";
        case VIDEO_DRIVER_VIRTUAL_MACHINE: return "Máquina Virtual";
        default: return "Desconocido";
    }
}

const char* window_hardware_get_audio_driver_name(AudioDriverType driver)
{
    switch (driver) {
        case AUDIO_DRIVER_ALSA: return "Alsa Audio";
        case AUDIO_DRIVER_PIPEWIRE: return "pipewire";
        case AUDIO_DRIVER_PULSEAUDIO: return "pulseaudio";
        case AUDIO_DRIVER_JACK2: return "Jack2";
        default: return "Desconocido";
    }
}

const char* window_hardware_get_wifi_driver_name(WifiDriverType driver)
{
    switch (driver) {
        case WIFI_DRIVER_NONE: return "Ninguno";
        case WIFI_DRIVER_OPEN_SOURCE: return "Open Source";
        case WIFI_DRIVER_BROADCOM_WL: return "broadcom-wl";
        case WIFI_DRIVER_REALTEK: return "Realtek";
        default: return "Desconocido";
    }
}

const char* window_hardware_get_bluetooth_driver_name(BluetoothDriverType driver)
{
    switch (driver) {
        case BLUETOOTH_DRIVER_NONE: return "Ninguno";
        case BLUETOOTH_DRIVER_BLUETOOTHCTL: return "bluetoothctl (terminal)";
        case BLUETOOTH_DRIVER_BLUEMAN: return "blueman (Graphical)";
        default: return "Desconocido";
    }
}

// Funciones de persistencia
gboolean window_hardware_load_from_variables(WindowHardwareData *data)
{
    if (!data) return FALSE;

    // Cargar configuración desde variables.sh
    const char *variables_path = "data/variables.sh";
    gchar *config_content = NULL;
    GError *error = NULL;

    if (!g_file_get_contents(variables_path, &config_content, NULL, &error)) {
        if (error) {
            LOG_WARNING("No se pudo cargar variables.sh: %s", error->message);
            g_error_free(error);
        }

        // Establecer valores por defecto si no se puede cargar el archivo
        if (data->driver_video_combo) {
            adw_combo_row_set_selected(data->driver_video_combo, data->current_video_driver);
        }
        if (data->driver_sonido_combo) {
            adw_combo_row_set_selected(data->driver_sonido_combo, data->current_audio_driver);
        }
        if (data->driver_wifi_combo) {
            adw_combo_row_set_selected(data->driver_wifi_combo, data->current_wifi_driver);
        }
        if (data->driver_bluetooth_combo) {
            adw_combo_row_set_selected(data->driver_bluetooth_combo, data->current_bluetooth_driver);
        }

        return FALSE;
    }

    // Parsear las variables del archivo
    gchar **lines = g_strsplit(config_content, "\n", -1);

    for (int i = 0; lines[i] != NULL; i++) {
        char *line = lines[i];

        // Driver de Video
        if (g_str_has_prefix(line, "DRIVER_VIDEO=")) {
            char *value = line + 13; // Saltar "DRIVER_VIDEO="
            if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                value[strlen(value)-1] = 0;
                value++;
            }

            // Convertir string a enum
            if (g_strcmp0(value, "Open Source") == 0) {
                data->current_video_driver = VIDEO_DRIVER_OPEN_SOURCE;
            } else if (g_strcmp0(value, "nvidia") == 0) {
                data->current_video_driver = VIDEO_DRIVER_NVIDIA_LINUX;
            } else if (g_strcmp0(value, "nvidia-lts") == 0) {
                data->current_video_driver = VIDEO_DRIVER_NVIDIA_LTS;
            } else if (g_strcmp0(value, "nvidia-dkms") == 0) {
                data->current_video_driver = VIDEO_DRIVER_NVIDIA_DKMS;
            } else if (g_strcmp0(value, "nvidia-470xx-dkms") == 0) {
                data->current_video_driver = VIDEO_DRIVER_NVIDIA_470;
            } else if (g_strcmp0(value, "nvidia-390xx-dkms") == 0) {
                data->current_video_driver = VIDEO_DRIVER_NVIDIA_390;
            } else if (g_strcmp0(value, "AMD Private") == 0) {
                data->current_video_driver = VIDEO_DRIVER_AMD_PRIVATE;
            } else if (g_strcmp0(value, "Intel (Gen 8+)") == 0) {
                data->current_video_driver = VIDEO_DRIVER_INTEL_NEW;
            } else if (g_strcmp0(value, "Intel (Gen 2-7)") == 0) {
                data->current_video_driver = VIDEO_DRIVER_INTEL_OLD;
            } else if (g_strcmp0(value, "Máquina Virtual") == 0) {
                data->current_video_driver = VIDEO_DRIVER_VIRTUAL_MACHINE;
            }
            LOG_INFO("DRIVER_VIDEO cargado: %s", value);
        }
        // Driver de Audio
        else if (g_str_has_prefix(line, "DRIVER_AUDIO=")) {
            char *value = line + 13; // Saltar "DRIVER_AUDIO="
            if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                value[strlen(value)-1] = 0;
                value++;
            }

            if (g_strcmp0(value, "Alsa Audio") == 0) {
                data->current_audio_driver = AUDIO_DRIVER_ALSA;
            } else if (g_strcmp0(value, "pipewire") == 0) {
                data->current_audio_driver = AUDIO_DRIVER_PIPEWIRE;
            } else if (g_strcmp0(value, "pulseaudio") == 0) {
                data->current_audio_driver = AUDIO_DRIVER_PULSEAUDIO;
            } else if (g_strcmp0(value, "Jack2") == 0) {
                data->current_audio_driver = AUDIO_DRIVER_JACK2;
            }
            LOG_INFO("DRIVER_AUDIO cargado: %s", value);
        }
        // Driver de WiFi
        else if (g_str_has_prefix(line, "DRIVER_WIFI=")) {
            char *value = line + 12; // Saltar "DRIVER_WIFI="
            if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                value[strlen(value)-1] = 0;
                value++;
            }

            if (g_strcmp0(value, "Ninguno") == 0) {
                data->current_wifi_driver = WIFI_DRIVER_NONE;
            } else if (g_strcmp0(value, "Open Source") == 0) {
                data->current_wifi_driver = WIFI_DRIVER_OPEN_SOURCE;
            } else if (g_strcmp0(value, "broadcom-wl") == 0) {
                data->current_wifi_driver = WIFI_DRIVER_BROADCOM_WL;
            } else if (g_strcmp0(value, "Realtek") == 0) {
                data->current_wifi_driver = WIFI_DRIVER_REALTEK;
            }
            LOG_INFO("DRIVER_WIFI cargado: %s", value);
        }
        // Driver de Bluetooth
        else if (g_str_has_prefix(line, "DRIVER_BLUETOOTH=")) {
            char *value = line + 17; // Saltar "DRIVER_BLUETOOTH="
            if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                value[strlen(value)-1] = 0;
                value++;
            }

            if (g_strcmp0(value, "Ninguno") == 0) {
                data->current_bluetooth_driver = BLUETOOTH_DRIVER_NONE;
            } else if (g_strcmp0(value, "bluetoothctl (terminal)") == 0) {
                data->current_bluetooth_driver = BLUETOOTH_DRIVER_BLUETOOTHCTL;
            } else if (g_strcmp0(value, "blueman (Graphical)") == 0) {
                data->current_bluetooth_driver = BLUETOOTH_DRIVER_BLUEMAN;
            }
            LOG_INFO("DRIVER_BLUETOOTH cargado: %s", value);
        }
    }

    // Actualizar la UI con los valores cargados
    if (data->driver_video_combo) {
        adw_combo_row_set_selected(data->driver_video_combo, data->current_video_driver);
    }
    if (data->driver_sonido_combo) {
        adw_combo_row_set_selected(data->driver_sonido_combo, data->current_audio_driver);
    }
    if (data->driver_wifi_combo) {
        adw_combo_row_set_selected(data->driver_wifi_combo, data->current_wifi_driver);
    }
    if (data->driver_bluetooth_combo) {
        adw_combo_row_set_selected(data->driver_bluetooth_combo, data->current_bluetooth_driver);
    }

    g_strfreev(lines);
    g_free(config_content);

    LOG_INFO("Configuración de drivers cargada desde variables.sh");
    return TRUE;
}

// Estructura para pasar datos al callback de timeout del driver de video
typedef struct {
    WindowHardwareData *data;
    int retry_count;
    int max_retries;
} VideoDriverAutoSelectData;

// Callback para auto-selección con timeout del driver de video
static gboolean window_hardware_video_driver_timeout_callback(gpointer user_data)
{
    VideoDriverAutoSelectData *callback_data = (VideoDriverAutoSelectData *)user_data;
    WindowHardwareData *data = callback_data->data;
    
    if (!data || !data->driver_video_combo) {
        LOG_WARNING("Auto-selección driver video fallida: datos no inicializados (intento %d/%d)", 
                   callback_data->retry_count + 1, callback_data->max_retries);
        callback_data->retry_count++;
        
        if (callback_data->retry_count >= callback_data->max_retries) {
            g_free(callback_data);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }

    // Obtener el modelo del combo
    GtkStringList *model = GTK_STRING_LIST(adw_combo_row_get_model(data->driver_video_combo));
    if (!model) {
        LOG_WARNING("Auto-selección driver video fallida: modelo no encontrado (intento %d/%d)", 
                   callback_data->retry_count + 1, callback_data->max_retries);
        callback_data->retry_count++;
        
        if (callback_data->retry_count >= callback_data->max_retries) {
            g_free(callback_data);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }

    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(model));
    
    // Si no hay opciones, continuar esperando
    if (n_items == 0) {
        LOG_INFO("Esperando que se carguen las opciones del driver de video (intento %d/%d)", 
                callback_data->retry_count + 1, callback_data->max_retries);
        callback_data->retry_count++;
        
        if (callback_data->retry_count >= callback_data->max_retries) {
            LOG_WARNING("Timeout: no se encontraron opciones de driver de video después de %d intentos", 
                       callback_data->max_retries);
            g_free(callback_data);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }

    // Seleccionar siempre el índice 0 (primera opción)
    adw_combo_row_set_selected(data->driver_video_combo, 0);
    
    // Actualizar el estado interno
    data->current_video_driver = VIDEO_DRIVER_OPEN_SOURCE; // índice 0
    
    LOG_INFO("Driver de video auto-seleccionado en índice 0");
    LOG_INFO("Total de opciones disponibles: %u", n_items);
    
    // Obtener el texto de la opción seleccionada para logging
    const gchar *selected_option = gtk_string_list_get_string(model, 0);
    if (selected_option) {
        LOG_INFO("Opción seleccionada: %s", selected_option);
    }
    
    g_free(callback_data);
    return G_SOURCE_REMOVE;
}

// Función para auto-seleccionar índice 0 en driver_video_combo
void window_hardware_auto_select_video_driver_index_0(WindowHardwareData *data)
{
    if (!data) {
        LOG_WARNING("No se puede auto-seleccionar driver de video: data es NULL");
        return;
    }
    
    // Crear datos para el callback de timeout
    VideoDriverAutoSelectData *callback_data = g_malloc0(sizeof(VideoDriverAutoSelectData));
    callback_data->data = data;
    callback_data->retry_count = 0;
    callback_data->max_retries = 10; // Máximo 10 intentos (2 segundos total)
    
    // Programar el callback con un pequeño delay para asegurar que la lista esté poblada
    g_timeout_add(200, window_hardware_video_driver_timeout_callback, callback_data); // 200ms de delay inicial
    
    LOG_INFO("Auto-selección de driver de video programada con timeout de 200ms");
}

gboolean window_hardware_save_to_variables(WindowHardwareData *data)
{
    if (!data) return FALSE;

    return window_hardware_save_driver_variables(data);
}

gboolean window_hardware_save_driver_variables(WindowHardwareData *data)
{
    if (!data) return FALSE;

    const char *variables_path = "data/variables.sh";
    gchar *config_content = NULL;
    GError *error = NULL;

    // Leer el archivo actual
    if (!g_file_get_contents(variables_path, &config_content, NULL, &error)) {
        if (error) {
            LOG_ERROR("No se pudo leer variables.sh: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }

    // Buscar y actualizar/agregar las variables de drivers
    gchar **lines = g_strsplit(config_content, "\n", -1);
    GString *new_content = g_string_new("");

    gboolean video_found = FALSE, audio_found = FALSE, wifi_found = FALSE, bluetooth_found = FALSE;

    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "DRIVER_VIDEO=")) {
            g_string_append_printf(new_content, "DRIVER_VIDEO=\"%s\"\n",
                                 window_hardware_get_video_driver_name(data->current_video_driver));
            video_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_AUDIO=")) {
            g_string_append_printf(new_content, "DRIVER_AUDIO=\"%s\"\n",
                                 window_hardware_get_audio_driver_name(data->current_audio_driver));
            audio_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_WIFI=")) {
            g_string_append_printf(new_content, "DRIVER_WIFI=\"%s\"\n",
                                 window_hardware_get_wifi_driver_name(data->current_wifi_driver));
            wifi_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_BLUETOOTH=")) {
            g_string_append_printf(new_content, "DRIVER_BLUETOOTH=\"%s\"\n",
                                 window_hardware_get_bluetooth_driver_name(data->current_bluetooth_driver));
            bluetooth_found = TRUE;
        } else {
            // Mantener otras líneas tal como están, controlando saltos de línea
            g_string_append(new_content, lines[i]);
            if (lines[i + 1] != NULL) {
                g_string_append_c(new_content, '\n');
            }
        }
    }

    // Agregar variables que no existían
    if (!video_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append_printf(new_content, "\n# Driver de Video\nDRIVER_VIDEO=\"%s\"",
                             window_hardware_get_video_driver_name(data->current_video_driver));
    }
    if (!audio_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append_printf(new_content, "\n# Driver de Audio\nDRIVER_AUDIO=\"%s\"",
                             window_hardware_get_audio_driver_name(data->current_audio_driver));
    }
    if (!wifi_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append_printf(new_content, "\n# Driver de WiFi\nDRIVER_WIFI=\"%s\"",
                             window_hardware_get_wifi_driver_name(data->current_wifi_driver));
    }
    if (!bluetooth_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append_printf(new_content, "\n# Driver de Bluetooth\nDRIVER_BLUETOOTH=\"%s\"",
                             window_hardware_get_bluetooth_driver_name(data->current_bluetooth_driver));
    }

    // Escribir el archivo actualizado
    if (!g_file_set_contents(variables_path, new_content->str, -1, &error)) {
        if (error) {
            LOG_ERROR("No se pudo escribir variables.sh: %s", error->message);
            g_error_free(error);
        }
        g_string_free(new_content, TRUE);
        g_strfreev(lines);
        g_free(config_content);
        return FALSE;
    }

    LOG_INFO("Drivers guardados en variables.sh:");
    LOG_INFO("  Video: %s", window_hardware_get_video_driver_name(data->current_video_driver));
    LOG_INFO("  Audio: %s", window_hardware_get_audio_driver_name(data->current_audio_driver));
    LOG_INFO("  WiFi: %s", window_hardware_get_wifi_driver_name(data->current_wifi_driver));
    LOG_INFO("  Bluetooth: %s", window_hardware_get_bluetooth_driver_name(data->current_bluetooth_driver));

    g_string_free(new_content, TRUE);
    g_strfreev(lines);
    g_free(config_content);

    return TRUE;
}

// Funciones de utilidad
void window_hardware_log_driver_change(const char *component, const char *old_driver, const char *new_driver)
{
    if (!component || !old_driver || !new_driver) return;

    LOG_INFO("Driver de %s cambiado: %s -> %s", component, old_driver, new_driver);
}

void window_hardware_free_hardware_info(HardwareInfo *info)
{
    if (!info) return;

    g_free(info->graphics_card_name);
    g_free(info->audio_card_name);
    g_free(info->wifi_card_name);
    g_free(info->bluetooth_card_name);
    g_free(info);
}

void window_hardware_cleanup(WindowHardwareData *data)
{
    if (!data) return;

    if (data->hardware_info) {
        window_hardware_free_hardware_info(data->hardware_info);
        data->hardware_info = NULL;
    }

    if (data->builder) {
        g_object_unref(data->builder);
        data->builder = NULL;
    }

    g_free(data);
    LOG_INFO("WindowHardwareData limpiada");
}

WindowHardwareData* window_hardware_get_instance(void)
{
    if (!g_hardware_instance) {
        g_hardware_instance = window_hardware_new();
        if (g_hardware_instance) {
            window_hardware_init(g_hardware_instance);
        }
    }
    return g_hardware_instance;
}

// Función para inicializar las variables de drivers por defecto al inicio de la aplicación
gboolean window_hardware_init_default_variables(void)
{
    const char *variables_path = "data/variables.sh";
    gchar *config_content = NULL;
    GError *error = NULL;

    // Leer el archivo actual
    if (!g_file_get_contents(variables_path, &config_content, NULL, &error)) {
        if (error) {
            LOG_ERROR("No se pudo leer variables.sh: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }

    // Verificar si las variables ya existen
    gchar **lines = g_strsplit(config_content, "\n", -1);
    GString *new_content = g_string_new("");

    gboolean video_found = FALSE, audio_found = FALSE, wifi_found = FALSE, bluetooth_found = FALSE;

    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "DRIVER_VIDEO=")) {
            video_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_AUDIO=")) {
            audio_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_WIFI=")) {
            wifi_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_BLUETOOTH=")) {
            bluetooth_found = TRUE;
        }

        // Mantener todas las líneas existentes
        g_string_append_printf(new_content, "%s\n", lines[i]);
    }

    // Agregar variables que no existen con valores por defecto
    if (!video_found) {
        g_string_append_printf(new_content, "\n# Driver de Video\nDRIVER_VIDEO=\"Open Source\"\n");
        LOG_INFO("Variable DRIVER_VIDEO inicializada con valor por defecto: Open Source");
    }
    if (!audio_found) {
        g_string_append_printf(new_content, "\n# Driver de Audio\nDRIVER_AUDIO=\"Alsa Audio\"\n");
        LOG_INFO("Variable DRIVER_AUDIO inicializada con valor por defecto: Alsa Audio");
    }
    if (!wifi_found) {
        g_string_append_printf(new_content, "\n# Driver de WiFi\nDRIVER_WIFI=\"Ninguno\"\n");
        LOG_INFO("Variable DRIVER_WIFI inicializada con valor por defecto: Ninguno");
    }
    if (!bluetooth_found) {
        g_string_append_printf(new_content, "\n# Driver de Bluetooth\nDRIVER_BLUETOOTH=\"Ninguno\"\n");
        LOG_INFO("Variable DRIVER_BLUETOOTH inicializada con valor por defecto: Ninguno");
    }

    // Escribir el archivo actualizado solo si se agregaron variables nuevas
    if (!video_found || !audio_found || !wifi_found || !bluetooth_found) {
        if (!g_file_set_contents(variables_path, new_content->str, -1, &error)) {
            if (error) {
                LOG_ERROR("No se pudo escribir variables.sh: %s", error->message);
                g_error_free(error);
            }
            g_string_free(new_content, TRUE);
            g_strfreev(lines);
            g_free(config_content);
            return FALSE;
        }
        LOG_INFO("Variables de drivers de hardware inicializadas en variables.sh");
    } else {
        LOG_INFO("Variables de drivers ya existen en variables.sh");
    }

    g_string_free(new_content, TRUE);
    g_strfreev(lines);
    g_free(config_content);

    return TRUE;
}

// Función para inicialización automática al inicio de la aplicación
gboolean window_hardware_init_auto_variables(void)
{
    const char *variables_path = "data/variables.sh";
    gchar *config_content = NULL;
    GError *error = NULL;

    // Leer el archivo actual
    if (!g_file_get_contents(variables_path, &config_content, NULL, &error)) {
        if (error) {
            LOG_ERROR("No se pudo leer variables.sh: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }

    // Verificar si las variables ya existen
    gchar **lines = g_strsplit(config_content, "\n", -1);
    GString *new_content = g_string_new("");

    gboolean video_found = FALSE, audio_found = FALSE, wifi_found = FALSE, bluetooth_found = FALSE;
    gboolean needs_update = FALSE;

    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "DRIVER_VIDEO=")) {
            video_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_AUDIO=")) {
            audio_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_WIFI=")) {
            wifi_found = TRUE;
        } else if (g_str_has_prefix(lines[i], "DRIVER_BLUETOOTH=")) {
            bluetooth_found = TRUE;
        }

        // Mantener todas las líneas existentes
        g_string_append(new_content, lines[i]);
        if (lines[i + 1] != NULL) {
            g_string_append_c(new_content, '\n');
        }
    }

    // Agregar variables que no existen con valores por defecto
    if (!video_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append(new_content, "\n# Driver de Video\nDRIVER_VIDEO=\"Open Source\"\n");
        LOG_INFO("Variable DRIVER_VIDEO inicializada automáticamente: Open Source");
        needs_update = TRUE;
    }
    if (!audio_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append(new_content, "\n# Driver de Audio\nDRIVER_AUDIO=\"Alsa Audio\"\n");
        LOG_INFO("Variable DRIVER_AUDIO inicializada automáticamente: Alsa Audio");
        needs_update = TRUE;
    }
    if (!wifi_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append(new_content, "\n# Driver de WiFi\nDRIVER_WIFI=\"Ninguno\"\n");
        LOG_INFO("Variable DRIVER_WIFI inicializada automáticamente: Ninguno");
        needs_update = TRUE;
    }
    if (!bluetooth_found) {
        if (new_content->len > 0 && new_content->str[new_content->len - 1] != '\n') {
            g_string_append_c(new_content, '\n');
        }
        g_string_append(new_content, "\n# Driver de Bluetooth\nDRIVER_BLUETOOTH=\"Ninguno\"\n");
        LOG_INFO("Variable DRIVER_BLUETOOTH inicializada automáticamente: Ninguno");
        needs_update = TRUE;
    }

    // Escribir el archivo actualizado solo si se agregaron variables nuevas
    if (needs_update) {
        if (!g_file_set_contents(variables_path, new_content->str, -1, &error)) {
            if (error) {
                LOG_ERROR("No se pudo escribir variables.sh: %s", error->message);
                g_error_free(error);
            }
            g_string_free(new_content, TRUE);
            g_strfreev(lines);
            g_free(config_content);
            return FALSE;
        }
        LOG_INFO("Variables de drivers inicializadas automáticamente al inicio de la aplicación");
    } else {
        LOG_INFO("Variables de drivers ya están presentes - no se requiere inicialización");
    }

    g_string_free(new_content, TRUE);
    g_strfreev(lines);
    g_free(config_content);

    return TRUE;
}
