#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include <udisks/udisks.h>

// Estructura para información de partición configurada
typedef struct _PartitionConfig {
    gchar *device_path;       // ej: /dev/sda1
    gchar *filesystem;        // ej: ext4, btrfs, etc.
    gchar *mount_point;       // ej: /, /home, /boot, etc.
    gboolean is_swap;         // si es partición swap
    gboolean format_needed;   // si necesita formatear
    gchar *original_filesystem; // filesystem original (para comparar)
} PartitionConfig;

// Estructura principal del PartitionManager
typedef struct _PartitionManager {
    // Ventana del diálogo
    AdwWindow *partition_dialog;
    
    // Widgets del diálogo
    GtkSwitch *swap_switch;
    AdwComboRow *mount_point_combo;
    AdwComboRow *format_combo;
    GtkButton *cancel_button;
    GtkButton *save_button;
    AdwWindowTitle *window_title;
    
    // Modelos de datos
    GtkStringList *mount_point_list;
    GtkStringList *format_list;
    
    // Configuración actual
    PartitionConfig *current_config;
    
    // Lista de todas las particiones configuradas
    GList *partition_configs;  // Lista de PartitionConfig*
    
    // Callback para cuando se guarda la configuración
    void (*on_config_saved)(PartitionConfig *config, gpointer user_data);
    gpointer callback_data;
    
} PartitionManager;

// Funciones principales del PartitionManager
PartitionManager* partition_manager_new(void);
void partition_manager_free(PartitionManager *manager);

// Funciones de inicialización
gboolean partition_manager_init(PartitionManager *manager, GtkBuilder *builder);
gboolean partition_manager_setup_widgets(PartitionManager *manager);

// Funciones para mostrar el diálogo
void partition_manager_show_dialog(PartitionManager *manager, 
                                  const gchar *device_path, 
                                  const gchar *current_filesystem,
                                  const gchar *current_mount_point,
                                  GtkWindow *parent);

// Funciones para manejar configuraciones
PartitionConfig* partition_manager_create_config(const gchar *device_path,
                                                const gchar *filesystem,
                                                const gchar *mount_point,
                                                gboolean is_swap);
void partition_manager_free_config(PartitionConfig *config);
PartitionConfig* partition_manager_copy_config(PartitionConfig *config);

// Funciones para guardar/cargar configuraciones
gboolean partition_manager_save_to_variables(PartitionManager *manager);
gboolean partition_manager_load_from_variables(PartitionManager *manager);

// Funciones para manejar la lista de particiones
void partition_manager_add_config(PartitionManager *manager, PartitionConfig *config);
void partition_manager_remove_config(PartitionManager *manager, const gchar *device_path);
PartitionConfig* partition_manager_find_config(PartitionManager *manager, const gchar *device_path);
void partition_manager_clear_configs(PartitionManager *manager);

// Funciones auxiliares
const gchar* partition_manager_get_filesystem_name(const gchar *filesystem);
const gchar* partition_manager_get_mount_point_display(const gchar *mount_point);
gboolean partition_manager_is_valid_mount_point(const gchar *mount_point);
gboolean partition_manager_validate_config(PartitionConfig *config);

// Funciones de conversión de formatos
gchar* partition_manager_format_to_mkfs(const gchar *format);
gchar* partition_manager_mkfs_to_format(const gchar *mkfs_format);

// Funciones para conversión de datos
gchar* partition_manager_config_to_string(PartitionConfig *config);
PartitionConfig* partition_manager_config_from_string(const gchar *config_string);

// Callbacks para señales del diálogo
void on_partition_dialog_cancel_clicked(GtkButton *button, gpointer user_data);
void on_partition_dialog_save_clicked(GtkButton *button, gpointer user_data);
gboolean on_partition_dialog_close_request(AdwWindow *window, gpointer user_data);

// Callbacks para cambios en los widgets
gboolean on_swap_switch_toggled(GtkSwitch *switch_widget, gboolean state, gpointer user_data);
void on_mount_point_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);
void on_format_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data);

// Funciones para validación y verificación
gboolean partition_manager_has_root_partition(PartitionManager *manager);
gboolean partition_manager_has_boot_partition(PartitionManager *manager);
gboolean partition_manager_has_swap_partition(PartitionManager *manager);
gboolean partition_manager_validate_all_configs(PartitionManager *manager);

// Funciones para obtener información
GList* partition_manager_get_configs(PartitionManager *manager);
guint partition_manager_get_config_count(PartitionManager *manager);
gchar* partition_manager_get_summary_text(PartitionManager *manager);

// Funciones para callback personalizado
void partition_manager_set_save_callback(PartitionManager *manager, 
                                        void (*callback)(PartitionConfig *config, gpointer user_data),
                                        gpointer user_data);

#endif /* PARTITION_MANAGER_H */