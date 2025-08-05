#include "partitionmanual.h"
#include "page3.h"
#include "disk_manager.h"
#include "config.h"
#include "partition_manager.h"
#include <stdio.h>

#include <string.h>
#include <udisks/udisks.h>

// Variable global para datos del particionado manual
static PartitionManualData *g_partitionmanual_data = NULL;
static DiskManager *g_disk_manager = NULL;

// Funciones privadas
static void partitionmanual_connect_signals(PartitionManualData *data);
static gchar* partitionmanual_format_disk_size(guint64 size_bytes);
static gboolean partitionmanual_validate_selection(PartitionManualData *data);
static void partitionmanual_setup_disk_manager(PartitionManualData *data, GtkBuilder *page_builder);

// Función principal de inicialización del particionado manual
void partitionmanual_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos
    g_partitionmanual_data = g_malloc0(sizeof(PartitionManualData));

    // Guardar referencias importantes
    g_partitionmanual_data->carousel = carousel;
    g_partitionmanual_data->revealer = revealer;

    // Cargar la página desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/partitionmanual.ui");
    GtkWidget *partitionmanual = GTK_WIDGET(gtk_builder_get_object(page_builder, "partitionmanual"));

    if (!partitionmanual) {
        LOG_ERROR("No se pudo cargar partitionmanual desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }

    // Obtener widgets específicos
    g_partitionmanual_data->main_content = partitionmanual;
    g_partitionmanual_data->partition_stack = GTK_STACK(gtk_builder_get_object(page_builder, "partition_stack"));

    // Widgets de la página de selección de disco
    g_partitionmanual_data->disk_selection_page = GTK_WIDGET(gtk_builder_get_object(page_builder, "disk_selection_page"));
    g_partitionmanual_data->disk_label = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_label"));
    g_partitionmanual_data->disk_size_label = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_size_label"));
    g_partitionmanual_data->disk_label_mount = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_label_mount"));
    g_partitionmanual_data->disk_size_label_mount = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_size_label_mount"));
    g_partitionmanual_data->refresh_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "refresh_button"));
    g_partitionmanual_data->disk_combo = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "disk_combo"));

    // Radiobuttons
    g_partitionmanual_data->auto_partition_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "auto_partition_radio"));
    g_partitionmanual_data->auto_btrfs_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "auto_btrfs_radio"));
    g_partitionmanual_data->manual_partition_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "manual_partition_radio"));

    // Widgets de la página de puntos de montaje
    g_partitionmanual_data->mount_points_page = GTK_WIDGET(gtk_builder_get_object(page_builder, "mount_points_page"));
    g_partitionmanual_data->gparted_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "gparted_button"));
    g_partitionmanual_data->partitions_group = ADW_PREFERENCES_GROUP(gtk_builder_get_object(page_builder, "partitions_group"));

    // Verificar que todos los widgets se obtuvieron correctamente
    if (!g_partitionmanual_data->partition_stack || !g_partitionmanual_data->disk_combo ||
        !g_partitionmanual_data->auto_partition_radio || !g_partitionmanual_data->auto_btrfs_radio ||
        !g_partitionmanual_data->manual_partition_radio || !g_partitionmanual_data->gparted_button) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de partitionmanual");
        g_object_unref(page_builder);
        return;
    }

    // Configurar el administrador de discos
    partitionmanual_setup_disk_manager(g_partitionmanual_data, page_builder);

    // Inicializar cliente UDisks2 para obtener información de particiones
    GError *error = NULL;
    g_partitionmanual_data->udisks_client = udisks_client_new_sync(NULL, &error);
    if (!g_partitionmanual_data->udisks_client) {
        LOG_WARNING("No se pudo inicializar cliente UDisks2: %s", error ? error->message : "Error desconocido");
        if (error) g_error_free(error);
    } else {
        LOG_INFO("Cliente UDisks2 inicializado para partitionmanual");
    }

    // Inicializar listas
    g_partitionmanual_data->partitions = NULL;
    g_partitionmanual_data->partition_rows = NULL;

    // Inicializar manejador de particiones
    partitionmanual_init_partition_manager(g_partitionmanual_data);

    // Configurar el stack
    partitionmanual_setup_stack(g_partitionmanual_data);

    // Realizar configuraciones iniciales
    partitionmanual_setup_widgets(g_partitionmanual_data);
    partitionmanual_load_data(g_partitionmanual_data);

    // Conectar señales
    partitionmanual_connect_signals(g_partitionmanual_data);

    // Crear botones de navegación
    partitionmanual_create_navigation_buttons(g_partitionmanual_data);

    // Añadir la página al carousel
    adw_carousel_append(carousel, partitionmanual);

    // Liberar el builder de la página
    g_object_unref(page_builder);

    LOG_INFO("Particionado Manual inicializado correctamente");
}

// Configurar el administrador de discos
static void partitionmanual_setup_disk_manager(PartitionManualData *data, GtkBuilder *page_builder)
{
    if (!data) return;

    // Crear el administrador de discos
    g_disk_manager = disk_manager_new();
    if (!g_disk_manager) {
        LOG_ERROR("No se pudo crear el DiskManager");
        return;
    }

    // Inicializar widgets del administrador de discos
    if (!disk_manager_init(g_disk_manager, page_builder)) {
        LOG_ERROR("No se pudieron inicializar los widgets del DiskManager");
        disk_manager_free(g_disk_manager);
        g_disk_manager = NULL;
        return;
    }

    LOG_INFO("DiskManager configurado correctamente para partitionmanual");
}

// Configurar el GtkStack
void partitionmanual_setup_stack(PartitionManualData *data)
{
    if (!data || !data->partition_stack) return;

    // Configurar el stack para mostrar la primera página por defecto
    data->current_stack_page = PARTITION_STACK_DISK_SELECTION;
    gtk_stack_set_visible_child_name(data->partition_stack, "disk_selection");

    LOG_INFO("GtkStack configurado para partitionmanual");
}

// Cambiar a una página específica del stack
void partitionmanual_switch_to_stack_page(PartitionManualData *data, PartitionStackPage page)
{
    if (!data || !data->partition_stack) return;

    data->current_stack_page = page;

    switch (page) {
        case PARTITION_STACK_DISK_SELECTION:
            gtk_stack_set_visible_child_name(data->partition_stack, "disk_selection");
            LOG_INFO("Cambiando a página de selección de disco");
            break;
        case PARTITION_STACK_MOUNT_POINTS:
            gtk_stack_set_visible_child_name(data->partition_stack, "mount_points");
            // Actualizar información del disco en la segunda página
            partitionmanual_update_disk_info(data);
            LOG_INFO("Cambiando a página de puntos de montaje");
            break;
        default:
            LOG_WARNING("Página de stack desconocida: %d", page);
            break;
    }
}

// Función para configurar widgets
void partitionmanual_setup_widgets(PartitionManualData *data)
{
    if (!data) return;

    // Configurar estado inicial de los radio buttons
    gtk_check_button_set_active(data->auto_partition_radio, TRUE);

    LOG_INFO("Widgets de partitionmanual configurados");
}

// Función para cargar datos
void partitionmanual_load_data(PartitionManualData *data)
{
    if (!data || !g_disk_manager) return;

    // Cargar configuración guardada desde variables.sh
    disk_manager_load_from_variables(g_disk_manager);

    // Actualizar UI según la configuración cargada
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (selected_disk) {
        LOG_INFO("Disco previamente seleccionado: %s", selected_disk);
        partitionmanual_update_disk_info(data);
    }

    LOG_INFO("Datos de partitionmanual cargados");
}

// Actualizar información del disco
void partitionmanual_update_disk_info(PartitionManualData *data)
{
    if (!data || !g_disk_manager) return;

    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (!selected_disk) {
        gtk_label_set_text(data->disk_label, "No hay disco seleccionado");
        gtk_label_set_text(data->disk_size_label, "");
        return;
    }

    // Obtener tamaño del disco
    gchar *disk_size = partitionmanual_get_disk_size(selected_disk);
    if (disk_size) {
        // Actualizar título: "Disco - XXX GB"
        gchar *disk_title = g_strdup_printf("Disco - %s", disk_size);
        gtk_label_set_text(data->disk_label, disk_title);

        // También actualizar los labels de mount points si existen
        if (data->disk_label_mount) {
            gtk_label_set_text(data->disk_label_mount, disk_title);
        }

        g_free(disk_title);
        g_free(disk_size);

        // Actualizar subtítulo: ruta del disco
        gtk_label_set_text(data->disk_size_label, selected_disk);

        // También actualizar el label de mount points si existe
        if (data->disk_size_label_mount) {
            gtk_label_set_text(data->disk_size_label_mount, selected_disk);
        }
    } else {
        gtk_label_set_text(data->disk_label, "Disco - Tamaño desconocido");
        gtk_label_set_text(data->disk_size_label, selected_disk);

        // También actualizar los labels de mount points si existen
        if (data->disk_label_mount) {
            gtk_label_set_text(data->disk_label_mount, "Disco - Tamaño desconocido");
        }
        if (data->disk_size_label_mount) {
            gtk_label_set_text(data->disk_size_label_mount, selected_disk);
        }
    }

    // Si estamos en la página de puntos de montaje, actualizar particiones
    if (data->current_stack_page == PARTITION_STACK_MOUNT_POINTS) {
        partitionmanual_populate_partitions(data, selected_disk);
    }

    LOG_INFO("Información del disco actualizada: %s", selected_disk);
}

// Conectar señales
static void partitionmanual_connect_signals(PartitionManualData *data)
{
    if (!data) return;

    // Conectar señales de los radiobuttons
    g_signal_connect(data->auto_partition_radio, "toggled",
                     G_CALLBACK(on_partitionmanual_auto_partition_toggled), data);
    g_signal_connect(data->auto_btrfs_radio, "toggled",
                     G_CALLBACK(on_partitionmanual_auto_btrfs_toggled), data);
    g_signal_connect(data->manual_partition_radio, "toggled",
                     G_CALLBACK(on_partitionmanual_manual_partition_toggled), data);

    // Conectar señales de botones
    g_signal_connect(data->refresh_button, "clicked",
                     G_CALLBACK(on_partitionmanual_refresh_clicked), data);
    g_signal_connect(data->gparted_button, "clicked",
                     G_CALLBACK(on_partitionmanual_gparted_button_clicked), data);

    LOG_INFO("Señales conectadas para partitionmanual");
}

// Validar selección antes de continuar
static gboolean partitionmanual_validate_selection(PartitionManualData *data)
{
    if (!data || !g_disk_manager) return FALSE;

    // Verificar que se haya seleccionado un disco
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (!selected_disk) {
        LOG_WARNING("No se ha seleccionado ningún disco");

        // Mostrar mensaje de error al usuario
        AdwDialog *dialog = adw_alert_dialog_new(
            "Disco no seleccionado",
            "Por favor selecciona un disco para continuar"
        );

        adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "ok", "Aceptar");
        adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "ok");
        adw_dialog_present(dialog, NULL);

        return FALSE;
    }

    return TRUE;
}

// Guardar configuración actual
static void partitionmanual_save_configuration(PartitionManualData *data)
{
    if (!data || !g_disk_manager) return;

    // Guardar configuración en variables.sh
    if (disk_manager_save_to_variables(g_disk_manager)) {
        const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
        if (selected_disk) {
            LOG_INFO("Configuración de disco guardada: %s", selected_disk);
        }
    } else {
        LOG_WARNING("No se pudo guardar la configuración de disco");
    }
}

// Función para avanzar a la siguiente página
gboolean partitionmanual_go_to_next_page(PartitionManualData *data)
{
    if (!data || !g_disk_manager) return FALSE;

    // Validar selección
    if (!partitionmanual_validate_selection(data)) {
        return FALSE;
    }

    // Guardar configuración
    partitionmanual_save_configuration(data);

    // Si estamos en el stack de puntos de montaje, ir a page4
    if (data->current_stack_page == PARTITION_STACK_MOUNT_POINTS) {
        LOG_INFO("Navegando desde puntos de montaje a page4");
        if (data->carousel) {
            GtkWidget *page4 = adw_carousel_get_nth_page(data->carousel, 3); // índice 3 para page4
            if (page4) {
                adw_carousel_scroll_to(data->carousel, page4, TRUE);
            } else {
                LOG_ERROR("No se pudo encontrar page4 en el carousel");
            }
        }
        return TRUE;
    }

    // Verificar el modo de particionado seleccionado
    DiskMode mode = partitionmanual_get_partition_mode();

    switch (mode) {
        case DISK_MODE_AUTO_PARTITION:
        case DISK_MODE_AUTO_BTRFS:
            // Ir a page4 (particionado automático)
            LOG_INFO("Modo automático seleccionado, dirigiendo a page4");
            if (data->carousel) {
                GtkWidget *page4 = adw_carousel_get_nth_page(data->carousel, 3); // índice 3 para page4
                if (page4) {
                    adw_carousel_scroll_to(data->carousel, page4, TRUE);
                } else {
                    LOG_ERROR("No se pudo encontrar page4 en el carousel");
                }
            }
            break;

        case DISK_MODE_MANUAL_PARTITION:
            // Cambiar al stack de puntos de montaje
            LOG_INFO("Modo manual seleccionado, cambiando a configuración de puntos de montaje");
            partitionmanual_switch_to_stack_page(data, PARTITION_STACK_MOUNT_POINTS);
            break;

        default:
            LOG_WARNING("Modo de particionado desconocido");
            return FALSE;
    }

    return TRUE;
}

// Función para ir a la página anterior
gboolean partitionmanual_go_to_previous_page(PartitionManualData *data)
{
    if (!data) return FALSE;

    // Si estamos en la página de puntos de montaje, regresar a selección de disco
    if (data->current_stack_page == PARTITION_STACK_MOUNT_POINTS) {
        partitionmanual_switch_to_stack_page(data, PARTITION_STACK_DISK_SELECTION);
        return TRUE;
    }

    // Si estamos en la página de selección, regresar a page3
    LOG_INFO("Regresando a página anterior desde partitionmanual");

    if (data->carousel) {
        GtkWidget *page3 = adw_carousel_get_nth_page(data->carousel, 2); // índice 2 para page3
        if (page3) {
            adw_carousel_scroll_to(data->carousel, page3, TRUE);
        } else {
            LOG_ERROR("No se pudo encontrar page3 en el carousel");
        }
    }

    return TRUE;
}

// Función para obtener el disco seleccionado
const char* partitionmanual_get_selected_disk(void)
{
    if (!g_disk_manager) return NULL;
    return disk_manager_get_selected_disk(g_disk_manager);
}

// Función para obtener el modo de particionado
DiskMode partitionmanual_get_partition_mode(void)
{
    if (!g_partitionmanual_data) return DISK_MODE_AUTO_PARTITION;

    if (g_partitionmanual_data->auto_btrfs_radio &&
        gtk_check_button_get_active(g_partitionmanual_data->auto_btrfs_radio)) {
        return DISK_MODE_AUTO_BTRFS;
    } else if (g_partitionmanual_data->manual_partition_radio &&
               gtk_check_button_get_active(g_partitionmanual_data->manual_partition_radio)) {
        return DISK_MODE_MANUAL_PARTITION;
    }

    return DISK_MODE_AUTO_PARTITION;
}

// Callbacks para radiobuttons
void on_partitionmanual_auto_partition_toggled(GtkCheckButton *button, gpointer user_data)
{
    PartitionManualData *data = (PartitionManualData *)user_data;
    if (!data || !gtk_check_button_get_active(button)) return;

    LOG_INFO("Particionado automático seleccionado");
}

void on_partitionmanual_auto_btrfs_toggled(GtkCheckButton *button, gpointer user_data)
{
    PartitionManualData *data = (PartitionManualData *)user_data;
    if (!data || !gtk_check_button_get_active(button)) return;

    LOG_INFO("Particionado automático Btrfs seleccionado");
}

void on_partitionmanual_manual_partition_toggled(GtkCheckButton *button, gpointer user_data)
{
    PartitionManualData *data = (PartitionManualData *)user_data;
    if (!data || !gtk_check_button_get_active(button)) return;

    LOG_INFO("Particionado manual seleccionado");
}

// Callback para botón de refresh
void on_partitionmanual_refresh_clicked(GtkButton *button, gpointer user_data)
{
    LOG_INFO("Actualizando lista de discos desde partitionmanual");
    if (g_disk_manager) {
        disk_manager_refresh(g_disk_manager);
    }
}

// Callback para botón de Gparted
void on_partitionmanual_gparted_button_clicked(GtkButton *button, gpointer user_data)
{
    PartitionManualData *data = (PartitionManualData *)user_data;
    if (!data || !g_disk_manager) return;

    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (selected_disk) {
        partitionmanual_open_gparted(selected_disk);
    } else {
        LOG_WARNING("No hay disco seleccionado para abrir Gparted");
    }
}

// Funciones de navegación
void partitionmanual_create_navigation_buttons(PartitionManualData *data)
{
    if (!data || !data->main_content) return;

    // Crear el contenedor de navegación
    GtkWidget *navigation_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(navigation_box, 24);
    gtk_widget_set_halign(navigation_box, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(navigation_box, TRUE);

    // Crear botón atrás
    GtkButton *back_button = GTK_BUTTON(gtk_button_new_with_label("Atrás"));
    gtk_widget_set_halign(GTK_WIDGET(back_button), GTK_ALIGN_START);
    gtk_widget_add_css_class(GTK_WIDGET(back_button), "pill");

    // Crear espaciador
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);

    // Crear botón siguiente
    GtkButton *next_button = GTK_BUTTON(gtk_button_new_with_label("Siguiente"));
    gtk_widget_set_halign(GTK_WIDGET(next_button), GTK_ALIGN_END);
    gtk_widget_add_css_class(GTK_WIDGET(next_button), "pill");
    gtk_widget_add_css_class(GTK_WIDGET(next_button), "suggested-action");

    // Agregar widgets al contenedor
    gtk_box_append(GTK_BOX(navigation_box), GTK_WIDGET(back_button));
    gtk_box_append(GTK_BOX(navigation_box), spacer);
    gtk_box_append(GTK_BOX(navigation_box), GTK_WIDGET(next_button));

    // Conectar señales
    g_signal_connect(back_button, "clicked",
                     G_CALLBACK(on_partitionmanual_back_button_clicked), data);
    g_signal_connect(next_button, "clicked",
                     G_CALLBACK(on_partitionmanual_next_button_clicked), data);

    // Buscar el contenedor principal y agregar navegación
    GtkWidget *status_page = gtk_widget_get_first_child(data->main_content);
    if (status_page) {
        GtkWidget *status_child = gtk_widget_get_first_child(status_page);
        if (status_child && GTK_IS_BOX(status_child)) {
            gtk_box_append(GTK_BOX(status_child), navigation_box);
        }
    }

    LOG_INFO("Botones de navegación creados para partitionmanual");
}

// Callbacks de navegación
void on_partitionmanual_next_button_clicked(GtkButton *button, gpointer user_data)
{
    PartitionManualData *data = (PartitionManualData *)user_data;

    if (partitionmanual_go_to_next_page(data)) {
        LOG_INFO("Navegación exitosa desde partitionmanual");
    } else {
        LOG_WARNING("No se pudo navegar desde partitionmanual");
    }
}

void on_partitionmanual_back_button_clicked(GtkButton *button, gpointer user_data)
{
    PartitionManualData *data = (PartitionManualData *)user_data;

    if (partitionmanual_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde partitionmanual");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde partitionmanual");
    }
}

// Funciones de utilidad del disco
gchar* partitionmanual_get_disk_size(const gchar *disk_path)
{
    if (!disk_path || !g_partitionmanual_data || !g_partitionmanual_data->udisks_client) {
        return g_strdup("Tamaño desconocido");
    }

    // Usar UDisks2 para obtener información precisa del disco
    GList *objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(g_partitionmanual_data->udisks_client));
    GList *l;

    for (l = objects; l != NULL; l = l->next) {
        UDisksObject *object = UDISKS_OBJECT(l->data);
        UDisksBlock *block = udisks_object_peek_block(object);

        if (block != NULL) {
            const gchar *device_path = udisks_block_get_device(block);

            if (device_path && g_strcmp0(device_path, disk_path) == 0) {
                // Encontramos el disco, obtener su tamaño
                guint64 size_bytes = udisks_block_get_size(block);
                g_list_free_full(objects, g_object_unref);
                return partitionmanual_format_disk_size(size_bytes);
            }
        }
    }

    g_list_free_full(objects, g_object_unref);
    return g_strdup("Tamaño desconocido");
}

static gchar* partitionmanual_format_disk_size(guint64 size_bytes)
{
    // Usar estándar binario: size / 1073741824.0 para GiB
    if (size_bytes >= 1099511627776ULL) {
        return g_strdup_printf("%.2f TiB", size_bytes / 1099511627776.0);
    } else if (size_bytes >= 1073741824ULL) {
        return g_strdup_printf("%.2f GiB", size_bytes / 1073741824.0);
    } else if (size_bytes >= 1048576ULL) {
        return g_strdup_printf("%.2f MiB", size_bytes / 1048576.0);
    } else if (size_bytes >= 1024ULL) {
        return g_strdup_printf("%.2f KiB", size_bytes / 1024.0);
    } else {
        return g_strdup_printf("%lu bytes", size_bytes);
    }
}

// Abrir Gparted
void partitionmanual_open_gparted(const gchar *disk_path)
{
    if (!disk_path) return;

    gchar *command = g_strdup_printf("sudo pkexec gparted %s", disk_path);

    GError *error = NULL;
    gboolean success = g_spawn_command_line_async(command, &error);

    if (success) {
        LOG_INFO("Gparted abierto para disco: %s", disk_path);
    } else {
        LOG_ERROR("Error al abrir Gparted: %s", error ? error->message : "Error desconocido");
        if (error) g_error_free(error);
    }

    g_free(command);
}

// Función de comparación para ordenar particiones por número
static gint partitionmanual_partition_compare_func(gconstpointer a, gconstpointer b)
{
    const PartitionInfo *info_a = (const PartitionInfo*)a;
    const PartitionInfo *info_b = (const PartitionInfo*)b;

    if (!info_a || !info_b || !info_a->device_path || !info_b->device_path) {
        return 0;
    }

    // Extraer el nombre base del dispositivo (ej: sda1 -> sda1, nvme0n1p1 -> nvme0n1p1)
    gchar *basename_a = g_path_get_basename(info_a->device_path);
    gchar *basename_b = g_path_get_basename(info_b->device_path);

    gchar *num_start_a = NULL;
    gchar *num_start_b = NULL;

    // Para dispositivos nvme (formato: nvme0n1p1, nvme0n1p2, etc.)
    if (g_str_has_prefix(basename_a, "nvme") && strstr(basename_a, "p")) {
        num_start_a = strrchr(basename_a, 'p');
        if (num_start_a) num_start_a++; // Saltar la 'p'
    } else {
        // Para dispositivos tradicionales (formato: sda1, sdb2, etc.)
        // Encontrar donde comienzan los números
        num_start_a = basename_a;
        while (*num_start_a && !g_ascii_isdigit(*num_start_a)) {
            num_start_a++;
        }
    }

    // Mismo proceso para el segundo dispositivo
    if (g_str_has_prefix(basename_b, "nvme") && strstr(basename_b, "p")) {
        num_start_b = strrchr(basename_b, 'p');
        if (num_start_b) num_start_b++; // Saltar la 'p'
    } else {
        // Para dispositivos tradicionales
        num_start_b = basename_b;
        while (*num_start_b && !g_ascii_isdigit(*num_start_b)) {
            num_start_b++;
        }
    }

    // Si ambos tienen números, compararlos numéricamente
    gint result = 0;
    if (num_start_a && num_start_b && g_ascii_isdigit(*num_start_a) && g_ascii_isdigit(*num_start_b)) {
        gint num_a = atoi(num_start_a);
        gint num_b = atoi(num_start_b);
        result = num_a - num_b;
    } else {
        // Si no tienen números, comparar alfabéticamente
        result = g_strcmp0(basename_a, basename_b);
    }

    g_free(basename_a);
    g_free(basename_b);

    return result;
}

// Funciones de particiones (implementación básica)
void partitionmanual_populate_partitions(PartitionManualData *data, const gchar *disk_path)
{
    if (!data || !disk_path || !data->udisks_client) {
        LOG_WARNING("partitionmanual_populate_partitions: parámetros inválidos");
        return;
    }

    LOG_INFO("Obteniendo particiones del disco: %s", disk_path);

    // Limpiar particiones anteriores
    partitionmanual_clear_partitions(data);

    // Obtener todos los objetos de UDisks2
    GList *objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(data->udisks_client));

    int partition_count = 0;
    GList *l;
    for (l = objects; l != NULL; l = l->next) {
        UDisksObject *object = UDISKS_OBJECT(l->data);
        UDisksBlock *block = udisks_object_peek_block(object);
        UDisksPartition *partition = udisks_object_peek_partition(object);

        if (block != NULL && partition != NULL) {
            const gchar *device_path = udisks_block_get_device(block);

            // Verificar si esta partición pertenece al disco seleccionado
            if (device_path && partitionmanual_is_partition_of_disk(device_path, disk_path)) {
                // Crear información de la partición
                PartitionInfo *partition_info = partitionmanual_create_partition_info(device_path, partition, block, object);

                if (partition_info) {
                    // Agregar a la lista de particiones (sin agregar a la interfaz todavía)
                    data->partitions = g_list_append(data->partitions, partition_info);

                    partition_count++;
                    LOG_INFO("Partición encontrada: %s (%s, %s)",
                            partition_info->device_path,
                            partition_info->filesystem,
                            partition_info->size_formatted);
                }
            }
        }
    }

    g_list_free_full(objects, g_object_unref);

    // Ordenar la lista de particiones por número
    if (data->partitions) {
        data->partitions = g_list_sort(data->partitions, partitionmanual_partition_compare_func);
        LOG_INFO("Particiones ordenadas por número");

        // Ahora agregar las particiones ordenadas a la interfaz
        GList *current = data->partitions;
        while (current) {
            PartitionInfo *info = (PartitionInfo*)current->data;
            partitionmanual_add_partition_row(data, info);
            current = current->next;
        }
    }

    if (partition_count == 0) {
        // Si no hay particiones, mostrar mensaje
        AdwActionRow *empty_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(empty_row), "No hay particiones");
        adw_action_row_set_subtitle(empty_row, "El disco no tiene particiones o no se pudieron detectar");

        GtkImage *icon = GTK_IMAGE(gtk_image_new_from_icon_name("dialog-information-symbolic"));
        adw_action_row_add_prefix(empty_row, GTK_WIDGET(icon));

        adw_preferences_group_add(data->partitions_group, GTK_WIDGET(empty_row));

        // Guardar referencia a la fila vacía también
        data->partition_rows = g_list_append(data->partition_rows, empty_row);
    }

    LOG_INFO("Poblado completo: %d particiones encontradas para %s", partition_count, disk_path);

    // Actualizar visualización de particiones configuradas
    partitionmanual_update_partition_display(data);
}

void partitionmanual_clear_partitions(PartitionManualData *data)
{
    if (!data) return;

    // Limpiar lista de particiones
    g_list_free_full(data->partitions, (GDestroyNotify)partitionmanual_free_partition_info);
    data->partitions = NULL;

    // Limpiar filas de particiones de la interfaz gráfica
    if (data->partition_rows && data->partitions_group) {
        GList *l;
        for (l = data->partition_rows; l != NULL; l = l->next) {
            AdwActionRow *row = ADW_ACTION_ROW(l->data);
            if (row) {
                // Remover la fila del grupo de particiones
                adw_preferences_group_remove(data->partitions_group, GTK_WIDGET(row));
            }
        }
    }

    // Liberar lista de filas de particiones
    g_list_free(data->partition_rows);
    data->partition_rows = NULL;

    LOG_INFO("Particiones y filas de interfaz limpiadas");
}

void partitionmanual_free_partition_info(PartitionInfo *info)
{
    if (!info) return;

    g_free(info->device_path);
    g_free(info->filesystem);
    g_free(info->mount_point);
    g_free(info->size_formatted);
    g_free(info->label);
    g_free(info->uuid);
    g_free(info);
}

// Funciones de manejador de particiones
void partitionmanual_init_partition_manager(PartitionManualData *data)
{
    if (!data) return;

    data->partition_manager = partition_manager_new();
    if (data->partition_manager) {
        LOG_INFO("Partition manager inicializado para partitionmanual");
    } else {
        LOG_WARNING("No se pudo inicializar partition manager");
    }
}

void partitionmanual_cleanup_partition_manager(PartitionManualData *data)
{
    if (!data || !data->partition_manager) return;

    partition_manager_free(data->partition_manager);
    data->partition_manager = NULL;

    LOG_INFO("Partition manager limpiado");
}

// Funciones de limpieza
void partitionmanual_cleanup(PartitionManualData *data)
{
    if (g_partitionmanual_data) {
        // Limpiar lista de particiones
        partitionmanual_clear_partitions(g_partitionmanual_data);

        // Limpiar partition manager
        partitionmanual_cleanup_partition_manager(g_partitionmanual_data);

        // Limpiar cliente UDisks2
        if (g_partitionmanual_data->udisks_client) {
            g_object_unref(g_partitionmanual_data->udisks_client);
        }

        g_free(g_partitionmanual_data);
        g_partitionmanual_data = NULL;

        LOG_INFO("Particionado manual limpiado correctamente");
    }

    if (g_disk_manager) {
        disk_manager_free(g_disk_manager);
        g_disk_manager = NULL;
    }
}

// Funciones de estado
gboolean partitionmanual_is_configuration_valid(void)
{
    if (!g_partitionmanual_data) return FALSE;
    return partitionmanual_validate_selection(g_partitionmanual_data);
}

void partitionmanual_on_page_shown(void)
{
    if (!g_partitionmanual_data) return;

    LOG_INFO("Particionado manual mostrado, actualizando información del disco");
    partitionmanual_update_disk_info(g_partitionmanual_data);
}

void partitionmanual_test_update(void)
{
    if (!g_partitionmanual_data) {
        LOG_ERROR("partitionmanual_test_update: g_partitionmanual_data es NULL");
        return;
    }

    LOG_INFO("=== INICIANDO TEST DE ACTUALIZACIÓN PARTITIONMANUAL ===");

    // Forzar actualización de información del disco
    partitionmanual_update_disk_info(g_partitionmanual_data);

    // Verificar disco seleccionado actual
    const char *selected_disk = partitionmanual_get_selected_disk();
    LOG_INFO("Disco seleccionado: '%s'", selected_disk ? selected_disk : "NULL");

    LOG_INFO("=== FIN TEST DE ACTUALIZACIÓN PARTITIONMANUAL ===");
}

void partitionmanual_refresh_disk_info(void)
{
    if (!g_partitionmanual_data) return;
    partitionmanual_update_disk_info(g_partitionmanual_data);
}

// Función callback para cuando cambia el disco seleccionado
void partitionmanual_on_disk_changed(const gchar *disk_path)
{
    if (!g_partitionmanual_data) return;

    LOG_INFO("Disco cambiado en partitionmanual: %s", disk_path ? disk_path : "NULL");

    // Actualizar información del disco
    partitionmanual_update_disk_info(g_partitionmanual_data);
}

// Función pública para refrescar particiones (llamada desde disk_manager)
void partitionmanual_refresh_partitions(void)
{
    if (!g_partitionmanual_data) return;

    const char *selected_disk = partitionmanual_get_selected_disk();
    if (selected_disk && g_partitionmanual_data->current_stack_page == PARTITION_STACK_MOUNT_POINTS) {
        LOG_INFO("Refrescando particiones para disco: %s", selected_disk);
        partitionmanual_populate_partitions(g_partitionmanual_data, selected_disk);
    }
}

// Funciones adicionales para compatibilidad
void partitionmanual_add_partition_row(PartitionManualData *data, PartitionInfo *partition)
{
    if (!data || !partition) return;

    // Crear AdwActionRow
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());

    // Configurar título y subtítulo
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), partition->device_path);

    gchar *subtitle = g_strdup_printf("%s • %s%s%s",
                                      partition->size_formatted,
                                      partition->filesystem,
                                      partition->mount_point ? " • " : "",
                                      partition->mount_point ? partition->mount_point : "");
    adw_action_row_set_subtitle(row, subtitle);
    g_free(subtitle);

    // Añadir icono según el tipo de filesystem
    GtkImage *icon = GTK_IMAGE(gtk_image_new_from_icon_name(partitionmanual_get_filesystem_icon(partition->filesystem)));
    adw_action_row_add_prefix(row, GTK_WIDGET(icon));

    // Añadir botón de configuración
    GtkButton *config_button = GTK_BUTTON(gtk_button_new_from_icon_name("list-add-symbolic"));
    gtk_widget_set_valign(GTK_WIDGET(config_button), GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(GTK_WIDGET(config_button), "Configurar partición");

    // Añadir clases CSS
    gtk_widget_add_css_class(GTK_WIDGET(config_button), "flat");
    gtk_widget_add_css_class(GTK_WIDGET(config_button), "circular");

    // Conectar señal
    g_signal_connect(config_button, "clicked", G_CALLBACK(on_partitionmanual_partition_configure_clicked), partition);

    adw_action_row_add_suffix(row, GTK_WIDGET(config_button));

    // Añadir al grupo
    adw_preferences_group_add(data->partitions_group, GTK_WIDGET(row));

    // Guardar referencia a la fila para poder eliminarla después
    data->partition_rows = g_list_append(data->partition_rows, row);

    LOG_INFO("Fila de partición añadida: %s", partition->device_path);
}

gboolean partitionmanual_is_partition_of_disk(const gchar *partition_path, const gchar *disk_path)
{
    if (!partition_path || !disk_path) return FALSE;

    // Extraer el nombre base del disco (ej: /dev/sda -> sda)
    gchar *disk_name = g_path_get_basename(disk_path);

    // Verificar si la partición comienza con el nombre del disco
    gchar *partition_name = g_path_get_basename(partition_path);
    gboolean is_partition = g_str_has_prefix(partition_name, disk_name);

    g_free(disk_name);
    g_free(partition_name);

    return is_partition;
}

gchar* partitionmanual_format_partition_size(guint64 size_bytes)
{
    // Usar estándar binario: size / 1073741824.0 para GiB
    if (size_bytes >= 1099511627776ULL) {
        return g_strdup_printf("%.2f TiB", size_bytes / 1099511627776.0);
    } else if (size_bytes >= 1073741824ULL) {
        return g_strdup_printf("%.2f GiB", size_bytes / 1073741824.0);
    } else if (size_bytes >= 1048576ULL) {
        return g_strdup_printf("%.2f MiB", size_bytes / 1048576.0);
    } else if (size_bytes >= 1024ULL) {
        return g_strdup_printf("%.2f KiB", size_bytes / 1024.0);
    } else {
        return g_strdup_printf("%lu bytes", size_bytes);
    }
}

const gchar* partitionmanual_get_filesystem_icon(const gchar *filesystem)
{
    if (!filesystem) return "drive-harddisk-symbolic";

    if (g_str_has_prefix(filesystem, "ext")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "ntfs")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "fat32") || g_str_equal(filesystem, "vfat")) {
        return "drive-removable-media-symbolic";
    } else if (g_str_equal(filesystem, "swap")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "btrfs")) {
        return "drive-harddisk-symbolic";
    } else if (g_str_equal(filesystem, "xfs")) {
        return "drive-harddisk-symbolic";
    } else {
        return "drive-harddisk-symbolic";
    }
}

PartitionInfo* partitionmanual_create_partition_info(const gchar *device_path, UDisksPartition *partition, UDisksBlock *block, UDisksObject *object)
{
    if (!device_path || !block) return NULL;

    PartitionInfo *info = g_malloc0(sizeof(PartitionInfo));

    // Información básica
    info->device_path = g_strdup(device_path);
    info->size = udisks_block_get_size(block);
    info->size_formatted = partitionmanual_format_partition_size(info->size);

    // Obtener filesystem
    UDisksFilesystem *filesystem = udisks_object_peek_filesystem(object);
    if (filesystem) {
        info->filesystem = g_strdup(udisks_block_get_id_type(block));

        // Obtener puntos de montaje
        const gchar *const *mount_points = udisks_filesystem_get_mount_points(filesystem);
        if (mount_points && mount_points[0]) {
            info->mount_point = g_strdup(mount_points[0]);
            info->is_mounted = TRUE;
        }
    } else {
        info->filesystem = g_strdup("Sin formato");
        info->is_mounted = FALSE;
    }

    // Obtener label y UUID
    info->label = g_strdup(udisks_block_get_id_label(block));
    info->uuid = g_strdup(udisks_block_get_id_uuid(block));

    // Si no hay label, usar un nombre genérico
    if (!info->label || strlen(info->label) == 0) {
        g_free(info->label);
        info->label = g_strdup(g_path_get_basename(device_path));
    }

    return info;
}

void partitionmanual_update_partition_display(PartitionManualData *data)
{
    if (!data || !data->partition_manager) return;

    // Actualizar visualización de particiones
    // basada en las configuraciones guardadas
    GList *configs = partition_manager_get_configs(data->partition_manager);
    guint config_count = g_list_length(configs);

    LOG_INFO("Actualizando visualización de particiones: %u configuraciones", config_count);

    // Actualizar información en la interfaz si es necesario
    if (config_count > 0) {
        // Mostrar resumen de configuraciones
        LOG_INFO("Particiones configuradas:");
        GList *l;
        for (l = configs; l != NULL; l = l->next) {
            PartitionConfig *config = (PartitionConfig*)l->data;
            if (config) {
                LOG_INFO("  %s -> %s (%s)",
                        config->device_path,
                        config->is_swap ? "swap" : config->mount_point,
                        config->filesystem);
            }
        }
    }
}

void on_partitionmanual_partition_configure_clicked(GtkButton *button, gpointer user_data)
{
    PartitionInfo *partition = (PartitionInfo*)user_data;

    if (!partition || !g_partitionmanual_data || !g_partitionmanual_data->partition_manager) return;

    LOG_INFO("Configurando partición: %s", partition->device_path);

    // Mostrar diálogo de configuración de partición
    partition_manager_show_dialog(g_partitionmanual_data->partition_manager,
                                 partition->device_path,
                                 partition->filesystem,
                                 partition->mount_point,
                                 NULL);
}

void on_partitionmanual_config_saved(PartitionConfig *config, gpointer user_data)
{
    PartitionManualData *data = (PartitionManualData*)user_data;

    if (!data || !config) return;

    LOG_INFO("Configuración de partición guardada desde partitionmanual: %s", config->device_path);

    // Actualizar visualización
    partitionmanual_update_partition_display(data);
}
