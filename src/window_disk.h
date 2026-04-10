#ifndef WINDOW_DISK_H
#define WINDOW_DISK_H

#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct _WindowDiskData {
    GtkWindow  *window;
    GtkBuilder *builder;

    GtkButton  *close_button;
    GtkButton  *save_button;

    /* Sistema de archivos */
    GtkCheckButton *ext4_radio;
    GtkCheckButton *btrfs_radio;
    GtkCheckButton *xfs_radio;

    /* Directorio personal */
    GtkCheckButton *home_no_radio;
    GtkCheckButton *home_subvolume_radio;
    GtkCheckButton *home_partition_radio;
    AdwExpanderRow *root_size_expander;
    GtkScale       *root_size_scale;
    GtkLabel       *root_size_label;
    GtkLabel       *root_size_suffix_label;
    GtkLabel       *home_size_label;

    /* Cifrado */
    GtkSwitch          *encryption_switch;
    AdwExpanderRow     *encryption_expander;
    AdwEntryRow        *encryption_password_entry;
    AdwEntryRow        *encryption_confirm_entry;
    AdwActionRow       *encryption_error_row;

    /* Swap */
    GtkCheckButton *swap_none_radio;
    GtkCheckButton *swap_half_radio;
    GtkCheckButton *swap_equal_radio;
    GtkCheckButton *swap_custom_radio;
    GtkCheckButton *swap_disabled_radio;
    GtkButton      *swap_decrease_button;
    GtkButton      *swap_increase_button;
    GtkLabel       *swap_size_label;

    guint    disk_total_gb;   /* tamaño del disco seleccionado en GB */
    gboolean is_initialized;

    /* Widgets para traducción */
    AdwWindowTitle       *title_widget;
    AdwPreferencesGroup  *filesystem_group;
    AdwPreferencesGroup  *home_group;
    AdwPreferencesGroup  *swap_group;
    AdwActionRow         *home_subvolume_row;
    AdwActionRow         *home_partition_row;
    AdwActionRow         *slider_row;
    AdwActionRow         *root_label_row;
    AdwActionRow         *home_label_row;
    AdwActionRow         *swap_none_row;
    AdwActionRow         *swap_half_row;
    AdwActionRow         *swap_equal_row;
    AdwActionRow         *swap_custom_row;
    AdwActionRow         *swap_disabled_row;
    AdwPreferencesGroup  *encryption_group;
    AdwActionRow         *encryption_toggle_row;
} WindowDiskData;

/* Ciclo de vida */
WindowDiskData *window_disk_new(void);
void            window_disk_init(WindowDiskData *data);
void            window_disk_update_language(WindowDiskData *data);
void            window_disk_show(WindowDiskData *data, GtkWindow *parent);

/* Variables.sh */
void     window_disk_init_variables(void);
void     window_disk_load_from_variables(WindowDiskData *data);
gboolean window_disk_save_to_variables(WindowDiskData *data);

/* Instancia global */
WindowDiskData *window_disk_get_instance(void);

/* Internos */
void window_disk_load_widgets(WindowDiskData *data);
void window_disk_connect_signals(WindowDiskData *data);

/* Callbacks */
void on_disk_filesystem_radio_toggled(GtkCheckButton *radio, gpointer user_data);
void on_disk_close_button_clicked(GtkButton *button, gpointer user_data);
void on_disk_save_button_clicked(GtkButton *button, gpointer user_data);
void on_disk_home_radio_toggled(GtkCheckButton *radio, gpointer user_data);
void on_disk_swap_radio_toggled(GtkCheckButton *radio, gpointer user_data);
void on_disk_root_scale_value_changed(GtkRange *range, gpointer user_data);
void on_disk_swap_decrease_clicked(GtkButton *button, gpointer user_data);
void on_disk_swap_increase_clicked(GtkButton *button, gpointer user_data);
gboolean on_disk_encryption_switch_toggled(GtkSwitch *sw, gboolean active, gpointer user_data);
gboolean on_disk_window_close_request(GtkWindow *window, gpointer user_data);
void on_disk_encryption_password_changed(AdwEntryRow *entry, gpointer user_data);
void on_disk_encryption_confirm_changed(AdwEntryRow *entry, gpointer user_data);

#endif /* WINDOW_DISK_H */
