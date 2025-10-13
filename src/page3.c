#include "page3.h"

#include "disk_manager.h"
#include "partition_manager.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <udisks/udisks.h>

// Variable global para datos de la página 3
static Page3Data *g_page3_data = NULL;
static DiskManager *g_disk_manager = NULL;

// Funciones privadas
static void page3_setup_disk_manager(Page3Data *data, GtkBuilder *page_builder);
static gboolean page3_validate_selection(Page3Data *data);
static void page3_save_configuration(Page3Data *data);


// Función principal de inicialización de la página 3
void page3_init(GtkBuilder *builder, AdwCarousel *carousel, GtkRevealer *revealer)
{
    // Allocar memoria para los datos de la página
    g_page3_data = g_malloc0(sizeof(Page3Data));

    // Guardar referencias importantes
    g_page3_data->carousel = carousel;
    g_page3_data->revealer = revealer;

    // Cargar la página 3 desde el archivo UI
    GtkBuilder *page_builder = gtk_builder_new_from_resource("/org/gtk/arcris/page3.ui");
    GtkWidget *page3 = GTK_WIDGET(gtk_builder_get_object(page_builder, "navigation_view"));

    if (!page3) {
        LOG_ERROR("No se pudo cargar la página 3 desde el archivo UI");
        g_object_unref(page_builder);
        return;
    }

    // Obtener widgets específicos de la página
    g_page3_data->main_content = page3;
    g_page3_data->navigation_view = ADW_NAVIGATION_VIEW(gtk_builder_get_object(page_builder, "navigation_view"));
    g_page3_data->disk_combo = ADW_COMBO_ROW(gtk_builder_get_object(page_builder, "disk_combo"));
    g_page3_data->auto_partition_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "auto_partition_radio"));
    g_page3_data->auto_btrfs_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "auto_btrfs_radio"));
    g_page3_data->cifrado_partition_button = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "cifrado_partition_button"));
    g_page3_data->manual_partition_radio = GTK_CHECK_BUTTON(gtk_builder_get_object(page_builder, "manual_partition_radio"));
    g_page3_data->refresh_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "refresh_button"));
    g_page3_data->configure_partitions_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "configure_partitions_button"));
    g_page3_data->save_key_disk_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "save_key_disk_button"));

    // Obtener widgets de la página de particiones manuales
    g_page3_data->disk_label_page4 = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_label_page4"));
    g_page3_data->disk_size_label_page4 = GTK_LABEL(gtk_builder_get_object(page_builder, "disk_size_label_page4"));
    g_page3_data->gparted_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "gparted_button"));
    g_page3_data->refresh_partitions_button = GTK_BUTTON(gtk_builder_get_object(page_builder, "refresh_partitions_button"));
    g_page3_data->return_disks = GTK_BUTTON(gtk_builder_get_object(page_builder, "return_disks"));
    g_page3_data->return_disks_encryption = ADW_BUTTON_ROW(gtk_builder_get_object(page_builder, "return_disks_encryption"));
    g_page3_data->partitions_group = ADW_PREFERENCES_GROUP(gtk_builder_get_object(page_builder, "partitions_group"));

    // Obtener widgets para particionado cifrado
    g_page3_data->password_entry = ADW_PASSWORD_ENTRY_ROW(gtk_builder_get_object(page_builder, "password_entry"));
    g_page3_data->password_confirm_entry = ADW_PASSWORD_ENTRY_ROW(gtk_builder_get_object(page_builder, "password_confirm_entry"));
    g_page3_data->password_error_label = GTK_LABEL(gtk_builder_get_object(page_builder, "password_error_label"));

    // Verificar que todos los widgets se obtuvieron correctamente
    if (!g_page3_data->navigation_view || !g_page3_data->disk_combo || !g_page3_data->auto_partition_radio ||
        !g_page3_data->auto_btrfs_radio || !g_page3_data->cifrado_partition_button || !g_page3_data->manual_partition_radio ||
        !g_page3_data->refresh_button || !g_page3_data->configure_partitions_button || !g_page3_data->save_key_disk_button ||
        !g_page3_data->disk_label_page4 || !g_page3_data->disk_size_label_page4 ||
        !g_page3_data->gparted_button || !g_page3_data->return_disks || !g_page3_data->return_disks_encryption ||
        !g_page3_data->partitions_group || !g_page3_data->password_entry || !g_page3_data->password_confirm_entry ||
        !g_page3_data->password_error_label) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios de la página 3");
        g_object_unref(page_builder);
        return;
    }

    // Configurar administrador de discos
    page3_setup_disk_manager(g_page3_data, page_builder);

    // Inicializar cliente UDisks2 para obtener información de particiones
    GError *error = NULL;
    g_page3_data->udisks_client = udisks_client_new_sync(NULL, &error);
    if (!g_page3_data->udisks_client) {
        LOG_WARNING("No se pudo inicializar cliente UDisks2: %s", error ? error->message : "Error desconocido");
        if (error) g_error_free(error);
    } else {
        LOG_INFO("Cliente UDisks2 inicializado para page3");
    }

    // Inicializar listas de particiones
    g_page3_data->partitions = NULL;
    g_page3_data->partition_rows = NULL;

    // Inicializar estado del cifrado
    g_page3_data->encryption_enabled = FALSE;
    g_page3_data->passwords_match = FALSE;
    g_page3_data->password_length_valid = FALSE;

    // Realizar configuraciones iniciales específicas de la página 3
    page3_setup_widgets(g_page3_data);
    page3_load_data(g_page3_data);

    // Conectar señales para navegación interna
    g_signal_connect(g_page3_data->configure_partitions_button, "clicked",
                     G_CALLBACK(on_page3_configure_partitions_clicked), g_page3_data);
    g_signal_connect(g_page3_data->manual_partition_radio, "toggled",
                     G_CALLBACK(on_page3_partition_mode_changed), g_page3_data);
    g_signal_connect(g_page3_data->auto_partition_radio, "toggled",
                     G_CALLBACK(on_page3_partition_mode_changed), g_page3_data);
    g_signal_connect(g_page3_data->auto_btrfs_radio, "toggled",
                     G_CALLBACK(on_page3_partition_mode_changed), g_page3_data);
    g_signal_connect(g_page3_data->cifrado_partition_button, "toggled",
                     G_CALLBACK(on_page3_partition_mode_changed), g_page3_data);
    g_signal_connect(g_page3_data->save_key_disk_button, "clicked",
                     G_CALLBACK(on_page3_save_key_disk_clicked), g_page3_data);

    // Conectar señales de los campos de contraseña
    if (g_page3_data->password_entry) {
        g_signal_connect(g_page3_data->password_entry, "changed",
                         G_CALLBACK(on_page3_password_changed), g_page3_data);
    }

    if (g_page3_data->password_confirm_entry) {
        g_signal_connect(g_page3_data->password_confirm_entry, "changed",
                         G_CALLBACK(on_page3_password_confirm_changed), g_page3_data);
    }
    g_signal_connect(g_page3_data->gparted_button, "clicked",
                     G_CALLBACK(on_page3_gparted_button_clicked), g_page3_data);
    g_signal_connect(g_page3_data->refresh_partitions_button, "clicked",
                     G_CALLBACK(on_page3_refresh_partitions_clicked), g_page3_data);
    g_signal_connect(g_page3_data->return_disks, "clicked",
                     G_CALLBACK(on_page3_return_disks_clicked), g_page3_data);
    g_signal_connect(g_page3_data->return_disks_encryption, "activated",
                     G_CALLBACK(on_page3_return_disks_encryption_clicked), g_page3_data);

    // Crear botones de navegación
    page3_create_navigation_buttons(g_page3_data);

    // Añadir la página al carousel
    adw_carousel_append(carousel, page3);

    // Liberar el builder de la página
    g_object_unref(page_builder);

    // Inicializar el manejador de particiones
    page3_init_partition_manager(g_page3_data);

    // Establecer modo por defecto automáticamente
    page3_save_partition_mode("auto");

    // Cargar modo de particionado desde variables.sh
    page3_load_partition_mode(g_page3_data);

    LOG_INFO("Página 3 (Selección de Disco) inicializada correctamente");
}

// Función de limpieza
void page3_cleanup(Page3Data *data)
{
    if (g_page3_data) {
        // Limpiar lista de particiones
        page3_clear_partitions(g_page3_data);

        // Limpiar lista de filas de particiones
        if (g_page3_data->partition_rows) {
            g_list_free(g_page3_data->partition_rows);
            g_page3_data->partition_rows = NULL;
        }

        // Limpiar cliente UDisks2
        if (g_page3_data->udisks_client) {
            g_object_unref(g_page3_data->udisks_client);
            g_page3_data->udisks_client = NULL;
        }

        // Liberar el administrador de discos
        if (g_disk_manager) {
            disk_manager_free(g_disk_manager);
            g_disk_manager = NULL;
        }

        // Liberar el manejador de particiones
        if (g_page3_data->partition_manager) {
            partition_manager_free(g_page3_data->partition_manager);
            g_page3_data->partition_manager = NULL;
        }

        g_free(g_page3_data);
        g_page3_data = NULL;
        LOG_INFO("Página 3 limpiada correctamente");
    }
}

// Configurar el administrador de discos
static void page3_setup_disk_manager(Page3Data *data, GtkBuilder *page_builder)
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

    LOG_INFO("DiskManager configurado correctamente para la página 3");
}

// Función para configurar widgets
void page3_setup_widgets(Page3Data *data)
{
    if (!data) return;

    // Configurar estado inicial de los radio buttons
    gtk_check_button_set_active(data->auto_partition_radio, TRUE);

    // Configurar estado inicial del botón de configurar particiones
    if (data->configure_partitions_button) {
        gtk_widget_set_sensitive(GTK_WIDGET(data->configure_partitions_button), FALSE);
    }

    // Obtener los widgets AdwActionRow correctos navegando por el árbol de widgets
    GtkWidget *auto_partition_row = GTK_WIDGET(data->auto_partition_radio);
    while (auto_partition_row && !ADW_IS_ACTION_ROW(auto_partition_row)) {
        auto_partition_row = gtk_widget_get_parent(auto_partition_row);
    }

    GtkWidget *auto_btrfs_row = GTK_WIDGET(data->auto_btrfs_radio);
    while (auto_btrfs_row && !ADW_IS_ACTION_ROW(auto_btrfs_row)) {
        auto_btrfs_row = gtk_widget_get_parent(auto_btrfs_row);
    }

    GtkWidget *cifrado_row = GTK_WIDGET(data->cifrado_partition_button);
    while (cifrado_row && !ADW_IS_ACTION_ROW(cifrado_row)) {
        cifrado_row = gtk_widget_get_parent(cifrado_row);
    }

}

// Función para cargar datos
void page3_load_data(Page3Data *data)
{
    if (!data || !g_disk_manager) return;

    // Cargar configuración guardada desde variables.sh
    disk_manager_load_from_variables(g_disk_manager);

    // Auto-seleccionar siempre la opción 0 del disk_combo
    page3_auto_select_disk_option_1();

    // Actualizar UI según la configuración cargada
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (selected_disk) {
        LOG_INFO("Disco previamente seleccionado: %s", selected_disk);
    }

    // Configurar modo de particionado por defecto
    if (data->auto_partition_radio) {
        gtk_check_button_set_active(data->auto_partition_radio, TRUE);
    }

    LOG_INFO("Datos de la página 3 cargados");
}

// Validar selección antes de continuar
static gboolean page3_validate_selection(Page3Data *data)
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

    // Verificar que se haya seleccionado un modo de particionado
    if (data->auto_partition_radio && gtk_check_button_get_active(data->auto_partition_radio)) {
        LOG_INFO("Modo de particionado seleccionado: Auto Partition");
    } else if (data->auto_btrfs_radio && gtk_check_button_get_active(data->auto_btrfs_radio)) {
        LOG_INFO("Modo de particionado seleccionado: Auto Btrfs");
    } else if (data->manual_partition_radio && gtk_check_button_get_active(data->manual_partition_radio)) {
        LOG_INFO("Modo de particionado seleccionado: Manual Partition");
    }

    return TRUE;
}

// Guardar configuración actual
static void page3_save_configuration(Page3Data *data)
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
gboolean page3_go_to_next_page(Page3Data *data)
{
    if (!data || !g_disk_manager) return FALSE;

    // Validar selección
    if (!page3_validate_selection(data)) {
        return FALSE;
    }

    // Guardar configuración
    page3_save_configuration(data);

    // Ir siempre a page4 (la siguiente página en secuencia)
    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    LOG_INFO("Dirigiendo a página 4 para disco: %s", selected_disk);

    // Ir a page4 (índice 3 en el carousel)
    if (data->carousel) {
        GtkWidget *page4 = adw_carousel_get_nth_page(data->carousel, 3);
        if (page4) {
            adw_carousel_scroll_to(data->carousel, page4, TRUE);
            // La actualización del disco ahora se maneja internamente en page3
        } else {
            LOG_ERROR("No se pudo encontrar page4 en el carousel");
        }
    }

    return TRUE;
}

// Función para obtener el disco seleccionado (para uso externo)
const char* page3_get_selected_disk(void)
{
    if (!g_disk_manager) return NULL;
    return disk_manager_get_selected_disk(g_disk_manager);
}

// Función para crear botones de navegación programáticamente
void page3_create_navigation_buttons(Page3Data *data)
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
                     G_CALLBACK(on_page3_back_button_clicked), data);
    g_signal_connect(next_button, "clicked",
                     G_CALLBACK(on_page3_next_button_clicked), data);

    // Buscar el contenedor principal de page3 y agregar navegación
    GtkWidget *status_page = gtk_widget_get_first_child(data->main_content);
    if (status_page) {
        GtkWidget *status_child = gtk_widget_get_first_child(status_page);
        if (status_child && GTK_IS_BOX(status_child)) {
            gtk_box_append(GTK_BOX(status_child), navigation_box);
        }
    }

    LOG_INFO("Botones de navegación creados para página 3");
}

// Función para ir a la página anterior
gboolean page3_go_to_previous_page(Page3Data *data)
{
    if (!data) return FALSE;

    LOG_INFO("Regresando a página anterior desde página 3");

    // Ir a la página anterior en el carousel (page2, índice 1)
    if (data->carousel) {
        GtkWidget *page2 = adw_carousel_get_nth_page(data->carousel, 1);
        if (page2) {
            adw_carousel_scroll_to(data->carousel, page2, TRUE);
        } else {
            LOG_ERROR("No se pudo encontrar page2 en el carousel");
        }
    }

    return TRUE;
}

// Función para obtener el modo de particionado (para uso externo)
DiskMode page3_get_partition_mode(void)
{
    if (!g_page3_data) return DISK_MODE_AUTO_PARTITION;

    if (g_page3_data->auto_btrfs_radio && gtk_check_button_get_active(g_page3_data->auto_btrfs_radio)) {
        return DISK_MODE_AUTO_BTRFS;
    } else if (g_page3_data->cifrado_partition_button && gtk_check_button_get_active(g_page3_data->cifrado_partition_button)) {
        return DISK_MODE_CIFRADO;
    } else if (g_page3_data->manual_partition_radio && gtk_check_button_get_active(g_page3_data->manual_partition_radio)) {
        return DISK_MODE_MANUAL_PARTITION;
    }

    return DISK_MODE_AUTO_PARTITION;
}

// Función para refrescar la lista de discos (para uso externo)
void page3_refresh_disk_list(void)
{
    if (!g_disk_manager) return;
    disk_manager_refresh(g_disk_manager);
    
    // Auto-seleccionar siempre la opción 0 después del refresh
    page3_auto_select_disk_option_1();
}

// Estructura para pasar datos al callback de timeout
typedef struct {
    int retry_count;
    int max_retries;
} AutoSelectData;

// Callback para auto-selección con timeout
static gboolean page3_auto_select_timeout_callback(gpointer user_data)
{
    AutoSelectData *data = (AutoSelectData *)user_data;
    
    if (!g_page3_data || !g_page3_data->disk_combo || !g_disk_manager) {
        LOG_WARNING("Auto-selección fallida: datos no inicializados (intento %d/%d)", 
                   data->retry_count + 1, data->max_retries);
        data->retry_count++;
        
        if (data->retry_count >= data->max_retries) {
            g_free(data);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }

    // Obtener el número total de opciones disponibles
    GtkStringList *model = GTK_STRING_LIST(adw_combo_row_get_model(g_page3_data->disk_combo));
    if (!model) {
        LOG_WARNING("Auto-selección fallida: modelo no encontrado (intento %d/%d)", 
                   data->retry_count + 1, data->max_retries);
        data->retry_count++;
        
        if (data->retry_count >= data->max_retries) {
            g_free(data);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }

    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(model));
    
    // Si no hay discos, continuar esperando
    if (n_items == 0) {
        LOG_INFO("Esperando que se carguen los discos (intento %d/%d)", 
                data->retry_count + 1, data->max_retries);
        data->retry_count++;
        
        if (data->retry_count >= data->max_retries) {
            LOG_WARNING("Timeout: no se encontraron discos después de %d intentos", data->max_retries);
            g_free(data);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }
    
    // Seleccionar la opción 0 (índice 0, que es la primera opción)
    adw_combo_row_set_selected(g_page3_data->disk_combo, 0);
    
    // Obtener el path real del disco (sin formato) del disk_manager
    const gchar *selected_disk_path = NULL;
    if (g_disk_manager) {
        // Simular el callback de selección para obtener el path correcto
        on_disk_manager_selection_changed(G_OBJECT(g_page3_data->disk_combo), NULL, g_disk_manager);
        selected_disk_path = disk_manager_get_selected_disk(g_disk_manager);
    }
    
    if (!selected_disk_path) {
        LOG_WARNING("No se puede obtener el path del disco después de la selección (intento %d/%d)", 
                   data->retry_count + 1, data->max_retries);
        data->retry_count++;
        
        if (data->retry_count >= data->max_retries) {
            g_free(data);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }
    
    LOG_INFO("Opción 0 del disk_combo seleccionada automáticamente: %s", selected_disk_path);
    LOG_INFO("Total de discos disponibles: %u", n_items);
    
    // Obtener el texto de la opción seleccionada para logging (texto formateado)
    const gchar *selected_option = gtk_string_list_get_string(model, 0);
    if (selected_option) {
        LOG_INFO("Texto mostrado: %s", selected_option);
    }
    
    g_free(data);
    return G_SOURCE_REMOVE;
}

// Función para seleccionar automáticamente la opción 0 del disk_combo
void page3_auto_select_disk_option_1(void)
{
    // Crear datos para el callback de timeout
    AutoSelectData *data = g_malloc0(sizeof(AutoSelectData));
    data->retry_count = 0;
    data->max_retries = 10; // Máximo 10 intentos (2 segundos total)
    
    // Programar el callback con un pequeño delay para asegurar que la lista esté poblada
    g_timeout_add(200, page3_auto_select_timeout_callback, data); // 200ms de delay inicial
    
    LOG_INFO("Auto-selección de disco programada con timeout de 200ms");
}

// Función para verificar si la configuración es válida
gboolean page3_is_configuration_valid(void)
{
    if (!g_page3_data) return FALSE;
    return page3_validate_selection(g_page3_data);
}

// Callback para el botón de siguiente (si se necesita integración externa)
// static void on_page3_next_button_clicked(GtkButton *button, gpointer user_data)
// {
//     Page3Data *data = (Page3Data *)user_data;
//
//     if (page3_go_to_next_page(data)) {
//         LOG_INFO("Avanzando desde página 3");
//
//         // Aquí se podría emitir una señal o llamar a una función del carousel
//         // para avanzar a la siguiente página
//
//         // Por ejemplo:
//         // carousel_manager_go_to_next_page();
//     }
// }

// Funciones de callback que pueden ser conectadas externamente

void on_page3_disk_selection_changed(AdwComboRow *combo, GParamSpec *param, gpointer user_data)
{
    // Este callback ya se maneja en disk_manager.c
    // Se incluye aquí por compatibilidad si se necesita lógica adicional

    Page3Data *data = (Page3Data *)user_data;
    if (!data || !g_disk_manager) return;

    const char *selected_disk = disk_manager_get_selected_disk(g_disk_manager);
    if (selected_disk) {
        LOG_INFO("Disco seleccionado en página 3: %s", selected_disk);

        // Limpiar configuraciones del disco anterior
        page3_clear_previous_disk_configs(data, selected_disk);

        // Refrescar vista con particiones del nuevo disco
        page3_update_manual_partitions_info(data);
        page3_update_all_partition_subtitles(data);
    }
}

void on_page3_partition_mode_changed(GtkCheckButton *button, gpointer user_data)
{
    LOG_INFO("=== on_page3_partition_mode_changed INICIADO ===");

    Page3Data *data = (Page3Data *)user_data;
    if (!data) {
        LOG_ERROR("on_page3_partition_mode_changed: data es NULL");
        return;
    }

    if (!g_disk_manager) {
        LOG_ERROR("on_page3_partition_mode_changed: g_disk_manager es NULL");
        return;
    }

    // Determinar el modo de particionado actual
    gboolean is_manual = gtk_check_button_get_active(data->manual_partition_radio);
    gboolean is_auto = gtk_check_button_get_active(data->auto_partition_radio);
    gboolean is_auto_btrfs = gtk_check_button_get_active(data->auto_btrfs_radio);
    gboolean is_cifrado = gtk_check_button_get_active(data->cifrado_partition_button);

    LOG_INFO("Estados de radio buttons: manual=%s, auto=%s, auto_btrfs=%s, cifrado=%s",
             is_manual ? "TRUE" : "FALSE",
             is_auto ? "TRUE" : "FALSE",
             is_auto_btrfs ? "TRUE" : "FALSE",
             is_cifrado ? "TRUE" : "FALSE");

    // Guardar modo en variables.sh
    const gchar *partition_mode = "auto";
    if (is_manual) {
        partition_mode = "manual";
    } else if (is_auto_btrfs) {
        partition_mode = "auto_btrfs";
    } else if (is_cifrado) {
        partition_mode = "cifrado";
    } else if (is_auto) {
        partition_mode = "auto";
    }

    LOG_INFO("Modo de particionado determinado: %s", partition_mode);

    // Guardar en variables.sh
    LOG_INFO("Guardando modo de particionado en variables.sh...");
    page3_save_partition_mode(partition_mode);

    // Habilitar/deshabilitar botón de configurar particiones según el modo
    if (data->configure_partitions_button) {
        gtk_widget_set_sensitive(GTK_WIDGET(data->configure_partitions_button), is_manual);
        LOG_INFO("Botón configurar particiones %s", is_manual ? "ACTIVADO" : "DESACTIVADO");
    } else {
        LOG_WARNING("configure_partitions_button es NULL");
    }

    // Actualizar estado del botón de cifrado
    page3_update_encryption_button_state(data);

    // Habilitar/deshabilitar botón siguiente según el modo
    // En modo manual y cifrado, el botón siguiente se desactiva hasta que se configure
    LOG_INFO("Actualizando sensibilidad del botón siguiente...");
    page3_update_next_button_sensitivity(data, is_manual || is_cifrado);

    LOG_INFO("Modo de particionado cambiado a: %s", partition_mode);
    LOG_INFO("=== on_page3_partition_mode_changed FINALIZADO ===");
}

void on_page3_refresh_clicked(GtkButton *button, gpointer user_data)
{
    LOG_INFO("Actualizando lista de discos desde página 3");
    page3_refresh_disk_list();
}

// Callback para el botón de siguiente
void on_page3_next_button_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;

    if (page3_go_to_next_page(data)) {
        LOG_INFO("Navegación exitosa desde página 3");
    } else {
        LOG_WARNING("No se pudo navegar desde página 3");
    }
}

// Callback para el botón atrás
void on_page3_back_button_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;

    if (page3_go_to_previous_page(data)) {
        LOG_INFO("Navegación hacia atrás exitosa desde página 3");
    } else {
        LOG_WARNING("No se pudo navegar hacia atrás desde página 3");
    }
}

// Callback para el botón de configurar particiones
void on_page3_configure_partitions_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data || !data->navigation_view) return;

    LOG_INFO("Navegando a configuración de particiones manuales");
    page3_navigate_to_manual_partitions(data);
}

// Callback para el botón de Gparted
void on_page3_gparted_button_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data) return;

    const char *selected_disk = page3_get_selected_disk();
    if (!selected_disk) {
        LOG_WARNING("No hay disco seleccionado para abrir Gparted");
        return;
    }

    LOG_INFO("Abriendo Gparted para disco: %s", selected_disk);

    // Construir comando para abrir Gparted
    gchar *command = g_strdup_printf("sudo pkexec gparted %s", selected_disk);

    GError *error = NULL;
    gboolean success = g_spawn_command_line_async(command, &error);

    if (!success) {
        LOG_ERROR("Error al abrir Gparted: %s", error ? error->message : "Error desconocido");
        if (error) g_error_free(error);
    }

    g_free(command);
}

// Callback para actualizar particiones
void on_page3_refresh_partitions_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data) return;

    LOG_INFO("Actualizando información de particiones");
    page3_update_manual_partitions_info(data);
}

// Función para navegar a la página de particiones manuales
void page3_navigate_to_manual_partitions(Page3Data *data)
{
    if (!data || !data->navigation_view) return;

    // Actualizar información antes de navegar
    page3_update_manual_partitions_info(data);

    // Navegar a la página de particiones manuales
    AdwNavigationPage *manual_page = adw_navigation_view_find_page(data->navigation_view, "manual_partitions");
    if (manual_page) {
        adw_navigation_view_push(data->navigation_view, manual_page);
        LOG_INFO("Navegación a particiones manuales exitosa");
    } else {
        LOG_ERROR("No se pudo encontrar la página de particiones manuales");
    }
}

// Función para navegar de regreso a la selección de disco
void page3_navigate_back_to_disk_selection(Page3Data *data)
{
    if (!data || !data->navigation_view) return;

    // Regresar a la página principal
    adw_navigation_view_pop(data->navigation_view);
    LOG_INFO("Navegación de regreso a selección de disco");

    // Verificar el estado del botón siguiente al regresar
    if (data->manual_partition_radio && gtk_check_button_get_active(data->manual_partition_radio)) {
        LOG_INFO("Verificando estado del botón siguiente al regresar de configuración manual");
        page3_update_next_button_sensitivity(data, TRUE);
    }
}

// Función para actualizar información de particiones manuales
void page3_update_manual_partitions_info(Page3Data *data)
{
    if (!data) {
        LOG_WARNING("page3_update_manual_partitions_info: data es NULL");
        return;
    }

    if (!data->udisks_client) {
        LOG_WARNING("page3_update_manual_partitions_info: udisks_client no inicializado");
        return;
    }

    LOG_INFO("=== INICIANDO page3_update_manual_partitions_info ===");

    const char *selected_disk = page3_get_selected_disk();
    if (!selected_disk) {
        LOG_WARNING("No hay disco seleccionado para mostrar información");
        return;
    }

    if (strlen(selected_disk) == 0) {
        LOG_WARNING("Disco seleccionado está vacío");
        return;
    }

    LOG_INFO("Disco seleccionado: %s", selected_disk);

    // Verificar que los widgets existan
    if (!data->disk_label_page4) {
        LOG_ERROR("disk_label_page4 es NULL");
        return;
    }
    if (!data->disk_size_label_page4) {
        LOG_ERROR("disk_size_label_page4 es NULL");
        return;
    }

    // Actualizar etiquetas de disco
    gchar *disk_text = g_strdup_printf("Disco %s", selected_disk);
    gtk_label_set_text(data->disk_label_page4, disk_text);
    LOG_INFO("Label de disco actualizado: %s", disk_text);
    g_free(disk_text);

    // Obtener información completa del disco
    LOG_INFO("Obteniendo tamaño del disco...");
    gchar *disk_size = page3_get_disk_size(selected_disk);
    LOG_INFO("Tamaño obtenido: %s", disk_size ? disk_size : "NULL");

    LOG_INFO("Obteniendo tipo de tabla de particiones...");
    gchar *partition_table = page3_get_partition_table_type(selected_disk);
    LOG_INFO("Tabla de particiones obtenida: %s", partition_table ? partition_table : "NULL");

    LOG_INFO("Obteniendo tipo de firmware...");
    gchar *firmware_type = page3_get_firmware_type();
    LOG_INFO("Firmware obtenido: %s", firmware_type ? firmware_type : "NULL");

    if (disk_size && partition_table && firmware_type) {
        gchar *complete_info = g_strdup_printf("%s - %s - %s", disk_size, partition_table, firmware_type);
        LOG_INFO("Información completa generada: %s", complete_info);
        gtk_label_set_text(data->disk_size_label_page4, complete_info);
        LOG_INFO("Label disk_size_label_page4 actualizado exitosamente");
        g_free(complete_info);
    } else {
        LOG_WARNING("Información incompleta, usando texto por defecto");
        gtk_label_set_text(data->disk_size_label_page4, "Información desconocida");
    }

    g_free(disk_size);
    g_free(partition_table);
    g_free(firmware_type);

    LOG_INFO("=== FINALIZANDO page3_update_manual_partitions_info ===");

    // Limpiar particiones anteriores
    page3_clear_partitions(data);

    // Poblar con nuevas particiones
    page3_populate_partitions(data, selected_disk);

    LOG_INFO("Información de particiones actualizada para disco: %s", selected_disk);
}

// Callback para el botón de regresar a selección de discos
void on_page3_return_disks_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data || !data->navigation_view) return;

    LOG_INFO("Regresando a selección de discos");
    page3_navigate_back_to_disk_selection(data);
}

void on_page3_return_disks_encryption_clicked(AdwButtonRow *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data || !data->navigation_view) return;

    LOG_INFO("Regresando a selección de discos desde página de encriptación");

    // Desactivar el botón siguiente al regresar de la página de encriptación
    GtkWidget *next_button = NULL;
    if (data->revealer) {
        GtkWidget *revealer_child = gtk_revealer_get_child(data->revealer);
        if (revealer_child) {
            next_button = page3_find_next_button_recursive(revealer_child);
        }
    }

    if (next_button) {
        gtk_widget_set_sensitive(next_button, FALSE);
        LOG_INFO("Botón siguiente desactivado al regresar de página de encriptación");
    } else {
        LOG_WARNING("No se pudo encontrar el botón siguiente para desactivar");
    }

    page3_navigate_back_to_disk_selection(data);
}

// Función para obtener el tamaño del disco
gchar* page3_get_disk_size(const gchar *disk_path)
{
    if (!disk_path) return NULL;

    // Usar udisks2 para obtener información del disco
    gchar *command = g_strdup_printf("lsblk -b -d -n -o SIZE %s", disk_path);

    gchar *output = NULL;
    GError *error = NULL;

    if (g_spawn_command_line_sync(command, &output, NULL, NULL, &error)) {
        if (output) {
            g_strstrip(output);
            guint64 size_bytes = g_ascii_strtoull(output, NULL, 10);
            g_free(output);
            g_free(command);

            if (size_bytes > 0) {
                return page3_format_disk_size(size_bytes);
            }
        }
    }

    if (error) {
        LOG_ERROR("Error obteniendo tamaño del disco: %s", error->message);
        g_error_free(error);
    }

    g_free(command);
    g_free(output);
    return NULL;
}

// Función para formatear el tamaño del disco
gchar* page3_format_disk_size(guint64 size_bytes)
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

// Función de comparación para ordenar particiones por número
static gint page3_partition_compare_func(gconstpointer a, gconstpointer b)
{
    const Page3PartitionInfo *info_a = (const Page3PartitionInfo*)a;
    const Page3PartitionInfo *info_b = (const Page3PartitionInfo*)b;

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

// Función para limpiar particiones del grupo
void page3_clear_partitions(Page3Data *data)
{
    if (!data) return;

    LOG_INFO("Iniciando limpieza de particiones...");

    // Primero limpiar las filas de particiones usando las referencias almacenadas
    if (data->partition_rows && data->partitions_group) {
        LOG_INFO("Eliminando %d filas de particiones", g_list_length(data->partition_rows));

        GList *current_row = data->partition_rows;
        while (current_row) {
            AdwActionRow *row = ADW_ACTION_ROW(current_row->data);
            if (row && GTK_IS_WIDGET(row)) {
                // Remover la fila del grupo de particiones
                adw_preferences_group_remove(data->partitions_group, GTK_WIDGET(row));
                LOG_INFO("Fila de partición removida");
            }
            current_row = current_row->next;
        }

        // Limpiar la lista de referencias a filas
        g_list_free(data->partition_rows);
        data->partition_rows = NULL;
    }

    // Limpiar lista de información de particiones
    if (data->partitions) {
        GList *current = data->partitions;
        while (current) {
            Page3PartitionInfo *info = (Page3PartitionInfo*)current->data;
            page3_free_partition_info(info);
            current = current->next;
        }
        g_list_free(data->partitions);
        data->partitions = NULL;
    }

    LOG_INFO("Particiones y filas de interfaz limpiadas");
}

// Función para poblar particiones
void page3_populate_partitions(Page3Data *data, const gchar *disk_path)
{
    if (!data) {
        LOG_WARNING("page3_populate_partitions: data es NULL");
        return;
    }
    
    if (!disk_path || strlen(disk_path) == 0) {
        LOG_WARNING("page3_populate_partitions: disk_path es NULL o está vacío");
        return;
    }
    
    if (!data->udisks_client) {
        LOG_WARNING("page3_populate_partitions: udisks_client no está inicializado");
        return;
    }

    LOG_INFO("Obteniendo particiones del disco: %s", disk_path);

    // Limpiar particiones anteriores
    page3_clear_partitions(data);

    LOG_INFO("Particiones anteriores limpiadas, iniciando búsqueda...");

    // Obtener todos los objetos de UDisks2
    GList *objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(data->udisks_client));

    int partition_count = 0;
    for (GList *l = objects; l != NULL; l = l->next) {
        UDisksObject *object = UDISKS_OBJECT(l->data);
        UDisksPartition *partition = udisks_object_peek_partition(object);
        UDisksBlock *block = udisks_object_peek_block(object);

        if (partition && block) {
            const gchar *partition_device = udisks_block_get_device(block);

            // Verificar si esta partición pertenece al disco seleccionado
            if (partition_device && page3_is_partition_of_disk(partition_device, disk_path)) {
                LOG_INFO("Partición encontrada: %s", partition_device);

                // Crear información de la partición
                Page3PartitionInfo *info = page3_create_partition_info(partition_device, partition, block, object);
                if (info) {
                    // Añadir a la lista
                    data->partitions = g_list_append(data->partitions, info);
                    partition_count++;
                }
            }
        }
    }

    g_list_free_full(objects, g_object_unref);

    // Ordenar la lista de particiones por número
    if (data->partitions) {
        data->partitions = g_list_sort(data->partitions, page3_partition_compare_func);
        LOG_INFO("Particiones ordenadas por número");

        // Añadir particiones ordenadas a la interfaz
        GList *current = data->partitions;
        while (current) {
            Page3PartitionInfo *info = (Page3PartitionInfo*)current->data;
            page3_add_partition_row(data, info);
            current = current->next;
        }

        // Actualizar subtítulos con configuraciones guardadas del disco actual
        page3_update_all_partition_subtitles(data);
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

    LOG_INFO("Particiones encontradas: %d", partition_count);
}

// Función para agregar una fila de partición
void page3_add_partition_row(Page3Data *data, Page3PartitionInfo *partition)
{
    if (!data || !partition) return;

    // Crear AdwActionRow
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());

    // Configurar título y subtítulo
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), partition->device_path);

    // Obtener configuración guardada para esta partición del disco actual
    PartitionConfig *config = NULL;
    const char *current_disk = page3_get_selected_disk();
    if (data->partition_manager && current_disk &&
        page3_is_partition_of_disk(partition->device_path, current_disk)) {
        config = partition_manager_find_config(data->partition_manager, partition->device_path);
    }

    gchar *subtitle;
    if (config) {
        // Mostrar configuración programada
        GString *subtitle_str = g_string_new(partition->size_formatted);

        // Mostrar filesystem actual vs configurado
        if (config->format_needed || g_strcmp0(partition->filesystem, config->filesystem) != 0) {
            g_string_append_printf(subtitle_str, " • %s → %s",
                                   partition->filesystem,
                                   config->filesystem);
            if (config->format_needed) {
                g_string_append(subtitle_str, " (formatear)");
            }
        } else {
            g_string_append_printf(subtitle_str, " • %s", partition->filesystem);
        }

        // Mostrar punto de montaje configurado
        if (config->mount_point) {
            if (config->is_swap) {
                g_string_append(subtitle_str, " • swap");
            } else {
                g_string_append_printf(subtitle_str, " • %s", config->mount_point);
            }
        } else if (partition->mount_point) {
            g_string_append_printf(subtitle_str, " • %s (actual)", partition->mount_point);
        }

        subtitle = g_string_free(subtitle_str, FALSE);
    } else {
        // Mostrar información actual sin configuración
        subtitle = g_strdup_printf("%s • %s%s%s",
                                   partition->size_formatted,
                                   partition->filesystem,
                                   partition->mount_point ? " • " : "",
                                   partition->mount_point ? partition->mount_point : "");
    }

    adw_action_row_set_subtitle(row, subtitle);
    g_free(subtitle);

    // Añadir icono según el tipo de filesystem
    GtkImage *icon = GTK_IMAGE(gtk_image_new_from_icon_name(page3_get_filesystem_icon(partition->filesystem)));
    adw_action_row_add_prefix(row, GTK_WIDGET(icon));

    // Añadir botón de configuración
    GtkButton *config_button = GTK_BUTTON(gtk_button_new_from_icon_name("list-add-symbolic"));
    gtk_widget_set_valign(GTK_WIDGET(config_button), GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(GTK_WIDGET(config_button), "Configurar partición");

    // Añadir clases CSS
    gtk_widget_add_css_class(GTK_WIDGET(config_button), "flat");
    gtk_widget_add_css_class(GTK_WIDGET(config_button), "circular");

    // Conectar señal
    g_signal_connect(config_button, "clicked", G_CALLBACK(on_page3_partition_configure_clicked), partition);

    adw_action_row_add_suffix(row, GTK_WIDGET(config_button));

    // Añadir al grupo
    adw_preferences_group_add(data->partitions_group, GTK_WIDGET(row));

    // Guardar referencia a la fila para poder eliminarla después
    data->partition_rows = g_list_append(data->partition_rows, row);

    LOG_INFO("Fila de partición añadida: %s", partition->device_path);
}

// Función para crear información de partición
Page3PartitionInfo* page3_create_partition_info(const gchar *device_path, UDisksPartition *partition, UDisksBlock *block, UDisksObject *object)
{
    if (!device_path || !block) return NULL;

    Page3PartitionInfo *info = g_malloc0(sizeof(Page3PartitionInfo));

    // Información básica
    info->device_path = g_strdup(device_path);
    info->size = udisks_block_get_size(block);
    info->size_formatted = page3_format_partition_size(info->size);

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

// Función para liberar información de partición
void page3_free_partition_info(Page3PartitionInfo *info)
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

// Función para verificar si una partición pertenece a un disco
gboolean page3_is_partition_of_disk(const gchar *partition_path, const gchar *disk_path)
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

// Función para formatear tamaño de partición (usando mismo método que DiskManager)
gchar* page3_format_partition_size(guint64 size_bytes)
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

// Función para obtener icono según el filesystem
const gchar* page3_get_filesystem_icon(const gchar *filesystem)
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

// Función pública para actualizar particiones (llamada desde disk_manager)
void page3_refresh_partitions(void)
{
    if (!g_page3_data) return;

    const char *selected_disk = page3_get_selected_disk();
    if (selected_disk) {
        LOG_INFO("Refrescando particiones para disco: %s", selected_disk);
        page3_update_manual_partitions_info(g_page3_data);
    }
}

// Función callback para cuando cambia el disco seleccionado
void page3_on_disk_changed(const gchar *disk_path)
{
    if (!g_page3_data) return;

    LOG_INFO("Disco cambiado en page3: %s", disk_path ? disk_path : "NULL");

    // Limpiar configuraciones del disco anterior
    if (disk_path) {
        page3_clear_previous_disk_configs(g_page3_data, disk_path);
    }

    // Actualizar información del disco si estamos en la página de particiones manuales
    if (disk_path) {
        page3_update_manual_partitions_info(g_page3_data);
        // Actualizar subtítulos para reflejar que no hay configuraciones del disco anterior
        page3_update_all_partition_subtitles(g_page3_data);
    }
}

// Función para limpiar configuraciones de particiones que no pertenecen al disco actual
void page3_clear_previous_disk_configs(Page3Data *data, const gchar *current_disk_path)
{
    if (!data || !data->partition_manager || !current_disk_path) return;

    LOG_INFO("Limpiando configuraciones de particiones no pertenecientes al disco: %s", current_disk_path);

    // Obtener todas las configuraciones actuales
    GList *configs = partition_manager_get_configs(data->partition_manager);
    GList *configs_to_remove = NULL;

    // Encontrar configuraciones que no pertenecen al disco actual
    GList *current = configs;
    while (current) {
        PartitionConfig *config = (PartitionConfig*)current->data;

        // Verificar si la partición pertenece al disco actual
        if (!page3_is_partition_of_disk(config->device_path, current_disk_path)) {
            configs_to_remove = g_list_prepend(configs_to_remove, g_strdup(config->device_path));
            LOG_INFO("Marcando para eliminar configuración: %s (no pertenece a %s)",
                     config->device_path, current_disk_path);
        }

        current = current->next;
    }

    // Eliminar las configuraciones que no pertenecen al disco actual
    current = configs_to_remove;
    while (current) {
        gchar *device_path = (gchar*)current->data;
        partition_manager_remove_config(data->partition_manager, device_path);
        LOG_INFO("Configuración eliminada: %s", device_path);
        g_free(device_path);
        current = current->next;
    }

    g_list_free(configs_to_remove);

    LOG_INFO("Limpieza de configuraciones completada para disco: %s", current_disk_path);
}

// Función para obtener el disco actualmente seleccionado
gchar* page3_get_current_selected_disk(Page3Data *data)
{
    if (!data || !data->disk_combo) return NULL;

    // Obtener el disco seleccionado del combo
    const gchar *selected_disk = page3_get_selected_disk();
    return selected_disk ? g_strdup(selected_disk) : NULL;
}

// Función para verificar si una configuración pertenece al disco actual
gboolean page3_config_belongs_to_current_disk(Page3Data *data, const gchar *device_path)
{
    if (!data || !device_path) return FALSE;

    gchar *current_disk = page3_get_current_selected_disk(data);
    if (!current_disk) return FALSE;

    gboolean belongs = page3_is_partition_of_disk(device_path, current_disk);
    g_free(current_disk);

    return belongs;
}

// Callback para configurar partición
void on_page3_partition_configure_clicked(GtkButton *button, gpointer user_data)
{
    Page3PartitionInfo *partition = (Page3PartitionInfo*)user_data;

    if (!partition || !g_page3_data || !g_page3_data->partition_manager) return;

    LOG_INFO("Configurando partición: %s", partition->device_path);

    // Mostrar diálogo de configuración de partición
    partition_manager_show_dialog(g_page3_data->partition_manager,
                                 partition->device_path,
                                 partition->filesystem,
                                 partition->mount_point,
                                 NULL);
}

// Función para actualizar el subtítulo de una fila específica
void page3_update_partition_row_subtitle(Page3Data *data, const gchar *device_path)
{
    if (!data || !device_path) return;

    // Buscar la fila correspondiente al device_path
    GList *row_item = data->partition_rows;
    GList *partition_item = data->partitions;

    while (row_item && partition_item) {
        AdwActionRow *row = ADW_ACTION_ROW(row_item->data);
        Page3PartitionInfo *partition = (Page3PartitionInfo*)partition_item->data;

        if (g_strcmp0(partition->device_path, device_path) == 0) {
            // Encontramos la fila, actualizar su subtítulo
            PartitionConfig *config = NULL;
            const char *current_disk = page3_get_selected_disk();
            if (data->partition_manager && current_disk &&
                page3_is_partition_of_disk(device_path, current_disk)) {
                config = partition_manager_find_config(data->partition_manager, device_path);
            }

            gchar *subtitle;
            if (config) {
                // Mostrar configuración programada
                GString *subtitle_str = g_string_new(partition->size_formatted);

                // Mostrar filesystem actual vs configurado
                if (config->format_needed || g_strcmp0(partition->filesystem, config->filesystem) != 0) {
                    g_string_append_printf(subtitle_str, " • %s → %s",
                                           partition->filesystem,
                                           config->filesystem);
                    if (config->format_needed) {
                        g_string_append(subtitle_str, " (formatear)");
                    }
                } else {
                    g_string_append_printf(subtitle_str, " • %s", partition->filesystem);
                }

                // Mostrar punto de montaje configurado
                if (config->mount_point) {
                    if (config->is_swap) {
                        g_string_append(subtitle_str, " • swap");
                    } else {
                        g_string_append_printf(subtitle_str, " • %s", config->mount_point);
                    }
                } else if (partition->mount_point) {
                    g_string_append_printf(subtitle_str, " • %s (actual)", partition->mount_point);
                }

                subtitle = g_string_free(subtitle_str, FALSE);
            } else {
                // Mostrar información actual sin configuración
                subtitle = g_strdup_printf("%s • %s%s%s",
                                           partition->size_formatted,
                                           partition->filesystem,
                                           partition->mount_point ? " • " : "",
                                           partition->mount_point ? partition->mount_point : "");
            }

            adw_action_row_set_subtitle(row, subtitle);
            g_free(subtitle);

            LOG_INFO("Subtítulo actualizado para partición: %s", device_path);
            break;
        }

        row_item = row_item->next;
        partition_item = partition_item->next;
    }
}

// Función para actualizar todos los subtítulos de las filas
void page3_update_all_partition_subtitles(Page3Data *data)
{
    if (!data) return;

    GList *row_item = data->partition_rows;
    GList *partition_item = data->partitions;

    while (row_item && partition_item) {
        AdwActionRow *row = ADW_ACTION_ROW(row_item->data);
        Page3PartitionInfo *partition = (Page3PartitionInfo*)partition_item->data;

        // Obtener configuración guardada para esta partición del disco actual
        PartitionConfig *config = NULL;
        const char *current_disk = page3_get_selected_disk();
        if (data->partition_manager && current_disk &&
            page3_is_partition_of_disk(partition->device_path, current_disk)) {
            config = partition_manager_find_config(data->partition_manager, partition->device_path);
        }

        gchar *subtitle;
        if (config) {
            // Mostrar configuración programada
            GString *subtitle_str = g_string_new(partition->size_formatted);

            // Mostrar filesystem actual vs configurado
            if (config->format_needed || g_strcmp0(partition->filesystem, config->filesystem) != 0) {
                g_string_append_printf(subtitle_str, " • %s → %s",
                                       partition->filesystem,
                                       config->filesystem);
                if (config->format_needed) {
                    g_string_append(subtitle_str, " (formatear)");
                }
            } else {
                g_string_append_printf(subtitle_str, " • %s", partition->filesystem);
            }

            // Mostrar punto de montaje configurado
            if (config->mount_point) {
                if (config->is_swap) {
                    g_string_append(subtitle_str, " • swap");
                } else {
                    g_string_append_printf(subtitle_str, " • %s", config->mount_point);
                }
            } else if (partition->mount_point) {
                g_string_append_printf(subtitle_str, " • %s (actual)", partition->mount_point);
            }

            subtitle = g_string_free(subtitle_str, FALSE);
        } else {
            // Mostrar información actual sin configuración
            subtitle = g_strdup_printf("%s • %s%s%s",
                                       partition->size_formatted,
                                       partition->filesystem,
                                       partition->mount_point ? " • " : "",
                                       partition->mount_point ? partition->mount_point : "");
        }

        adw_action_row_set_subtitle(row, subtitle);
        g_free(subtitle);

        row_item = row_item->next;
        partition_item = partition_item->next;
    }

    LOG_INFO("Subtítulos actualizados para todas las particiones");
}

// Callback para cuando se guarda la configuración de partición
void on_partition_config_saved(PartitionConfig *config, gpointer user_data)
{
    Page3Data *data = (Page3Data*)user_data;

    if (!data || !config) return;

    LOG_INFO("Configuración de partición guardada: %s -> %s",
             config->device_path,
             config->mount_point ? config->mount_point : "Sin punto de montaje");

    // Actualizar el subtítulo de la fila correspondiente solo si pertenece al disco actual
    const char *current_disk = page3_get_selected_disk();
    if (current_disk && page3_is_partition_of_disk(config->device_path, current_disk)) {
        page3_update_partition_row_subtitle(data, config->device_path);
    }

    // Si estamos en modo manual, actualizar sensibilidad del botón siguiente
    if (data->manual_partition_radio && gtk_check_button_get_active(data->manual_partition_radio)) {
        page3_update_next_button_sensitivity(data, TRUE);
    }
}




// Función para guardar el modo de particionado en variables.sh
void page3_save_partition_mode(const gchar *partition_mode)
{
    if (!partition_mode) return;

    LOG_INFO("Guardando PARTITION_MODE: %s", partition_mode);

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);

    // Leer el archivo existente para preservar otras variables
    GString *existing_content = g_string_new("");
    gchar *current_selected_disk = NULL;
    FILE *read_file = fopen(bash_file_path, "r");

    if (read_file) {
        char line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Preservar SELECTED_DISK para reescribirlo después
            if (g_str_has_prefix(line, "SELECTED_DISK=")) {
                gchar *disk_value = strchr(line, '=');
                if (disk_value) {
                    disk_value++; // Saltar el '='
                    disk_value = g_strstrip(disk_value);
                    // Remover comillas si existen
                    if (disk_value[0] == '"' && disk_value[strlen(disk_value)-1] == '"') {
                        disk_value[strlen(disk_value)-1] = '\0';
                        disk_value++;
                    }
                    current_selected_disk = g_strdup(disk_value);
                }
                continue;
            }
            // Skip la línea de PARTITION_MODE si existe
            if (g_str_has_prefix(line, "PARTITION_MODE=")) {
                continue;
            }
            // Skip líneas de fin de archivo
            if (g_str_has_prefix(line, "# Fin del archivo")) {
                continue;
            }
            g_string_append(existing_content, line);
        }
        fclose(read_file);
    }

    // Escribir el archivo actualizado
    FILE *file = fopen(bash_file_path, "w");
    if (!file) {
        LOG_ERROR("No se pudo crear el archivo %s", bash_file_path);
        g_free(bash_file_path);
        g_string_free(existing_content, TRUE);
        if (current_selected_disk) g_free(current_selected_disk);
        return;
    }

    // Si no había contenido previo, agregar header
    if (existing_content->len == 0) {
        fprintf(file, "#!/bin/bash\n");
        fprintf(file, "# Variables de configuración generadas por Arcris\n");
        fprintf(file, "# Archivo generado automáticamente - No editar manualmente\n\n");
    } else {
        // Escribir contenido existente
        fprintf(file, "%s", existing_content->str);
    }

    // Reescribir SELECTED_DISK si existía
    if (current_selected_disk) {
        fprintf(file, "SELECTED_DISK=\"%s\"\n", current_selected_disk);
        LOG_INFO("SELECTED_DISK preservado: %s", current_selected_disk);
        g_free(current_selected_disk);
    }

    // Agregar la variable del modo de particionado
    fprintf(file, "PARTITION_MODE=\"%s\"\n", partition_mode);

    fclose(file);
    g_string_free(existing_content, TRUE);

    LOG_INFO("Variable PARTITION_MODE guardada exitosamente: %s", partition_mode);
    g_free(bash_file_path);
}

// Función para actualizar sensibilidad del botón siguiente
void page3_update_next_button_sensitivity(Page3Data *data, gboolean is_manual_mode)
{
    if (!data) {
        LOG_ERROR("page3_update_next_button_sensitivity: data es NULL");
        return;
    }

    if (!data->revealer) {
        LOG_ERROR("page3_update_next_button_sensitivity: revealer es NULL");
        return;
    }

    LOG_INFO("Buscando botón siguiente en revealer para modo: %s", is_manual_mode ? "manual/cifrado" : "automático");

    // Buscar el botón siguiente en el revealer
    GtkWidget *next_button = NULL;

    // El botón siguiente debería estar en el revealer
    GtkWidget *revealer_child = gtk_revealer_get_child(data->revealer);
    if (revealer_child) {
        LOG_INFO("Revealer tiene hijo, buscando botón siguiente recursivamente...");
        // Buscar recursivamente el botón siguiente
        next_button = page3_find_next_button_recursive(revealer_child);
    } else {
        LOG_WARNING("El revealer no tiene hijo");
    }

    if (next_button) {
        LOG_INFO("Botón siguiente encontrado exitosamente");
        // Verificar si estamos en modo cifrado
        gboolean is_encryption_mode = data->cifrado_partition_button &&
                                     gtk_check_button_get_active(data->cifrado_partition_button);

        if (is_manual_mode) {
            // En modo manual, desactivar el botón hasta que se configure al menos una partición
            gboolean has_root_partition = FALSE;
            if (data->partition_manager) {
                has_root_partition = partition_manager_has_root_partition(data->partition_manager);
            }
            gtk_widget_set_sensitive(next_button, has_root_partition);
            LOG_INFO("Botón siguiente %s (modo manual, partición root: %s)",
                     has_root_partition ? "activado" : "desactivado",
                     has_root_partition ? "sí" : "no");
        } else if (is_encryption_mode) {
            // En modo cifrado, desactivar el botón hasta que las contraseñas sean válidas
            gboolean passwords_valid = data->passwords_match && data->password_length_valid;
            gtk_widget_set_sensitive(next_button, passwords_valid);
            LOG_INFO("Botón siguiente %s (modo cifrado, contraseñas válidas: %s)",
                     passwords_valid ? "activado" : "desactivado",
                     passwords_valid ? "sí" : "no");
        } else {
            // En modo automático, activar el botón
            gtk_widget_set_sensitive(next_button, TRUE);
            LOG_INFO("Botón siguiente activado (modo automático)");
        }
    } else {
        LOG_WARNING("No se pudo encontrar el botón siguiente en el revealer");

        // Como alternativa, intentar buscar en el carousel
        if (data->carousel) {
            GtkWidget *carousel_parent = gtk_widget_get_parent(GTK_WIDGET(data->carousel));
            if (carousel_parent) {
                LOG_INFO("Buscando botón siguiente en el padre del carousel como alternativa...");
                next_button = page3_find_next_button_recursive(carousel_parent);
                if (next_button) {
                    LOG_INFO("Botón siguiente encontrado en el padre del carousel");
                    gtk_widget_set_sensitive(next_button, !is_manual_mode);
                }
            }
        }
    }
}

// Función auxiliar para buscar el botón siguiente recursivamente
GtkWidget* page3_find_next_button_recursive(GtkWidget *widget)
{
    if (!widget) return NULL;

    // Si es un botón, verificar si es el botón siguiente
    if (GTK_IS_BUTTON(widget)) {
        const gchar *label = gtk_button_get_label(GTK_BUTTON(widget));
        const gchar *widget_name = gtk_widget_get_name(widget);

        LOG_INFO("Botón encontrado - Label: '%s', Name: '%s'",
                 label ? label : "NULL",
                 widget_name ? widget_name : "NULL");

        if (label && (g_str_has_suffix(label, "Siguiente") || g_str_has_suffix(label, "Next") ||
                      g_strrstr(label, "siguiente") || g_strrstr(label, "next"))) {
            LOG_INFO("¡Botón siguiente encontrado! Label: '%s'", label);
            return widget;
        }

        // También verificar por nombre del widget
        if (widget_name && (g_strrstr(widget_name, "next") || g_strrstr(widget_name, "siguiente"))) {
            LOG_INFO("¡Botón siguiente encontrado por nombre! Name: '%s'", widget_name);
            return widget;
        }
    }

    // Buscar en los hijos
    GtkWidget *child = gtk_widget_get_first_child(widget);
    while (child) {
        GtkWidget *result = page3_find_next_button_recursive(child);
        if (result) return result;
        child = gtk_widget_get_next_sibling(child);
    }

    return NULL;
}

// Función para cargar el modo de particionado desde variables.sh
void page3_load_partition_mode(Page3Data *data)
{
    if (!data) {
        LOG_ERROR("page3_load_partition_mode: data es NULL");
        return;
    }

    LOG_INFO("=== page3_load_partition_mode INICIADO ===");

    // Leer el archivo variables.sh
    gchar *variables_content = NULL;
    GError *error = NULL;

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);
    LOG_INFO("Intentando cargar archivo: %s", bash_file_path);

    if (g_file_get_contents(bash_file_path, &variables_content, NULL, &error)) {
        LOG_INFO("Archivo variables.sh leído exitosamente");
        LOG_INFO("Contenido del archivo:\n%s", variables_content);

        // Buscar la línea PARTITION_MODE
        gchar **lines = g_strsplit(variables_content, "\n", -1);
        gboolean partition_mode_found = FALSE;

        LOG_INFO("Buscando línea PARTITION_MODE en %d líneas...", g_strv_length(lines));

        for (int i = 0; lines[i]; i++) {
            gchar *line = g_strstrip(lines[i]);
            LOG_INFO("Línea %d: '%s'", i, line);

            if (g_str_has_prefix(line, "PARTITION_MODE=")) {
                LOG_INFO("¡Línea PARTITION_MODE encontrada!");
                partition_mode_found = TRUE;

                gchar *mode_value = strchr(line, '=');
                if (mode_value) {
                    mode_value++; // Saltar el '='
                    LOG_INFO("Valor inicial después del '=': '%s'", mode_value);

                    // Remover comillas si existen
                    mode_value = g_strstrip(mode_value);
                    LOG_INFO("Valor después de strstrip: '%s'", mode_value);

                    if (mode_value[0] == '"' && mode_value[strlen(mode_value)-1] == '"') {
                        mode_value[strlen(mode_value)-1] = '\0';
                        mode_value++;
                        LOG_INFO("Valor después de remover comillas: '%s'", mode_value);
                    }

                    // Establecer el radio button correspondiente
                    if (g_strcmp0(mode_value, "manual") == 0) {
                        LOG_INFO("Estableciendo modo manual");
                        gtk_check_button_set_active(data->manual_partition_radio, TRUE);
                        LOG_INFO("Radio button manual activado");
                    } else if (g_strcmp0(mode_value, "auto_btrfs") == 0) {
                        LOG_INFO("Estableciendo modo auto_btrfs");
                        gtk_check_button_set_active(data->auto_btrfs_radio, TRUE);
                        LOG_INFO("Radio button auto_btrfs activado");
                    } else {
                        LOG_INFO("Estableciendo modo auto (valor: '%s')", mode_value);
                        gtk_check_button_set_active(data->auto_partition_radio, TRUE);
                        LOG_INFO("Radio button auto activado");
                    }

                    // Actualizar sensibilidad del botón siguiente
                    gboolean is_manual = g_strcmp0(mode_value, "manual") == 0;
                    LOG_INFO("Actualizando sensibilidad del botón siguiente (manual: %s)", is_manual ? "TRUE" : "FALSE");
                    page3_update_next_button_sensitivity(data, is_manual);

                    break;
                } else {
                    LOG_WARNING("No se encontró '=' en la línea PARTITION_MODE");
                }
            }
        }

        if (!partition_mode_found) {
            LOG_WARNING("No se encontró la línea PARTITION_MODE en el archivo");
        }

        g_strfreev(lines);
        g_free(variables_content);
    } else {
        LOG_WARNING("No se pudo cargar variables.sh: %s", error ? error->message : "Error desconocido");
        if (error) g_error_free(error);

        // Establecer modo por defecto
        LOG_INFO("Estableciendo modo por defecto: auto");
        gtk_check_button_set_active(data->auto_partition_radio, TRUE);
        LOG_INFO("Radio button auto activado por defecto");
    }

    g_free(bash_file_path);
    LOG_INFO("=== page3_load_partition_mode FINALIZADO ===");
}

// Funciones adicionales para manejo de particiones
void page3_init_partition_manager(Page3Data *data)
{
    if (!data) return;

    // Crear manejador de particiones
    data->partition_manager = partition_manager_new();
    if (!data->partition_manager) {
        LOG_ERROR("No se pudo crear el PartitionManager");
        return;
    }

    // Cargar diálogo de partición
    GtkBuilder *partition_builder = gtk_builder_new_from_resource("/org/gtk/arcris/window_partition.ui");
    if (!partition_builder) {
        LOG_ERROR("No se pudo cargar window_partition.ui");
        partition_manager_free(data->partition_manager);
        data->partition_manager = NULL;
        return;
    }

    // Inicializar widgets del manejador de particiones
    if (!partition_manager_init(data->partition_manager, partition_builder)) {
        LOG_ERROR("No se pudieron inicializar los widgets del PartitionManager");
        partition_manager_free(data->partition_manager);
        data->partition_manager = NULL;
        g_object_unref(partition_builder);
        return;
    }

    // Configurar callback para cuando se guarda la configuración
    partition_manager_set_save_callback(data->partition_manager,
                                       on_partition_config_saved,
                                       data);

    g_object_unref(partition_builder);

    LOG_INFO("PartitionManager inicializado correctamente para page3");
}

// ============================================================================
// FUNCIONES PARA MANEJO DE PARTICIONADO CIFRADO
// ============================================================================

// Función para navegar a la página de clave de cifrado
void page3_navigate_to_encryption_key(Page3Data *data)
{
    if (!data || !data->navigation_view) return;

    AdwNavigationPage *encryption_page = adw_navigation_view_find_page(data->navigation_view, "encryption_key");
    if (encryption_page) {
        adw_navigation_view_push(data->navigation_view, encryption_page);
        LOG_INFO("Navegando a página de clave de cifrado");
    } else {
        LOG_ERROR("No se pudo encontrar la página de clave de cifrado");
    }
}

// Función para navegar de regreso desde la página de cifrado
void page3_navigate_back_from_encryption(Page3Data *data)
{
    if (!data || !data->navigation_view) return;

    adw_navigation_view_pop(data->navigation_view);
    LOG_INFO("Regresando desde página de clave de cifrado");
}

// Función para verificar coincidencia de contraseñas
void page3_check_password_match(Page3Data *data)
{
    if (!data || !data->password_entry || !data->password_confirm_entry) return;

    const gchar *password = gtk_editable_get_text(GTK_EDITABLE(data->password_entry));
    const gchar *confirm_password = gtk_editable_get_text(GTK_EDITABLE(data->password_confirm_entry));

    // Verificar si las contraseñas coinciden (solo si ambas tienen contenido)
    data->passwords_match = (password && confirm_password &&
                            strlen(password) > 0 &&
                            strlen(confirm_password) > 0 &&
                            strcmp(password, confirm_password) == 0);



    // Mostrar/ocultar mensaje de error y aplicar estilos
    if (data->password_error_label) {
        if (confirm_password && strlen(confirm_password) > 0) {
            if (!data->passwords_match) {
                // Mostrar error cuando no coinciden
                gtk_widget_set_visible(GTK_WIDGET(data->password_error_label), TRUE);
                gtk_widget_add_css_class(GTK_WIDGET(data->password_confirm_entry), "error");
                gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "success");
                // Remover success del campo de contraseña principal también
                gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "success");
            } else {
                // Ocultar error cuando coinciden y agregar success a ambos campos
                gtk_widget_set_visible(GTK_WIDGET(data->password_error_label), FALSE);
                gtk_widget_add_css_class(GTK_WIDGET(data->password_confirm_entry), "success");
                gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "error");
                // Agregar success al campo de contraseña principal también
                gtk_widget_add_css_class(GTK_WIDGET(data->password_entry), "success");
                gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "error");
            }
        } else {
            // Campo vacío - remover todas las clases y marcar como inválido
            data->passwords_match = FALSE;
            gtk_widget_set_visible(GTK_WIDGET(data->password_error_label), FALSE);
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "error");
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_confirm_entry), "success");
            // Remover clases del campo de contraseña principal también
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "success");
        }
    }

    // Actualizar estado del botón y revealer
    page3_update_encryption_button_state(data);

    // Verificar si ambos campos están en success y activar
    page3_check_success_and_activate(data);

    // Actualizar variables.sh si estamos en modo cifrado y las contraseñas son válidas
    gboolean encryption_mode = data->cifrado_partition_button && gtk_check_button_get_active(data->cifrado_partition_button);

    if (encryption_mode && data->passwords_match && data->password_length_valid) {
        LOG_INFO("Guardando contraseña de cifrado en variables.sh");
        page3_update_encryption_variables(data);
    }
}

// Función para validar longitud de contraseña
void page3_validate_password_length(Page3Data *data)
{
    if (!data || !data->password_entry) return;

    const gchar *password = gtk_editable_get_text(GTK_EDITABLE(data->password_entry));

    LOG_INFO("=== DEBUG page3_validate_password_length ===");
    LOG_INFO("password length: %zu", password ? strlen(password) : 0);

    if (strlen(password) > 0) {
        // Validar longitud mínima (8 caracteres)
        data->password_length_valid = (strlen(password) >= 8);
        LOG_INFO("password >= 8 chars: %s", data->password_length_valid ? "TRUE" : "FALSE");


        if (!data->password_length_valid) {
            gtk_widget_add_css_class(GTK_WIDGET(data->password_entry), "error");
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "success");
        } else {
            gtk_widget_add_css_class(GTK_WIDGET(data->password_entry), "success");
            gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "error");
        }
    } else {
        // Campo vacío = inválido
        data->password_length_valid = FALSE;
        LOG_INFO("password field is empty, setting password_length_valid = FALSE");
        gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "error");
        gtk_widget_remove_css_class(GTK_WIDGET(data->password_entry), "success");
    }

    LOG_INFO("page3_validate_password_length resultado final: %s", data->password_length_valid ? "TRUE" : "FALSE");

    // Actualizar estado del botón y revealer
    page3_update_encryption_button_state(data);

    // Verificar si ambos campos están en success y activar
    page3_check_success_and_activate(data);
}

// Función para actualizar el estado del botón de cifrado
void page3_update_encryption_button_state(Page3Data *data)
{
    if (!data || !data->save_key_disk_button) return;

    // Habilitar botón solo si el cifrado está seleccionado
    gboolean encryption_selected = data->cifrado_partition_button &&
                                  gtk_check_button_get_active(data->cifrado_partition_button);

    gtk_widget_set_sensitive(GTK_WIDGET(data->save_key_disk_button), encryption_selected);
}

// Función simple: verificar si ambos campos están en success y activar botón
void page3_check_success_and_activate(Page3Data *data)
{
    if (!data || !data->password_entry || !data->password_confirm_entry) return;

    // Verificar si estamos en modo cifrado
    gboolean is_encryption_mode = data->cifrado_partition_button &&
                                 gtk_check_button_get_active(data->cifrado_partition_button);

    if (!is_encryption_mode) return;

    // Verificar si ambos campos tienen la clase "success"
    gboolean password_has_success = gtk_widget_has_css_class(GTK_WIDGET(data->password_entry), "success");
    gboolean confirm_has_success = gtk_widget_has_css_class(GTK_WIDGET(data->password_confirm_entry), "success");

    LOG_INFO("=== VERIFICANDO CAMPOS SUCCESS ===");
    LOG_INFO("password_entry tiene success: %s", password_has_success ? "TRUE" : "FALSE");
    LOG_INFO("password_confirm_entry tiene success: %s", confirm_has_success ? "TRUE" : "FALSE");

    if (password_has_success && confirm_has_success) {
        LOG_INFO("*** AMBOS CAMPOS EN SUCCESS - ACTIVANDO SISTEMA ***");

        // Buscar el botón siguiente
        GtkWidget *next_button = NULL;
        if (data->revealer) {
            GtkWidget *revealer_child = gtk_revealer_get_child(data->revealer);
            if (revealer_child) {
                next_button = page3_find_next_button_recursive(revealer_child);
            }
        }

        if (next_button) {
            // Activar el botón siguiente
            gtk_widget_set_sensitive(next_button, TRUE);
            LOG_INFO("*** BOTÓN SIGUIENTE ACTIVADO ***");

            // Guardar la contraseña
            LOG_INFO("*** GUARDANDO CONTRASEÑA EN VARIABLES.SH ***");
            page3_update_encryption_variables(data);
            LOG_INFO("*** GUARDADO COMPLETADO ***");
        } else {
            LOG_WARNING("No se pudo encontrar el botón siguiente");
        }
    } else {
        LOG_INFO("Campos no están en success, no activando");

        // Desactivar el botón si no están en success
        GtkWidget *next_button = NULL;
        if (data->revealer) {
            GtkWidget *revealer_child = gtk_revealer_get_child(data->revealer);
            if (revealer_child) {
                next_button = page3_find_next_button_recursive(revealer_child);
            }
        }

        if (next_button) {
            gtk_widget_set_sensitive(next_button, FALSE);
            LOG_INFO("Botón siguiente desactivado");
        }
    }
}

// Función para actualizar las variables de cifrado en variables.sh
void page3_update_encryption_variables(Page3Data *data)
{
    if (!data) return;

    LOG_INFO("=== DEBUG page3_update_encryption_variables ===");

    const gchar *password = gtk_editable_get_text(GTK_EDITABLE(data->password_entry));
    LOG_INFO("Password obtenida: %s", password ? "SÍ" : "NULL");
    LOG_INFO("Password length: %zu", password ? strlen(password) : 0);

    if (!password || strlen(password) == 0) {
        LOG_ERROR("Password está vacía, no se puede guardar");
        return;
    }

    LOG_INFO("*** INICIANDO GUARDADO DE CONTRASEÑA ***");
    LOG_INFO("Actualizando contraseña de cifrado en variables.sh");

    // Actualizar archivo variables.sh con la contraseña
    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);

    // Leer archivo existente
    GString *existing_content = g_string_new("");
    FILE *read_file = fopen(bash_file_path, "r");
    if (read_file) {
        gchar line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Reemplazar líneas de cifrado con valores actualizados
            if (strncmp(line, "ENCRYPTION_ENABLED=", 19) == 0) {
                g_string_append(existing_content, "ENCRYPTION_ENABLED=\"true\"\n");
            } else if (strncmp(line, "ENCRYPTION_PASSWORD=", 20) == 0) {
                g_string_append_printf(existing_content, "ENCRYPTION_PASSWORD=\"%s\"\n", password);
            } else {
                g_string_append(existing_content, line);
            }
        }
        fclose(read_file);
    }

    // Escribir archivo actualizado
    FILE *file = fopen(bash_file_path, "w");
    if (file) {
        fprintf(file, "%s", existing_content->str);
        fclose(file);
        LOG_INFO("*** CONTRASEÑA GUARDADA EXITOSAMENTE EN VARIABLES.SH ***");

        // Verificar que se guardó correctamente
        if (strstr(existing_content->str, "ENCRYPTION_PASSWORD=") != NULL) {
            LOG_INFO("Verificación: ENCRYPTION_PASSWORD encontrada en el contenido");
        } else {
            LOG_WARNING("Verificación: ENCRYPTION_PASSWORD NO encontrada en el contenido");
        }
    } else {
        LOG_ERROR("*** ERROR: No se pudo abrir variables.sh para escritura ***");
        LOG_ERROR("Ruta del archivo: %s", bash_file_path);
    }

    g_free(bash_file_path);
    g_string_free(existing_content, TRUE);
}

// Función para obtener la contraseña de cifrado
const gchar* page3_get_encryption_password(Page3Data *data)
{
    if (!data || !data->password_entry) return NULL;
    return gtk_editable_get_text(GTK_EDITABLE(data->password_entry));
}

// Función para guardar la configuración de cifrado
void page3_save_encryption_config(Page3Data *data)
{
    if (!data) return;

    const gchar *password = page3_get_encryption_password(data);
    if (!password) return;

    LOG_INFO("Guardando configuración de cifrado");

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);

    // Leer el archivo existente para preservar otras variables
    GString *existing_content = g_string_new("");
    gchar *current_selected_disk = NULL;
    gchar *current_partition_mode = NULL;
    gchar *current_keyboard_layout = NULL;
    gchar *current_keymap_tty = NULL;
    gchar *current_timezone = NULL;
    gchar *current_locale = NULL;

    FILE *read_file = fopen(bash_file_path, "r");
    if (read_file) {
        gchar line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Preservar variables existentes (excepto ENCRYPTION_*)
            if (strncmp(line, "SELECTED_DISK=", 14) == 0) {
                gchar *disk_value = strchr(line, '=');
                if (disk_value) {
                    disk_value++; // Saltar el '='
                    disk_value = g_strstrip(disk_value);
                    // Remover comillas si existen
                    if (disk_value[0] == '"' && disk_value[strlen(disk_value)-1] == '"') {
                        disk_value[strlen(disk_value)-1] = '\0';
                        disk_value++;
                    }
                    current_selected_disk = g_strdup(disk_value);
                }
            } else if (strncmp(line, "PARTITION_MODE=", 15) == 0) {
                gchar *mode_value = strchr(line, '=');
                if (mode_value) {
                    mode_value++; // Saltar el '='
                    mode_value = g_strstrip(mode_value);
                    // Remover comillas si existen
                    if (mode_value[0] == '"' && mode_value[strlen(mode_value)-1] == '"') {
                        mode_value[strlen(mode_value)-1] = '\0';
                        mode_value++;
                    }
                    current_partition_mode = g_strdup(mode_value);
                }
            } else if (strncmp(line, "KEYBOARD_LAYOUT=", 16) == 0) {
                gchar *layout_value = strchr(line, '=');
                if (layout_value) {
                    layout_value++; // Saltar el '='
                    layout_value = g_strstrip(layout_value);
                    // Remover comillas si existen
                    if (layout_value[0] == '"' && layout_value[strlen(layout_value)-1] == '"') {
                        layout_value[strlen(layout_value)-1] = '\0';
                        layout_value++;
                    }
                    current_keyboard_layout = g_strdup(layout_value);
                }
            } else if (strncmp(line, "KEYMAP_TTY=", 11) == 0) {
                gchar *keymap_value = strchr(line, '=');
                if (keymap_value) {
                    keymap_value++; // Saltar el '='
                    keymap_value = g_strstrip(keymap_value);
                    // Remover comillas si existen
                    if (keymap_value[0] == '"' && keymap_value[strlen(keymap_value)-1] == '"') {
                        keymap_value[strlen(keymap_value)-1] = '\0';
                        keymap_value++;
                    }
                    current_keymap_tty = g_strdup(keymap_value);
                }
            } else if (strncmp(line, "TIMEZONE=", 9) == 0) {
                gchar *timezone_value = strchr(line, '=');
                if (timezone_value) {
                    timezone_value++; // Saltar el '='
                    timezone_value = g_strstrip(timezone_value);
                    // Remover comillas si existen
                    if (timezone_value[0] == '"' && timezone_value[strlen(timezone_value)-1] == '"') {
                        timezone_value[strlen(timezone_value)-1] = '\0';
                        timezone_value++;
                    }
                    current_timezone = g_strdup(timezone_value);
                }
            } else if (strncmp(line, "LOCALE=", 7) == 0) {
                gchar *locale_value = strchr(line, '=');
                if (locale_value) {
                    locale_value++; // Saltar el '='
                    locale_value = g_strstrip(locale_value);
                    // Remover comillas si existen
                    if (locale_value[0] == '"' && locale_value[strlen(locale_value)-1] == '"') {
                        locale_value[strlen(locale_value)-1] = '\0';
                        locale_value++;
                    }
                    current_locale = g_strdup(locale_value);
                }
            }
        }
        fclose(read_file);
    }

    // Escribir el archivo completo
    FILE *file = fopen(bash_file_path, "w");
    if (file) {
        fprintf(file, "#!/bin/bash\n");
        fprintf(file, "# Variables de configuración generadas por Arcris\n");
        fprintf(file, "# Archivo generado automáticamente - No editar manualmente\n\n");

        // Escribir variables preservadas
        if (current_keyboard_layout) {
            fprintf(file, "KEYBOARD_LAYOUT=\"%s\"\n", current_keyboard_layout);
        } else {
            fprintf(file, "KEYBOARD_LAYOUT=\"es\"\n");
        }

        if (current_keymap_tty) {
            fprintf(file, "KEYMAP_TTY=\"%s\"\n", current_keymap_tty);
        } else {
            fprintf(file, "KEYMAP_TTY=\"es\"\n");
        }

        if (current_timezone) {
            fprintf(file, "TIMEZONE=\"%s\"\n", current_timezone);
        } else {
            fprintf(file, "TIMEZONE=\"America/Lima\"\n");
        }

        if (current_locale) {
            fprintf(file, "LOCALE=\"%s\"\n", current_locale);
        } else {
            fprintf(file, "LOCALE=\"es_PE.UTF-8\"\n");
        }

        if (current_selected_disk) {
            fprintf(file, "SELECTED_DISK=\"%s\"\n", current_selected_disk);
        } else {
            fprintf(file, "SELECTED_DISK=\"/dev/sda\"\n");
        }

        if (current_partition_mode) {
            fprintf(file, "PARTITION_MODE=\"%s\"\n", current_partition_mode);
        } else {
            fprintf(file, "PARTITION_MODE=\"auto\"\n");
        }

        // Escribir variables de cifrado
        fprintf(file, "ENCRYPTION_ENABLED=\"true\"\n");
        fprintf(file, "ENCRYPTION_PASSWORD=\"%s\"\n", password);

        fclose(file);
        LOG_INFO("Configuración de cifrado guardada exitosamente");
    } else {
        LOG_ERROR("No se pudo escribir el archivo de configuración de cifrado");
    }

    // Limpiar memoria
    g_free(current_selected_disk);
    g_free(current_partition_mode);
    g_free(current_keyboard_layout);
    g_free(current_keymap_tty);
    g_free(current_timezone);
    g_free(current_locale);
    g_free(bash_file_path);
    g_string_free(existing_content, TRUE);
}

// Función para crear variables de cifrado iniciales
void page3_create_encryption_variables(void)
{
    LOG_INFO("Creando variables de cifrado iniciales");

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);

    // Leer el archivo existente
    GString *existing_content = g_string_new("");
    FILE *read_file = fopen(bash_file_path, "r");
    if (read_file) {
        gchar line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Evitar duplicar líneas de cifrado
            if (strncmp(line, "ENCRYPTION_ENABLED=", 19) != 0 &&
                strncmp(line, "ENCRYPTION_PASSWORD=", 20) != 0) {
                g_string_append(existing_content, line);
            }
        }
        fclose(read_file);
    }

    // Escribir el archivo con las variables de cifrado
    FILE *file = fopen(bash_file_path, "w");
    if (file) {
        fprintf(file, "%s", existing_content->str);
        fprintf(file, "ENCRYPTION_ENABLED=\"true\"\n");
        fprintf(file, "ENCRYPTION_PASSWORD=\"\"\n");
        fclose(file);
        LOG_INFO("Variables de cifrado creadas exitosamente");
    } else {
        LOG_ERROR("No se pudo crear las variables de cifrado");
    }

    g_free(bash_file_path);
    g_string_free(existing_content, TRUE);
}

// ============================================================================
// CALLBACKS PARA CAMPOS DE CONTRASEÑA
// ============================================================================

// Callback para cambios en el campo de contraseña
void on_page3_password_changed(AdwPasswordEntryRow *entry, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data) return;

    // Verificar longitud de contraseña
    page3_validate_password_length(data);

    // Verificar coincidencia cuando cambia la contraseña principal
    page3_check_password_match(data);

    // Verificar si ambos campos están en success y activar
    page3_check_success_and_activate(data);
}

// Callback para cambios en el campo de confirmación de contraseña
void on_page3_password_confirm_changed(AdwPasswordEntryRow *entry, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data) return;

    // Verificar coincidencia de contraseñas
    page3_check_password_match(data);

    // Verificar si ambos campos están en success y activar
    page3_check_success_and_activate(data);
}

// Callback para el botón de guardar clave de disco
void on_page3_save_key_disk_clicked(GtkButton *button, gpointer user_data)
{
    Page3Data *data = (Page3Data *)user_data;
    if (!data) return;

    LOG_INFO("Configurando clave de cifrado de disco");

    // Limpiar los campos de contraseña antes de navegar
    if (data->password_entry) {
        gtk_editable_set_text(GTK_EDITABLE(data->password_entry), "");
        LOG_INFO("Campo de contraseña limpiado");
    }

    if (data->password_confirm_entry) {
        gtk_editable_set_text(GTK_EDITABLE(data->password_confirm_entry), "");
        LOG_INFO("Campo de confirmación de contraseña limpiado");
    }

    // Crear variables de cifrado iniciales en variables.sh
    page3_create_encryption_variables();

    // Navegar a la página de configuración de clave
    page3_navigate_to_encryption_key(data);
}

// ============================================================================
// FUNCIONES PARA DETECTAR INFORMACIÓN DEL DISCO
// ============================================================================

// Función para obtener el tipo de tabla de particiones (GPT/MBR)
gchar* page3_get_partition_table_type(const gchar *disk_path)
{
    if (!disk_path) return g_strdup("Desconocido");

    // Usar lsblk para obtener tipo de tabla de particiones
    gchar *command = g_strdup_printf("/bin/sh -c \"lsblk -o PTTYPE %s | tail -1\"", disk_path);
    LOG_INFO("=== DEBUG: Ejecutando comando: %s ===", command);

    gchar *output = NULL;
    gchar *error_output = NULL;
    GError *error = NULL;
    gint exit_status = 0;

    // Usar g_spawn_command_line_sync con captura de stderr
    if (g_spawn_command_line_sync(command, &output, &error_output, &exit_status, &error)) {
        LOG_INFO("=== DEBUG: Comando ejecutado exitosamente ===");
        LOG_INFO("=== DEBUG: Exit status: %d ===", exit_status);
        LOG_INFO("=== DEBUG: Output crudo: '%s' ===", output ? output : "NULL");
        LOG_INFO("=== DEBUG: Error output: '%s' ===", error_output ? error_output : "NULL");

        if (exit_status == 0 && output) {
            g_strstrip(output);
            LOG_INFO("=== DEBUG: Output después de strip: '%s' ===", output);
            LOG_INFO("=== DEBUG: Longitud del output: %zu ===", strlen(output));

            if (strlen(output) > 0 && !g_str_equal(output, "")) {
                LOG_INFO("=== DEBUG: Comparando output con tipos conocidos ===");

                // Convertir la salida de lsblk a formato estándar
                if (g_strcmp0(output, "gpt") == 0 || g_strcmp0(output, "GPT") == 0) {
                    LOG_INFO("=== DEBUG: MATCH ENCONTRADO: '%s' -> Devolviendo GPT ===", output);
                    g_free(output);
                    g_free(error_output);
                    g_free(command);
                    return g_strdup("GPT");
                } else if (g_strcmp0(output, "dos") == 0 || g_strcmp0(output, "mbr") == 0 || g_strcmp0(output, "MBR") == 0) {
                    LOG_INFO("=== DEBUG: MATCH ENCONTRADO: '%s' -> Devolviendo MBR ===", output);
                    g_free(output);
                    g_free(error_output);
                    g_free(command);
                    return g_strdup("MBR");
                } else if (strlen(output) > 0) {
                    // Para otros tipos específicos, mapear apropiadamente
                    LOG_INFO("=== DEBUG: Tipo desconocido '%s' -> Devolviendo MBR por defecto ===", output);
                    g_free(output);
                    g_free(error_output);
                    g_free(command);
                    return g_strdup("Sin Etiqueta");
                }
            } else {
                LOG_WARNING("=== DEBUG: Output vacío o nulo ===");
            }
        } else {
            LOG_WARNING("lsblk falló con código de salida: %d", exit_status);
            if (error_output && strlen(error_output) > 0) {
                LOG_WARNING("Error de lsblk: %s", error_output);
            }
        }

        g_free(output);
        g_free(error_output);
    } else {
        LOG_ERROR("=== DEBUG: Error al ejecutar comando lsblk ===");
    }

    if (error) {
        LOG_WARNING("Error ejecutando lsblk: %s", error->message);
        g_error_free(error);
    }

    g_free(command);

    // Como fallback, intentar detectar con otro método
    // Método alternativo usando file
    command = g_strdup_printf("file -s %s", disk_path);
    output = NULL;
    error = NULL;

    if (g_spawn_command_line_sync(command, &output, NULL, NULL, &error)) {
        if (output) {
            g_strstrip(output);
            if (strstr(output, "GPT") != NULL || strstr(output, "GUID") != NULL) {
                g_free(output);
                g_free(command);
                return g_strdup("GPT");
            } else if (strstr(output, "DOS") != NULL || strstr(output, "MBR") != NULL) {
                g_free(output);
                g_free(command);
                return g_strdup("MBR");
            }
            g_free(output);
        }
    }

    if (error) {
        g_error_free(error);
    }

    g_free(command);

    // Si todo falla, asumir MBR como predeterminado
    return g_strdup("MBR");
}

// Función para obtener el tipo de firmware (UEFI/BIOS Legacy)
gchar* page3_get_firmware_type(void)
{
    // Verificar si existe el directorio /sys/firmware/efi
    if (g_file_test("/sys/firmware/efi", G_FILE_TEST_IS_DIR)) {
        return g_strdup("UEFI");
    }

    // Método alternativo: verificar si existe /sys/firmware/efi/efivars
    if (g_file_test("/sys/firmware/efi/efivars", G_FILE_TEST_IS_DIR)) {
        return g_strdup("UEFI");
    }

    // Método alternativo: usar efibootmgr para verificar UEFI
    gchar *output = NULL;
    GError *error = NULL;

    if (g_spawn_command_line_sync("efibootmgr -v", &output, NULL, NULL, &error)) {
        if (output && strlen(output) > 0) {
            g_free(output);
            return g_strdup("UEFI");
        }
        g_free(output);
    }

    if (error) {
        g_error_free(error);
    }

    // Si no se puede determinar que es UEFI, asumir BIOS Legacy
    return g_strdup("BIOS Legacy");
}
