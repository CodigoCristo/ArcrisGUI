#include "partition_manager.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Variable global para el manager (para uso en callbacks)
static PartitionManager *g_partition_manager = NULL;

// Crear nuevo PartitionManager
PartitionManager*
partition_manager_new(void)
{
    PartitionManager *manager = g_malloc0(sizeof(PartitionManager));

    // Inicializar variables
    manager->partition_dialog = NULL;
    manager->current_config = NULL;
    manager->partition_configs = NULL;
    manager->on_config_saved = NULL;
    manager->callback_data = NULL;

    return manager;
}

// Liberar memoria del PartitionManager
void
partition_manager_free(PartitionManager *manager)
{
    if (!manager) return;

    // Limpiar objetos GObject
    g_clear_object(&manager->partition_dialog);
    g_clear_object(&manager->mount_point_list);
    g_clear_object(&manager->format_list);

    // Liberar configuración actual
    if (manager->current_config) {
        partition_manager_free_config(manager->current_config);
    }

    // Limpiar lista de configuraciones
    partition_manager_clear_configs(manager);

    // Liberar la estructura
    g_free(manager);

    LOG_INFO("PartitionManager liberado correctamente");
}

// Inicializar PartitionManager
gboolean
partition_manager_init(PartitionManager *manager, GtkBuilder *builder)
{
    if (!manager || !builder) {
        LOG_ERROR("Parámetros inválidos para inicializar PartitionManager");
        return FALSE;
    }

    // Obtener widgets del builder
    manager->partition_dialog = ADW_WINDOW(gtk_builder_get_object(builder, "page5"));
    manager->swap_switch = GTK_SWITCH(gtk_builder_get_object(builder, "swap_switch"));
    manager->mount_point_combo = ADW_COMBO_ROW(gtk_builder_get_object(builder, "mount_point_combo"));
    manager->format_combo = ADW_COMBO_ROW(gtk_builder_get_object(builder, "format_combo"));
    manager->cancel_button = GTK_BUTTON(gtk_builder_get_object(builder, "cancel_button"));
    manager->save_button = GTK_BUTTON(gtk_builder_get_object(builder, "next_button"));
    manager->window_title = ADW_WINDOW_TITLE(gtk_builder_get_object(builder, "window_title"));

    // Verificar que se obtuvieron los widgets
    if (!manager->partition_dialog || !manager->swap_switch ||
        !manager->mount_point_combo || !manager->format_combo ||
        !manager->cancel_button || !manager->save_button) {
        LOG_ERROR("No se pudieron obtener todos los widgets necesarios del builder");
        return FALSE;
    }

    // Obtener modelos de datos
    manager->mount_point_list = GTK_STRING_LIST(gtk_builder_get_object(builder, "mount_point_list"));
    manager->format_list = GTK_STRING_LIST(gtk_builder_get_object(builder, "format_list"));

    // Configurar widgets
    if (!partition_manager_setup_widgets(manager)) {
        LOG_ERROR("No se pudieron configurar los widgets del PartitionManager");
        return FALSE;
    }

    // Cargar configuración guardada
    partition_manager_load_from_variables(manager);

    // Guardar referencia global
    g_partition_manager = manager;

    LOG_INFO("PartitionManager inicializado correctamente");
    return TRUE;
}

// Configurar widgets del PartitionManager
gboolean
partition_manager_setup_widgets(PartitionManager *manager)
{
    if (!manager) return FALSE;

    // Conectar señales
    g_signal_connect(manager->cancel_button, "clicked",
                     G_CALLBACK(on_partition_dialog_cancel_clicked), manager);
    g_signal_connect(manager->save_button, "clicked",
                     G_CALLBACK(on_partition_dialog_save_clicked), manager);
    g_signal_connect(manager->partition_dialog, "close-request",
                     G_CALLBACK(on_partition_dialog_close_request), manager);

    g_signal_connect(manager->swap_switch, "state-set",
                     G_CALLBACK(on_swap_switch_toggled), manager);
    g_signal_connect(manager->mount_point_combo, "notify::selected",
                     G_CALLBACK(on_mount_point_combo_changed), manager);
    g_signal_connect(manager->format_combo, "notify::selected",
                     G_CALLBACK(on_format_combo_changed), manager);

    LOG_INFO("Widgets del PartitionManager configurados correctamente");
    return TRUE;
}

// Mostrar el diálogo de configuración
void
partition_manager_show_dialog(PartitionManager *manager,
                             const gchar *device_path,
                             const gchar *current_filesystem,
                             const gchar *current_mount_point,
                             GtkWindow *parent)
{
    if (!manager || !device_path) return;

    // Limpiar configuración anterior
    if (manager->current_config) {
        partition_manager_free_config(manager->current_config);
        manager->current_config = NULL;
    }

    // Crear nueva configuración
    manager->current_config = partition_manager_create_config(device_path,
                                                              current_filesystem ? current_filesystem : "ext4",
                                                              current_mount_point ? current_mount_point : "/",
                                                              FALSE);

    // Actualizar título de la ventana
    if (manager->window_title) {
        adw_window_title_set_title(manager->window_title, device_path);
    }

    // Configurar valores iniciales
    if (manager->swap_switch) {
        gtk_switch_set_active(manager->swap_switch, manager->current_config->is_swap);
    }

    // Configurar punto de montaje
    if (manager->mount_point_combo && manager->current_config->mount_point) {
        // Buscar el índice del punto de montaje
        guint n_items = g_list_model_get_n_items(G_LIST_MODEL(manager->mount_point_list));
        for (guint i = 0; i < n_items; i++) {
            const gchar *item = gtk_string_list_get_string(manager->mount_point_list, i);
            if (g_strcmp0(item, manager->current_config->mount_point) == 0) {
                adw_combo_row_set_selected(manager->mount_point_combo, i);
                break;
            }
        }
    }

    // Configurar formato
    if (manager->format_combo && manager->current_config->filesystem) {
        // Buscar el índice del formato
        guint n_items = g_list_model_get_n_items(G_LIST_MODEL(manager->format_list));
        for (guint i = 0; i < n_items; i++) {
            const gchar *item = gtk_string_list_get_string(manager->format_list, i);
            gchar *format_without_mkfs = partition_manager_mkfs_to_format(manager->current_config->filesystem);
            if (g_ascii_strcasecmp(item, format_without_mkfs) == 0) {
                adw_combo_row_set_selected(manager->format_combo, i);
                g_free(format_without_mkfs);
                break;
            }
            g_free(format_without_mkfs);
        }
    }

    // Mostrar el diálogo
    if (parent) {
        gtk_window_set_transient_for(GTK_WINDOW(manager->partition_dialog), parent);
    }

    gtk_widget_set_visible(GTK_WIDGET(manager->partition_dialog), TRUE);

    LOG_INFO("Diálogo de configuración mostrado para: %s", device_path);
}

// Crear nueva configuración de partición
PartitionConfig*
partition_manager_create_config(const gchar *device_path,
                               const gchar *filesystem,
                               const gchar *mount_point,
                               gboolean is_swap)
{
    if (!device_path) return NULL;

    PartitionConfig *config = g_malloc0(sizeof(PartitionConfig));

    config->device_path = g_strdup(device_path);
    config->filesystem = g_strdup(filesystem ? filesystem : "ext4");
    config->mount_point = g_strdup(mount_point ? mount_point : "/");
    config->is_swap = is_swap;
    config->format_needed = FALSE;
    config->original_filesystem = g_strdup(filesystem ? filesystem : "ext4");

    return config;
}

// Liberar configuración de partición
void
partition_manager_free_config(PartitionConfig *config)
{
    if (!config) return;

    g_free(config->device_path);
    g_free(config->filesystem);
    g_free(config->mount_point);
    g_free(config->original_filesystem);
    g_free(config);
}

// Copiar configuración de partición
PartitionConfig*
partition_manager_copy_config(PartitionConfig *config)
{
    if (!config) return NULL;

    return partition_manager_create_config(config->device_path,
                                          config->filesystem,
                                          config->mount_point,
                                          config->is_swap);
}

// Guardar configuraciones en variables.sh
gboolean
partition_manager_save_to_variables(PartitionManager *manager)
{
    if (!manager) return FALSE;

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);

    // Leer el archivo existente para preservar otras variables
    GString *existing_content = g_string_new("");
    FILE *read_file = fopen(bash_file_path, "r");
    gboolean in_partitions_array = FALSE;

    if (read_file) {
        char line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Detectar inicio del array PARTITIONS
            if (g_str_has_prefix(line, "PARTITIONS=(")) {
                in_partitions_array = TRUE;
                continue; // No incluir esta línea
            }

            // Detectar fin del array PARTITIONS
            if (in_partitions_array && g_str_has_suffix(line, ")\n")) {
                in_partitions_array = FALSE;
                continue; // No incluir esta línea
            }

            // Si estamos dentro del array, saltar las líneas
            if (in_partitions_array) {
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
        return FALSE;
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

    // Agregar el array de particiones
    if (manager->partition_configs) {
        //fprintf(file, "# Crear array de \"estructuras\" separadas por espacio\n");
        fprintf(file, "PARTITIONS=(\n");

        GList *l;
        for (l = manager->partition_configs; l != NULL; l = l->next) {
            PartitionConfig *config = (PartitionConfig*)l->data;
            if (config) {
                if (config->is_swap) {
                    fprintf(file, "    \"%s %s swap\"\n",
                           config->device_path, config->filesystem);
                } else {
                    fprintf(file, "    \"%s %s %s\"\n",
                           config->device_path, config->filesystem, config->mount_point);
                }
            }
        }

        fprintf(file, ")\n");
    } else {
        //fprintf(file, "# Crear array de \"estructuras\" separadas por espacio\n");
        fprintf(file, "PARTITIONS=()\n");
    }

    fclose(file);
    g_string_free(existing_content, TRUE);

    LOG_INFO("Configuraciones de partición guardadas en: %s", bash_file_path);
    g_free(bash_file_path);
    return TRUE;
}

// Cargar configuraciones desde variables.sh
gboolean
partition_manager_load_from_variables(PartitionManager *manager)
{
    if (!manager) return FALSE;

    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);
    FILE *file = fopen(bash_file_path, "r");

    if (!file) {
        LOG_INFO("Archivo de variables no encontrado, usando valores por defecto");
        g_free(bash_file_path);
        return FALSE;
    }

    // Limpiar configuraciones existentes
    partition_manager_clear_configs(manager);

    char line[1024];
    gboolean in_partitions_array = FALSE;

    while (fgets(line, sizeof(line), file)) {
        // Remover salto de línea
        line[strcspn(line, "\n")] = 0;

        // Detectar inicio del array PARTITIONS
        if (g_str_has_prefix(line, "PARTITIONS=(")) {
            in_partitions_array = TRUE;
            continue;
        }

        // Detectar fin del array PARTITIONS
        if (in_partitions_array && g_str_has_suffix(line, ")")) {
            in_partitions_array = FALSE;
            continue;
        }

        // Si estamos dentro del array, procesar las líneas
        if (in_partitions_array) {
            // Limpiar espacios y comillas
            gchar *cleaned_line = g_strstrip(g_strdup(line));

            // Remover comillas al inicio y final
            if (cleaned_line[0] == '"' && strlen(cleaned_line) > 1 &&
                cleaned_line[strlen(cleaned_line)-1] == '"') {
                cleaned_line[strlen(cleaned_line)-1] = 0;
                cleaned_line++;
            }

            // Parsear la línea: "device filesystem mount_point"
            gchar **parts = g_strsplit(cleaned_line, " ", 3);
            if (parts && parts[0] && parts[1]) {
                gchar *device_path = parts[0];
                gchar *filesystem = parts[1];
                gchar *mount_point = parts[2];

                gboolean is_swap = (g_strcmp0(mount_point, "swap") == 0);

                PartitionConfig *config = partition_manager_create_config(
                    device_path, filesystem,
                    is_swap ? "swap" : mount_point,
                    is_swap);

                if (config) {
                    partition_manager_add_config(manager, config);
                    LOG_INFO("Configuración de partición cargada: %s %s %s",
                            device_path, filesystem, is_swap ? "swap" : mount_point);
                }
            }

            g_strfreev(parts);
            g_free(cleaned_line - 1); // Ajustar por el incremento del puntero
        }
    }

    fclose(file);
    g_free(bash_file_path);

    LOG_INFO("Configuraciones de partición cargadas desde variables.sh");
    return TRUE;
}

// Agregar configuración a la lista
void
partition_manager_add_config(PartitionManager *manager, PartitionConfig *config)
{
    if (!manager || !config) return;

    // Verificar si ya existe una configuración para este dispositivo
    PartitionConfig *existing = partition_manager_find_config(manager, config->device_path);
    if (existing) {
        // Reemplazar la configuración existente
        partition_manager_remove_config(manager, config->device_path);
    }

    manager->partition_configs = g_list_append(manager->partition_configs, config);
    LOG_INFO("Configuración agregada para: %s", config->device_path);
}

// Remover configuración de la lista
void
partition_manager_remove_config(PartitionManager *manager, const gchar *device_path)
{
    if (!manager || !device_path) return;

    GList *l;
    for (l = manager->partition_configs; l != NULL; l = l->next) {
        PartitionConfig *config = (PartitionConfig*)l->data;
        if (config && g_strcmp0(config->device_path, device_path) == 0) {
            manager->partition_configs = g_list_remove(manager->partition_configs, config);
            partition_manager_free_config(config);
            LOG_INFO("Configuración removida para: %s", device_path);
            break;
        }
    }
}

// Encontrar configuración por device_path
PartitionConfig*
partition_manager_find_config(PartitionManager *manager, const gchar *device_path)
{
    if (!manager || !device_path) return NULL;

    GList *l;
    for (l = manager->partition_configs; l != NULL; l = l->next) {
        PartitionConfig *config = (PartitionConfig*)l->data;
        if (config && g_strcmp0(config->device_path, device_path) == 0) {
            return config;
        }
    }

    return NULL;
}

// Limpiar todas las configuraciones
void
partition_manager_clear_configs(PartitionManager *manager)
{
    if (!manager) return;

    GList *l;
    for (l = manager->partition_configs; l != NULL; l = l->next) {
        PartitionConfig *config = (PartitionConfig*)l->data;
        if (config) {
            partition_manager_free_config(config);
        }
    }

    g_list_free(manager->partition_configs);
    manager->partition_configs = NULL;

    LOG_INFO("Todas las configuraciones de partición limpiadas");
}

// Obtener lista de configuraciones
GList*
partition_manager_get_configs(PartitionManager *manager)
{
    if (!manager) return NULL;
    return manager->partition_configs;
}

// Obtener cantidad de configuraciones
guint
partition_manager_get_config_count(PartitionManager *manager)
{
    if (!manager) return 0;
    return g_list_length(manager->partition_configs);
}

// Establecer callback para cuando se guarda
void
partition_manager_set_save_callback(PartitionManager *manager,
                                   void (*callback)(PartitionConfig *config, gpointer user_data),
                                   gpointer user_data)
{
    if (!manager) return;

    manager->on_config_saved = callback;
    manager->callback_data = user_data;
}

// Validar si hay partición root
gboolean
partition_manager_has_root_partition(PartitionManager *manager)
{
    if (!manager) return FALSE;

    GList *l;
    for (l = manager->partition_configs; l != NULL; l = l->next) {
        PartitionConfig *config = (PartitionConfig*)l->data;
        if (config && !config->is_swap && g_strcmp0(config->mount_point, "/") == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

// Validar si hay partición swap
gboolean
partition_manager_has_swap_partition(PartitionManager *manager)
{
    if (!manager) return FALSE;

    GList *l;
    for (l = manager->partition_configs; l != NULL; l = l->next) {
        PartitionConfig *config = (PartitionConfig*)l->data;
        if (config && config->is_swap) {
            return TRUE;
        }
    }

    return FALSE;
}

// CALLBACKS

// Callback para cancelar
void
on_partition_dialog_cancel_clicked(GtkButton *button, gpointer user_data)
{
    PartitionManager *manager = (PartitionManager*)user_data;

    if (!manager) return;

    LOG_INFO("Configuración de partición cancelada");
    gtk_widget_set_visible(GTK_WIDGET(manager->partition_dialog), FALSE);
}

// Callback para guardar
void
on_partition_dialog_save_clicked(GtkButton *button, gpointer user_data)
{
    PartitionManager *manager = (PartitionManager*)user_data;

    if (!manager || !manager->current_config) return;

    // Obtener valores actuales de los widgets
    gboolean is_swap = gtk_switch_get_active(manager->swap_switch);

    // Actualizar configuración
    manager->current_config->is_swap = is_swap;

    if (is_swap) {
        // Para swap, usar mkswap
        g_free(manager->current_config->filesystem);
        manager->current_config->filesystem = g_strdup("mkswap");
        g_free(manager->current_config->mount_point);
        manager->current_config->mount_point = g_strdup("swap");
    } else {
        // Obtener punto de montaje seleccionado
        guint mount_selected = adw_combo_row_get_selected(manager->mount_point_combo);
        if (mount_selected != GTK_INVALID_LIST_POSITION) {
            const gchar *mount_point = gtk_string_list_get_string(manager->mount_point_list, mount_selected);
            g_free(manager->current_config->mount_point);
            manager->current_config->mount_point = g_strdup(mount_point);
        }

        // Obtener formato seleccionado
        guint format_selected = adw_combo_row_get_selected(manager->format_combo);
        if (format_selected != GTK_INVALID_LIST_POSITION) {
            const gchar *format = gtk_string_list_get_string(manager->format_list, format_selected);
            g_free(manager->current_config->filesystem);
            
            // Agregar prefijo mkfs. al formato usando función auxiliar
            manager->current_config->filesystem = partition_manager_format_to_mkfs(format);
        }
    }

    // Verificar si necesita formateo
    if (g_strcmp0(manager->current_config->filesystem, manager->current_config->original_filesystem) != 0) {
        manager->current_config->format_needed = TRUE;
    }

    // Agregar/actualizar configuración en la lista
    PartitionConfig *config_copy = partition_manager_copy_config(manager->current_config);
    partition_manager_add_config(manager, config_copy);

    // Guardar en variables.sh
    partition_manager_save_to_variables(manager);

    // Llamar callback si existe
    if (manager->on_config_saved) {
        manager->on_config_saved(manager->current_config, manager->callback_data);
    }

    LOG_INFO("Configuración de partición guardada: %s -> %s (%s)",
            manager->current_config->device_path,
            manager->current_config->is_swap ? "swap" : manager->current_config->mount_point,
            manager->current_config->filesystem);

    // Ocultar diálogo
    gtk_widget_set_visible(GTK_WIDGET(manager->partition_dialog), FALSE);
}

// Callback para cerrar ventana
gboolean
on_partition_dialog_close_request(AdwWindow *window, gpointer user_data)
{
    LOG_INFO("Diálogo de configuración de partición cerrado");
    gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
    return TRUE; // Prevenir destrucción del diálogo
}

// Callback para cambio en switch de swap
gboolean
on_swap_switch_toggled(GtkSwitch *switch_widget, gboolean state, gpointer user_data)
{
    PartitionManager *manager = (PartitionManager*)user_data;

    if (!manager) {
        LOG_ERROR("Manager es NULL en on_swap_switch_toggled");
        return FALSE;
    }

    // Validar que los widgets existan
    if (!manager->mount_point_combo) {
        LOG_ERROR("mount_point_combo es NULL");
        return FALSE;
    }

    if (!manager->format_combo) {
        LOG_ERROR("format_combo es NULL");
        return FALSE;
    }

    gboolean is_swap = state;

    // Habilitar/deshabilitar widgets según el estado del swap
    gtk_widget_set_sensitive(GTK_WIDGET(manager->mount_point_combo), !is_swap);

    if (is_swap) {
        // Para swap, deshabilitar también el combo de formato
        gtk_widget_set_sensitive(GTK_WIDGET(manager->format_combo), FALSE);
    } else {
        // Habilitar el combo de formato cuando no es swap
        gtk_widget_set_sensitive(GTK_WIDGET(manager->format_combo), TRUE);
    }

    LOG_INFO("Swap %s", is_swap ? "habilitado" : "deshabilitado");
    return FALSE; // Permitir que el switch cambie de estado
}

// Callback para cambio en combo de punto de montaje
void
on_mount_point_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    PartitionManager *manager = (PartitionManager*)user_data;

    if (!manager) return;

    guint selected = adw_combo_row_get_selected(combo);

    if (selected != GTK_INVALID_LIST_POSITION) {
        const gchar *mount_point = gtk_string_list_get_string(manager->mount_point_list, selected);
        LOG_INFO("Punto de montaje seleccionado: %s", mount_point);
    }
}

// Callback para cambio en combo de formato
void
on_format_combo_changed(AdwComboRow *combo, GParamSpec *pspec, gpointer user_data)
{
    PartitionManager *manager = (PartitionManager*)user_data;

    if (!manager) return;

    guint selected = adw_combo_row_get_selected(combo);

    if (selected != GTK_INVALID_LIST_POSITION) {
        const gchar *format = gtk_string_list_get_string(manager->format_list, selected);
        LOG_INFO("Formato seleccionado: %s", format);
    }
}

// Función auxiliar para convertir formato de UI a formato mkfs
gchar* partition_manager_format_to_mkfs(const gchar *format)
{
    if (!format) return g_strdup("mkfs.ext4");
    
    // Caso especial: Sin Formatear
    if (g_strcmp0(format, "Sin Formatear") == 0) {
        return g_strdup("none");
    }
    
    gchar *format_lower = g_ascii_strdown(format, -1);
    gchar *result = g_strdup_printf("mkfs.%s", format_lower);
    g_free(format_lower);
    
    return result;
}

// Función auxiliar para convertir formato mkfs a formato de UI
gchar* partition_manager_mkfs_to_format(const gchar *mkfs_format)
{
    if (!mkfs_format) return g_strdup("Ext4");
    
    // Caso especial: none
    if (g_strcmp0(mkfs_format, "none") == 0) {
        return g_strdup("Sin Formatear");
    }
    
    // Caso especial: mkswap
    if (g_strcmp0(mkfs_format, "mkswap") == 0) {
        return g_strdup("Sin Formatear");
    }
    
    // Si ya tiene prefijo mkfs., quitarlo
    if (g_str_has_prefix(mkfs_format, "mkfs.")) {
        const gchar *format_part = mkfs_format + 5; // Saltar "mkfs."
        // Capitalizar primera letra
        if (strlen(format_part) > 0) {
            gchar *result = g_strdup(format_part);
            result[0] = g_ascii_toupper(result[0]);
            return result;
        }
    }
    
    // Si no tiene prefijo, capitalizar primera letra
    if (strlen(mkfs_format) > 0) {
        gchar *result = g_strdup(mkfs_format);
        result[0] = g_ascii_toupper(result[0]);
        return result;
    }
    
    return g_strdup("Ext4");
}
