#include "disk_manager.h"
#include "config.h"
#include <string.h>

// Función para liberar memoria de DiskInfo
void 
disk_info_free(DiskInfo *disk_info)
{
    if (disk_info) {
        g_free(disk_info->device_path);
        g_free(disk_info->display_text);
        g_free(disk_info);
    }
}

// Función para determinar si es un dispositivo principal (no partición)
gboolean
disk_manager_is_main_device(const gchar *device_name)
{
    if (!device_name)
        return FALSE;
        
    gsize len = strlen(device_name);
    if (len == 0)
        return FALSE;
    
    // Para dispositivos SATA/SCSI (sda, sdb, etc.)
    if (len >= 3 && g_str_has_prefix(device_name, "sd") && g_ascii_isdigit(device_name[len-1])) {
        return FALSE;
    }
    
    // Para dispositivos NVMe (nvme0n1p1, etc.)
    if (strstr(device_name, "p") && g_ascii_isdigit(device_name[len-1])) {
        return FALSE;
    }
    
    // Para dispositivos MMC (mmcblk0p1, etc.)
    if (strstr(device_name, "p") && g_str_has_prefix(device_name, "mmcblk")) {
        return FALSE;
    }
    
    return TRUE;
}

// Crear nuevo DiskManager
DiskManager* 
disk_manager_new(void)
{
    DiskManager *manager = g_malloc0(sizeof(DiskManager));
    
    // Inicializar variables
    manager->selected_disk_path = NULL;
    
    return manager;
}

// Liberar memoria del DiskManager
void 
disk_manager_free(DiskManager *manager)
{
    if (!manager) return;
    
    // Limpiar objetos GObject
    g_clear_object(&manager->udisks_client);
    g_clear_object(&manager->disk_store);
    g_clear_object(&manager->disk_paths);
    
    // Liberar strings
    g_free(manager->selected_disk_path);
    
    // Liberar la estructura
    g_free(manager);
    
    LOG_INFO("DiskManager liberado correctamente");
}

// Configurar UDisks2
gboolean 
disk_manager_setup_udisks(DiskManager *manager)
{
    if (!manager) return FALSE;
    
    GError *error = NULL;
    
    // Crear cliente UDisks2
    manager->udisks_client = udisks_client_new_sync(NULL, &error);
    if (error) {
        LOG_ERROR("No se pudo crear el cliente UDisks2: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    LOG_INFO("Cliente UDisks2 inicializado correctamente");
    return TRUE;
}

// Inicializar widgets del DiskManager
gboolean 
disk_manager_init(DiskManager *manager, GtkBuilder *builder)
{
    if (!manager || !builder) {
        LOG_ERROR("Parámetros inválidos para inicializar DiskManager");
        return FALSE;
    }
    
    // Obtener widgets del builder
    manager->disk_combo = ADW_COMBO_ROW(gtk_builder_get_object(builder, "disk_combo"));
    manager->refresh_button = GTK_BUTTON(gtk_builder_get_object(builder, "refresh_button"));
    
    // Verificar que se obtuvieron los widgets
    if (!manager->disk_combo) {
        LOG_ERROR("No se pudo obtener disk_combo del builder");
        return FALSE;
    }
    
    // Crear modelos de datos
    manager->disk_store = gtk_string_list_new(NULL);
    manager->disk_paths = gtk_string_list_new(NULL);
    
    // Configurar el combo row
    adw_combo_row_set_model(manager->disk_combo, G_LIST_MODEL(manager->disk_store));
    
    // Configurar propiedades del combo
    g_object_set(manager->disk_combo, 
                 "title", "Disco de instalación",
                 "subtitle", "Selecciona un dispositivo de almacenamiento",
                 NULL);
    
    // Conectar señales
    g_signal_connect(manager->disk_combo, "notify::selected", 
                     G_CALLBACK(on_disk_manager_selection_changed), manager);
    
    if (manager->refresh_button) {
        g_signal_connect(manager->refresh_button, "clicked", 
                         G_CALLBACK(on_disk_manager_refresh_clicked), manager);
    }
    
    // Configurar UDisks2
    if (!disk_manager_setup_udisks(manager)) {
        LOG_WARNING("UDisks2 no disponible, funcionalidad limitada");
    }
    
    // Poblar lista inicial
    disk_manager_populate_list(manager);
    
    // Cargar configuración guardada
    disk_manager_load_from_variables(manager);
    
    LOG_INFO("DiskManager inicializado correctamente");
    return TRUE;
}

// Función para poblar la lista de discos
void
disk_manager_populate_list(DiskManager *manager)
{
    if (!manager) return;
    
    GList *objects, *l;
    UDisksObject *object;
    UDisksBlock *block;
    
    // Limpiar listas actuales
    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(manager->disk_store));
    if (n_items > 0) {
        gtk_string_list_splice(manager->disk_store, 0, n_items, NULL);
    }
    
    n_items = g_list_model_get_n_items(G_LIST_MODEL(manager->disk_paths));
    if (n_items > 0) {
        gtk_string_list_splice(manager->disk_paths, 0, n_items, NULL);
    }
    
    if (!manager->udisks_client) {
        LOG_WARNING("Cliente UDisks2 no disponible");
        return;
    }
    
    LOG_INFO("Escaneando dispositivos de almacenamiento...");
    
    // Obtener todos los objetos de UDisks2
    objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(manager->udisks_client));
    
    int disk_count = 0;
    for (l = objects; l != NULL; l = l->next) {
        object = UDISKS_OBJECT(l->data);
        block = udisks_object_peek_block(object);
        
        if (block != NULL) {
            const gchar *device_path = udisks_block_get_device(block);
            guint64 size = udisks_block_get_size(block);
            
            // Solo mostrar dispositivos principales (no particiones)
            if (device_path && size > 0) {
                const gchar *device_name = g_path_get_basename(device_path);
                
                if (disk_manager_is_main_device(device_name)) {
                    // Crear descripción para mostrar: /dev/sda - XX.X GB
                    gchar *display_text = g_strdup_printf("%s - %.0f GB", 
                                                        device_path,
                                                        size / 1000000000.0);
                    
                    gtk_string_list_append(manager->disk_store, display_text);
                    gtk_string_list_append(manager->disk_paths, device_path);
                    
                    disk_count++;
                    LOG_INFO("Disco encontrado: %s", display_text);
                    
                    g_free(display_text);
                }
            }
        }
    }
    
    g_list_free_full(objects, g_object_unref);
    
    LOG_INFO("Escaneo completo: %d discos encontrados", disk_count);
}

// Función para refrescar la lista
void 
disk_manager_refresh(DiskManager *manager)
{
    if (!manager) return;
    
    LOG_INFO("Actualizando lista de discos...");
    
    // Limpiar selección actual
    adw_combo_row_set_selected(manager->disk_combo, GTK_INVALID_LIST_POSITION);
    g_object_set(manager->disk_combo, "subtitle", "Selecciona un dispositivo de almacenamiento", NULL);
    
    // Actualizar la lista
    disk_manager_populate_list(manager);
}

// Obtener disco seleccionado
const gchar* 
disk_manager_get_selected_disk(DiskManager *manager)
{
    if (!manager) return NULL;
    return manager->selected_disk_path;
}

// Callback para cuando se selecciona un disco
void
on_disk_manager_selection_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    DiskManager *manager = (DiskManager *)user_data;
    AdwComboRow *combo = ADW_COMBO_ROW(object);
    
    if (!manager || !combo) return;
    
    guint selected = adw_combo_row_get_selected(combo);
    
    if (selected != GTK_INVALID_LIST_POSITION) {
        const gchar *device_path = gtk_string_list_get_string(manager->disk_paths, selected);
        if (device_path) {
            // Actualizar disco seleccionado
            g_free(manager->selected_disk_path);
            manager->selected_disk_path = g_strdup(device_path);
            
            LOG_INFO("Disco seleccionado: %s", device_path);
            
            // Actualizar el subtitle del combo row
            gchar *subtitle_text = g_strdup_printf("Seleccionado: %s", device_path);
            g_object_set(manager->disk_combo, "subtitle", subtitle_text, NULL);
            g_free(subtitle_text);
            
            // Guardar la selección en variables.sh
            disk_manager_save_to_variables(manager);
        }
    } else {
        // Si no hay selección, limpiar
        g_free(manager->selected_disk_path);
        manager->selected_disk_path = NULL;
        g_object_set(manager->disk_combo, "subtitle", "Selecciona un dispositivo de almacenamiento", NULL);
        
        // Guardar el estado vacío en variables.sh
        disk_manager_save_to_variables(manager);
    }
}

// Callback para el botón de actualizar
void
on_disk_manager_refresh_clicked(GtkButton *button, gpointer user_data)
{
    DiskManager *manager = (DiskManager *)user_data;
    
    if (!manager) return;
    
    disk_manager_refresh(manager);
    
    LOG_INFO("Lista de discos actualizada manualmente");
}

// Función para guardar la variable del disco seleccionado al archivo variables.sh
gboolean 
disk_manager_save_to_variables(DiskManager *manager)
{
    if (!manager) return FALSE;
    
    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);
    
    // Leer el archivo existente para preservar otras variables
    GString *existing_content = g_string_new("");
    FILE *read_file = fopen(bash_file_path, "r");

    
    if (read_file) {
        char line[1024];
        while (fgets(line, sizeof(line), read_file)) {
            // Skip la línea de SELECTED_DISK si existe
            if (g_str_has_prefix(line, "SELECTED_DISK=")) {
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
    
    // Agregar la variable del disco seleccionado
    if (manager->selected_disk_path) {
        fprintf(file, "SELECTED_DISK=\"%s\"\n", manager->selected_disk_path);
    } else {
        fprintf(file, "SELECTED_DISK=\"\"\n");
    }
    
    // No agregar línea final duplicada
    
    fclose(file);
    g_string_free(existing_content, TRUE);
    
    LOG_INFO("Variable SELECTED_DISK guardada en: %s", bash_file_path);
    g_free(bash_file_path);
    return TRUE;
}

// Función para cargar la variable del disco desde el archivo variables.sh
gboolean 
disk_manager_load_from_variables(DiskManager *manager)
{
    if (!manager) return FALSE;
    
    gchar *bash_file_path = g_build_filename(".", "data", "variables.sh", NULL);
    FILE *file = fopen(bash_file_path, "r");
    
    if (!file) {
        LOG_INFO("Archivo de variables no encontrado, usando valores por defecto");
        g_free(bash_file_path);
        return FALSE;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Remover salto de línea
        line[strcspn(line, "\n")] = 0;
        
        // Buscar la variable SELECTED_DISK
        if (g_str_has_prefix(line, "SELECTED_DISK=")) {
            char *value = line + 14; // Saltar "SELECTED_DISK="
            
            // Remover comillas si existen
            if (value[0] == '"' && strlen(value) > 1 && value[strlen(value)-1] == '"') {
                value[strlen(value)-1] = 0;
                value++;
            }
            
            // Solo cargar si el valor no está vacío
            if (strlen(value) > 0) {
                g_free(manager->selected_disk_path);
                manager->selected_disk_path = g_strdup(value);
                LOG_INFO("Disco cargado desde variables: %s", value);
            }
            break;
        }
    }
    
    fclose(file);
    g_free(bash_file_path);
    return TRUE;
}