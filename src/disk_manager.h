#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include <udisks/udisks.h>

// Estructura simple para información del disco
typedef struct {
    gchar *device_path;     // Ruta del dispositivo (/dev/sda, etc.)
    gchar *display_text;    // Texto para mostrar (/dev/sda - 500 GB)
} DiskInfo;

// Estructura principal del DiskManager
typedef struct {
    // Widgets de la interfaz
    AdwComboRow *disk_combo;
    GtkButton *refresh_button;
    AdwToastOverlay *toast_overlay;
    
    // UDisks2 client
    UDisksClient *udisks_client;
    
    // Modelos de datos para el combo
    GtkStringList *disk_store;      // Para mostrar en el combo
    GtkStringList *disk_paths;      // Para almacenar las rutas reales
    
    // Variable para el disco seleccionado
    gchar *selected_disk_path;
    
} DiskManager;

// Funciones principales
DiskManager* disk_manager_new(void);
void disk_manager_free(DiskManager *manager);

// Funciones de inicialización
gboolean disk_manager_init(DiskManager *manager, GtkBuilder *builder);
gboolean disk_manager_setup_udisks(DiskManager *manager);

// Funciones para manejar discos
void disk_manager_populate_list(DiskManager *manager);
void disk_manager_refresh(DiskManager *manager);
const gchar* disk_manager_get_selected_disk(DiskManager *manager);

// Funciones para guardar/cargar variables
gboolean disk_manager_save_to_variables(DiskManager *manager);
gboolean disk_manager_load_from_variables(DiskManager *manager);

// Funciones auxiliares
void disk_info_free(DiskInfo *disk_info);
gboolean disk_manager_is_main_device(const gchar *device_name);

// Callbacks para señales
void on_disk_manager_selection_changed(GObject *object, GParamSpec *pspec, gpointer user_data);
void on_disk_manager_refresh_clicked(GtkButton *button, gpointer user_data);

#endif /* DISK_MANAGER_H */