#ifndef WINDOW_HARDWARE_H
#define WINDOW_HARDWARE_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Enumeraciones para tipos de drivers
typedef enum {
    VIDEO_DRIVER_OPEN_SOURCE = 0,
    VIDEO_DRIVER_NVIDIA_LINUX,
    VIDEO_DRIVER_NVIDIA_LTS,
    VIDEO_DRIVER_NVIDIA_DKMS,
    VIDEO_DRIVER_NVIDIA_470,
    VIDEO_DRIVER_NVIDIA_390,
    VIDEO_DRIVER_AMD_PRIVATE,
    VIDEO_DRIVER_INTEL_NEW,
    VIDEO_DRIVER_INTEL_OLD,
    VIDEO_DRIVER_VIRTUAL_MACHINE
} VideoDriverType;

typedef enum {
    AUDIO_DRIVER_ALSA = 0,
    AUDIO_DRIVER_PIPEWIRE,
    AUDIO_DRIVER_PULSEAUDIO,
    AUDIO_DRIVER_JACK2
} AudioDriverType;

typedef enum {
    WIFI_DRIVER_NONE = 0,
    WIFI_DRIVER_OPEN_SOURCE,
    WIFI_DRIVER_BROADCOM_WL,
    WIFI_DRIVER_REALTEK
} WifiDriverType;

typedef enum {
    BLUETOOTH_DRIVER_NONE = 0,
    BLUETOOTH_DRIVER_BLUETOOTHCTL,
    BLUETOOTH_DRIVER_BLUEMAN
} BluetoothDriverType;

// Estructura para información de hardware
typedef struct _HardwareInfo {
    char *graphics_card_name;
    char *audio_card_name;
    char *wifi_card_name;
    char *bluetooth_card_name;
} HardwareInfo;

// Estructura para datos de la ventana de hardware
typedef struct _WindowHardwareData {
    GtkWindow *window;
    GtkBuilder *builder;

    // Botones de la ventana
    GtkButton *close_button;
    GtkButton *save_button;

    // Grupos de preferencias
    AdwPreferencesGroup *video_group;
    AdwPreferencesGroup *audio_group;
    AdwPreferencesGroup *network_group;

    // ComboRows para selección de drivers
    AdwComboRow *driver_video_combo;
    AdwComboRow *driver_sonido_combo;
    AdwComboRow *driver_wifi_combo;
    AdwComboRow *driver_bluetooth_combo;

    // Estados actuales de drivers
    VideoDriverType current_video_driver;
    AudioDriverType current_audio_driver;
    WifiDriverType current_wifi_driver;
    BluetoothDriverType current_bluetooth_driver;

    // Información de hardware detectado
    HardwareInfo *hardware_info;

    // Estado de inicialización
    gboolean is_initialized;

} WindowHardwareData;

// Funciones principales de la ventana
WindowHardwareData* window_hardware_new(void);
void window_hardware_init(WindowHardwareData *data);
void window_hardware_cleanup(WindowHardwareData *data);
void window_hardware_show(WindowHardwareData *data, GtkWindow *parent);
void window_hardware_hide(WindowHardwareData *data);

// Funciones de configuración
void window_hardware_setup_widgets(WindowHardwareData *data);
void window_hardware_connect_signals(WindowHardwareData *data);
void window_hardware_load_widgets_from_builder(WindowHardwareData *data);

// Función para auto-seleccionar índice 0 en driver_video_combo
void window_hardware_auto_select_video_driver_index_0(WindowHardwareData *data);

// Funciones de detección de hardware
HardwareInfo* window_hardware_detect_hardware(void);
void window_hardware_free_hardware_info(HardwareInfo *info);
char* window_hardware_get_graphics_card_info(void);
char* window_hardware_get_audio_card_info(void);
char* window_hardware_get_wifi_card_info(void);
char* window_hardware_get_bluetooth_card_info(void);

// Funciones de actualización de UI
void window_hardware_update_hardware_descriptions(WindowHardwareData *data);
void window_hardware_update_video_description(WindowHardwareData *data, const char *graphics_card);
void window_hardware_update_audio_description(WindowHardwareData *data, const char *audio_card);

// Funciones de gestión de drivers
VideoDriverType window_hardware_get_selected_video_driver(WindowHardwareData *data);
AudioDriverType window_hardware_get_selected_audio_driver(WindowHardwareData *data);
WifiDriverType window_hardware_get_selected_wifi_driver(WindowHardwareData *data);
BluetoothDriverType window_hardware_get_selected_bluetooth_driver(WindowHardwareData *data);

void window_hardware_set_video_driver(WindowHardwareData *data, VideoDriverType driver);
void window_hardware_set_audio_driver(WindowHardwareData *data, AudioDriverType driver);
void window_hardware_set_wifi_driver(WindowHardwareData *data, WifiDriverType driver);
void window_hardware_set_bluetooth_driver(WindowHardwareData *data, BluetoothDriverType driver);

// Funciones de nombres de drivers
const char* window_hardware_get_video_driver_name(VideoDriverType driver);
const char* window_hardware_get_audio_driver_name(AudioDriverType driver);
const char* window_hardware_get_wifi_driver_name(WifiDriverType driver);
const char* window_hardware_get_bluetooth_driver_name(BluetoothDriverType driver);

// Funciones de persistencia (variables.sh)
gboolean window_hardware_load_from_variables(WindowHardwareData *data);
gboolean window_hardware_save_to_variables(WindowHardwareData *data);
gboolean window_hardware_save_driver_variables(WindowHardwareData *data);
gboolean window_hardware_init_default_variables(void);

// Función para inicialización automática al inicio de la aplicación
gboolean window_hardware_init_auto_variables(void);

// Callbacks de botones
void on_hardware_close_button_clicked(GtkButton *button, gpointer user_data);
void on_hardware_save_button_clicked(GtkButton *button, gpointer user_data);

// Callbacks de combo boxes
void on_video_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);
void on_audio_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);
void on_wifi_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);
void on_bluetooth_driver_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);

// Funciones de utilidad
gboolean window_hardware_is_valid_video_driver(VideoDriverType driver);
gboolean window_hardware_is_valid_audio_driver(AudioDriverType driver);
gboolean window_hardware_is_valid_wifi_driver(WifiDriverType driver);
gboolean window_hardware_is_valid_bluetooth_driver(BluetoothDriverType driver);

void window_hardware_reset_to_defaults(WindowHardwareData *data);
void window_hardware_log_driver_change(const char *component, const char *old_driver, const char *new_driver);

// Función para obtener la instancia global
WindowHardwareData* window_hardware_get_instance(void);

#endif /* WINDOW_HARDWARE_H */
