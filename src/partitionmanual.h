#ifndef PARTITIONMANUAL_H
#define PARTITIONMANUAL_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include <udisks/udisks.h>
#include "config.h"
#include "page3.h"
#include "disk_manager.h"
#include "partition_manager.h"

// Estructura para información de partición
typedef struct _PartitionInfo {
    gchar *device_path;       // ej: /dev/sda1
    gchar *filesystem;        // ej: ext4, ntfs, etc.
    gchar *mount_point;       // ej: /, /home, etc.
    guint64 size;            // tamaño en bytes
    gchar *size_formatted;    // tamaño formateado: "100 GB"
    gchar *label;            // etiqueta del volumen
    gboolean is_mounted;      // si está montado
    gchar *uuid;             // UUID de la partición
} PartitionInfo;

// Enumeración para las páginas del stack
typedef enum {
    PARTITION_STACK_DISK_SELECTION = 0,  // Vista principal: selección de disco + radiobuttons
    PARTITION_STACK_MOUNT_POINTS = 1     // Vista secundaria: configuración de puntos de montaje
} PartitionStackPage;

// Estructura para datos del particionado manual
typedef struct _PartitionManualData {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // GtkStack para las dos vistas
    GtkStack *partition_stack;
    GtkStackSwitcher *stack_switcher;
    
    // Vista 1: Selección de disco + radiobuttons
    GtkWidget *disk_selection_page;
    GtkLabel *disk_label;
    GtkLabel *disk_size_label;
    GtkButton *refresh_button;
    AdwComboRow *disk_combo;
    
    // Radiobuttons para tipo de particionado
    GtkCheckButton *auto_partition_radio;
    GtkCheckButton *auto_btrfs_radio;
    GtkCheckButton *manual_partition_radio;
    
    // Vista 2: Configuración de puntos de montaje
    GtkWidget *mount_points_page;
    GtkButton *gparted_button;
    AdwPreferencesGroup *partitions_group;
    
    // Información del disco actual
    gchar *current_disk_path;
    gchar *current_disk_size;
    
    // Lista de particiones
    GList *partitions;        // Lista de PartitionInfo*
    
    // Lista de filas de particiones (para poder eliminarlas correctamente)
    GList *partition_rows;    // Lista de AdwActionRow*
    
    // Cliente UDisks2 para obtener información de particiones
    UDisksClient *udisks_client;
    
    // Manejador de particiones
    PartitionManager *partition_manager;
    
    // Estado del stack
    PartitionStackPage current_stack_page;
    
} PartitionManualData;

// Funciones principales del particionado manual
void partitionmanual_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void partitionmanual_cleanup(PartitionManualData *data);

// Funciones de configuración y carga de datos
void partitionmanual_setup_widgets(PartitionManualData *data);
void partitionmanual_load_data(PartitionManualData *data);
void partitionmanual_update_disk_info(PartitionManualData *data);

// Funciones de navegación y validación
gboolean partitionmanual_go_to_next_page(PartitionManualData *data);
gboolean partitionmanual_go_to_previous_page(PartitionManualData *data);
gboolean partitionmanual_is_configuration_valid(void);

// Funciones del GtkStack
void partitionmanual_setup_stack(PartitionManualData *data);
void partitionmanual_switch_to_stack_page(PartitionManualData *data, PartitionStackPage page);
void partitionmanual_create_disk_selection_page(PartitionManualData *data);
void partitionmanual_create_mount_points_page(PartitionManualData *data);

// Navigation button functions
void partitionmanual_create_navigation_buttons(PartitionManualData *data);

// Funciones de utilidad
void partitionmanual_refresh_disk_info(void);
void partitionmanual_open_gparted(const gchar *disk_path);
gchar* partitionmanual_get_disk_size(const gchar *disk_path);
void partitionmanual_on_page_shown(void);
void partitionmanual_test_update(void);

// Funciones callback para cambios de disco
void partitionmanual_on_disk_changed(const gchar *disk_path);
void partitionmanual_refresh_partitions(void);

// Funciones para manejo de particiones
void partitionmanual_populate_partitions(PartitionManualData *data, const gchar *disk_path);
void partitionmanual_clear_partitions(PartitionManualData *data);
PartitionInfo* partitionmanual_create_partition_info(const gchar *device_path, UDisksPartition *partition, UDisksBlock *block, UDisksObject *object);
void partitionmanual_free_partition_info(PartitionInfo *info);
void partitionmanual_add_partition_row(PartitionManualData *data, PartitionInfo *partition);
gboolean partitionmanual_is_partition_of_disk(const gchar *partition_path, const gchar *disk_path);
gchar* partitionmanual_format_partition_size(guint64 size_bytes);
const gchar* partitionmanual_get_filesystem_icon(const gchar *filesystem);

// Callbacks para señales de widgets
void on_partitionmanual_gparted_button_clicked(GtkButton *button, gpointer user_data);
void on_partitionmanual_refresh_clicked(GtkButton *button, gpointer user_data);
void on_partitionmanual_partition_configure_clicked(GtkButton *button, gpointer user_data);

// Callbacks para radiobuttons
void on_partitionmanual_auto_partition_toggled(GtkCheckButton *button, gpointer user_data);
void on_partitionmanual_auto_btrfs_toggled(GtkCheckButton *button, gpointer user_data);
void on_partitionmanual_manual_partition_toggled(GtkCheckButton *button, gpointer user_data);

// Callback para configuración de partición guardada
void on_partitionmanual_config_saved(PartitionConfig *config, gpointer user_data);

// Funciones adicionales para manejo de particiones
void partitionmanual_init_partition_manager(PartitionManualData *data);
void partitionmanual_cleanup_partition_manager(PartitionManualData *data);
void partitionmanual_update_partition_display(PartitionManualData *data);

// Navigation callbacks
void on_partitionmanual_next_button_clicked(GtkButton *button, gpointer user_data);
void on_partitionmanual_back_button_clicked(GtkButton *button, gpointer user_data);

// Funciones para obtener información del disco seleccionado
const char* partitionmanual_get_selected_disk(void);
DiskMode partitionmanual_get_partition_mode(void);

#endif /* PARTITIONMANUAL_H */