#ifndef PAGE3_H
#define PAGE3_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include "types.h"
#include <udisks/udisks.h>
#include "disk_manager.h"
#include "partition_manager.h"



// Estructura para información de partición
typedef struct _Page3PartitionInfo {
    gchar *device_path;       // ej: /dev/sda1
    gchar *filesystem;        // ej: ext4, ntfs, etc.
    gchar *mount_point;       // ej: /, /home, etc.
    guint64 size;            // tamaño en bytes
    gchar *size_formatted;    // tamaño formateado: "100 GB"
    gchar *label;            // etiqueta del volumen
    gboolean is_mounted;      // si está montado
    gchar *uuid;             // UUID de la partición
} Page3PartitionInfo;

// Estructura para datos de la página 3
typedef struct _Page3Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // AdwNavigationView para navegación interna
    AdwNavigationView *navigation_view;
    
    // Widgets específicos de la página 3 (selección de disco)
    AdwComboRow *disk_combo;
    GtkCheckButton *auto_partition_radio;
    GtkCheckButton *auto_btrfs_radio;
    GtkCheckButton *cifrado_partition_button;
    GtkCheckButton *manual_partition_radio;
    GtkButton *refresh_button;
    GtkButton *configure_partitions_button;
    GtkButton *save_key_disk_button;
    
    // Widgets de la página de particiones manuales
    GtkLabel *disk_label_page4;
    GtkLabel *disk_size_label_page4;
    GtkButton *gparted_button;
    GtkButton *refresh_partitions_button;
    GtkButton *return_disks;
    AdwButtonRow *return_disks_encryption;
    AdwPreferencesGroup *partitions_group;
    
    // Widgets para particionado cifrado
    AdwPasswordEntryRow *password_entry;
    AdwPasswordEntryRow *password_confirm_entry;
    GtkLabel *password_error_label;
    
    // Estado del cifrado
    gboolean encryption_enabled;
    gboolean passwords_match;
    gboolean password_length_valid;
    
    // Cliente UDisks2 para obtener información de particiones
    UDisksClient *udisks_client;
    
    // Lista de particiones
    GList *partitions;        // Lista de Page3PartitionInfo*
    
    // Lista de filas de particiones (para poder eliminarlas correctamente)
    GList *partition_rows;    // Lista de AdwActionRow*
    
    // Manejador de particiones
    PartitionManager *partition_manager;
    
} Page3Data;

// Funciones principales de la página 3
void page3_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer);
void page3_cleanup(Page3Data *data);

// Funciones de configuración y carga de datos
void page3_setup_widgets(Page3Data *data);
void page3_load_data(Page3Data *data);

// Funciones de navegación y validación
gboolean page3_go_to_next_page(Page3Data *data);
gboolean page3_go_to_previous_page(Page3Data *data);
gboolean page3_is_configuration_valid(void);

// Navigation button functions
void page3_create_navigation_buttons(Page3Data *data);

// Funciones de acceso a datos (para uso externo)
const char* page3_get_selected_disk(void);
DiskMode page3_get_partition_mode(void);

// Funciones de actualización
void page3_refresh_disk_list(void);

// Funciones callback para cambios de disco
void page3_refresh_partitions(void);
void page3_on_disk_changed(const gchar *disk_path);

// Callbacks para señales de widgets
void on_page3_disk_selection_changed(AdwComboRow *combo, GParamSpec *param, gpointer user_data);
void on_page3_partition_mode_changed(GtkCheckButton *button, gpointer user_data);
void on_page3_refresh_clicked(GtkButton *button, gpointer user_data);
void on_page3_configure_partitions_clicked(GtkButton *button, gpointer user_data);
void on_page3_save_key_disk_clicked(GtkButton *button, gpointer user_data);

// Callbacks para la página de particiones manuales
void on_page3_gparted_button_clicked(GtkButton *button, gpointer user_data);
void on_page3_refresh_partitions_clicked(GtkButton *button, gpointer user_data);
void on_page3_return_disks_clicked(GtkButton *button, gpointer user_data);
void on_page3_return_disks_encryption_clicked(AdwButtonRow *button, gpointer user_data);

// Navigation callbacks
void on_page3_next_button_clicked(GtkButton *button, gpointer user_data);
void on_page3_back_button_clicked(GtkButton *button, gpointer user_data);

// Funciones de navegación interna
void page3_navigate_to_manual_partitions(Page3Data *data);
void page3_navigate_back_to_disk_selection(Page3Data *data);
void page3_update_manual_partitions_info(Page3Data *data);

// Funciones auxiliares para manejo de particiones
gchar* page3_get_disk_size(const gchar *disk_path);
gchar* page3_format_disk_size(guint64 size_bytes);
void page3_clear_partitions(Page3Data *data);
void page3_populate_partitions(Page3Data *data, const gchar *disk_path);
void page3_add_partition_row(Page3Data *data, Page3PartitionInfo *partition);

// Funciones para configuración de particiones
void on_page3_partition_configure_clicked(GtkButton *button, gpointer user_data);
void on_partition_config_saved(PartitionConfig *config, gpointer user_data);
void page3_init_partition_manager(Page3Data *data);
void page3_update_partition_row_subtitle(Page3Data *data, const gchar *device_path);
void page3_update_all_partition_subtitles(Page3Data *data);
void page3_clear_previous_disk_configs(Page3Data *data, const gchar *current_disk_path);
gchar* page3_get_current_selected_disk(Page3Data *data);
gboolean page3_config_belongs_to_current_disk(Page3Data *data, const gchar *device_path);
void page3_save_partition_mode(const gchar *partition_mode);
void page3_update_next_button_sensitivity(Page3Data *data, gboolean is_manual_mode);
void page3_load_partition_mode(Page3Data *data);
GtkWidget* page3_find_next_button_recursive(GtkWidget *widget);

// Funciones para manejo de cifrado
void page3_navigate_to_encryption_key(Page3Data *data);
void page3_navigate_back_from_encryption(Page3Data *data);
void page3_check_password_match(Page3Data *data);
void page3_validate_password_length(Page3Data *data);
void page3_update_encryption_button_state(Page3Data *data);
void page3_check_success_and_activate(Page3Data *data);
void page3_update_encryption_variables(Page3Data *data);
void page3_create_encryption_variables(void);
const gchar* page3_get_encryption_password(Page3Data *data);
void page3_save_encryption_config(Page3Data *data);

// Callbacks para campos de contraseña
void on_page3_password_changed(AdwPasswordEntryRow *entry, gpointer user_data);
void on_page3_password_confirm_changed(AdwPasswordEntryRow *entry, gpointer user_data);

// Funciones para manejo de particiones con UDisks2
Page3PartitionInfo* page3_create_partition_info(const gchar *device_path, UDisksPartition *partition, UDisksBlock *block, UDisksObject *object);
void page3_free_partition_info(Page3PartitionInfo *info);
gboolean page3_is_partition_of_disk(const gchar *partition_path, const gchar *disk_path);
gchar* page3_format_partition_size(guint64 size_bytes);
const gchar* page3_get_filesystem_icon(const gchar *filesystem);

// Funciones para detectar información del disco
gchar* page3_get_partition_table_type(const gchar *disk_path);
gchar* page3_get_firmware_type(void);

#endif /* PAGE3_H */