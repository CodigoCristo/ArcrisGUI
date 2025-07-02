#ifndef PAGE3_H
#define PAGE3_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include "disk_manager.h"

// Enumeración para modos de particionado
typedef enum {
    DISK_MODE_AUTO_PARTITION,
    DISK_MODE_AUTO_BTRFS,
    DISK_MODE_MANUAL_PARTITION
} DiskMode;

// Estructura para datos de la página 3
typedef struct _Page3Data {
    AdwCarousel *carousel;
    GtkRevealer *revealer;
    
    // Widget principal de la página
    GtkWidget *main_content;
    
    // Widgets específicos de la página 3 (selección de disco)
    AdwComboRow *disk_combo;
    GtkCheckButton *auto_partition_radio;
    GtkCheckButton *auto_btrfs_radio;
    GtkCheckButton *manual_partition_radio;
    GtkButton *refresh_button;
    
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

// Callbacks para señales de widgets
void on_page3_disk_selection_changed(AdwComboRow *combo, GParamSpec *param, gpointer user_data);
void on_page3_partition_mode_changed(GtkCheckButton *button, gpointer user_data);
void on_page3_refresh_clicked(GtkButton *button, gpointer user_data);

// Navigation callbacks
void on_page3_next_button_clicked(GtkButton *button, gpointer user_data);
void on_page3_back_button_clicked(GtkButton *button, gpointer user_data);

#endif /* PAGE3_H */