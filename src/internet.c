#include "internet.h"
#include <libsoup/soup.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib.h>
#include <gio/gio.h>

// Variables globales
CharPage1 char_page1 = {NULL, NULL, NULL}; // Inicialización global


// Función para leer la salida de un GInputStream
gchar* read_output_from_stream(GInputStream *stream, GError **error) {
    GDataInputStream *data_stream = g_data_input_stream_new(stream);
    gchar *output = g_data_input_stream_read_line(data_stream, NULL, NULL, error);
    g_object_unref(data_stream);
    return output;
}

// Función para obtener el idioma principal desde la API
char* get_primary_language_from_api() {
    GError *error = NULL;

    GSubprocess *process = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE, 
        &error, 
        "curl", 
        "-s", 
        "https://ipapi.co/languages", 
        NULL
    );

    if (!process) {
g_printerr("Error al ejecutar curl: %s\n", error->message);
g_clear_error(&error);
        return NULL;
}

    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(process);
    gchar *output = read_output_from_stream(stdout_stream, &error);
    g_object_unref(process);

    if (!output) {
        g_printerr("Error al leer la salida de curl: %s\n", error->message);
        g_clear_error(&error);
        return NULL;
    }

    gchar *primary_language = g_strndup(output, 2); // Extraer los primeros 2 caracteres
    g_free(output);

    return primary_language;
}

// Función para obtener la zona horaria desde la API
char* get_timezone_from_api() {
    GError *error = NULL;

    GSubprocess *process = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE, 
        &error, 
        "curl", 
        "-s", 
        "https://ipapi.co/timezone", 
        NULL
    );

    if (!process) {
        g_printerr("Error al ejecutar curl: %s\n", error->message);
        g_clear_error(&error);
        return NULL;
    }

    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(process);
    gchar *output = read_output_from_stream(stdout_stream, &error);
    g_object_unref(process);

    if (!output) {
        g_printerr("Error al leer la salida de curl: %s\n", error->message);
        g_clear_error(&error);
        return NULL;
    }

    output[strcspn(output, "\n")] = '\0'; // Eliminar salto de línea
    return output;
}

// Función para obtener el número de línea de una zona horaria
int get_timezone_line(const char *timezone) {
    GError *error = NULL;

    // Crear el comando con la zona horaria
    char command[256];
    snprintf(command, sizeof(command),
             "timedatectl --no-pager list-timezones | nl | grep '%s' | awk '{print $1}'",
             timezone);

    // Crear un subproceso para ejecutar el comando
    GSubprocess *process = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE,
        &error,
        "sh",
        "-c",
        command,
        NULL
    );

    if (!process) {
        g_printerr("Error al ejecutar el comando: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Obtener el flujo de salida estándar
    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(process);

    // Leer la salida del flujo
    gchar *output = read_output_from_stream(stdout_stream, &error);
    g_object_unref(process);

    if (!output) {
        g_printerr("Error al leer la salida del comando: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Convertir la salida a un entero (número de línea)
    int line_number = -1;
    if (sscanf(output, "%d", &line_number) != 1) {
        g_printerr("No se pudo parsear la línea del resultado\n");
        line_number = -1;
    }

    g_free(output);
    return line_number;
}

// Función para obtener el idioma y país desde la API
char* get_locale_from_api() {
    FILE *fp = popen("curl -s https://ipapi.co/languages", "r");
    if (!fp) {
        perror("Error al ejecutar curl");
        return NULL;
    }

    static char locale[128]; // Locale (estático para retornar puntero)
    if (!fgets(locale, sizeof(locale), fp)) {
        perror("Error al leer la respuesta de curl");
        pclose(fp);
        return NULL;
    }
    pclose(fp);

    // Eliminar el salto de línea
    locale[strcspn(locale, "\n")] = '\0';

    // Procesar la respuesta para obtener solo el idioma y país principal
    char *comma = strchr(locale, ','); // Buscar la primera coma
    if (comma) {
        *comma = '\0'; // Cortar la cadena en la coma
    }

    // Reemplazar '-' por '_' en el formato
    for (char *p = locale; *p; p++) {
        if (*p == '-') {
            *p = '_';
        }
    }

    // Añadir ".UTF-8" al final
    strcat(locale, ".UTF-8");

    return locale;
}



// Función para obtener el número de línea de un layout de teclado basado en el idioma principal
int get_keyboard_layout_line_from_language() {
    // Obtener el idioma principal desde la API
    char *primary_language = get_primary_language_from_api();
    if (!primary_language) {
        g_printerr("No se pudo obtener el idioma principal.\n");
        return -1;
    }

    GError *error = NULL;

    // Crear el comando con el idioma principal
    char command[256];
    snprintf(command, sizeof(command),
             "localectl list-x11-keymap-layouts | nl | grep '^.*%s.*$' | awk '{print $1}'",
             primary_language);

    // Crear un subproceso para ejecutar el comando
    GSubprocess *process = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE,
        &error,
        "sh",
        "-c",
        command,
        NULL
    );

    // Liberar la memoria del idioma principal
    g_free(primary_language);

    if (!process) {
        g_printerr("Error al ejecutar el comando: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Obtener el flujo de salida estándar
    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(process);

    // Leer la salida del flujo
    gchar *output = read_output_from_stream(stdout_stream, &error);
    g_object_unref(process);

    if (!output) {
        g_printerr("Error al leer la salida del comando: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Convertir la salida a un entero (número de línea)
    int line_number = -1;
    if (sscanf(output, "%d", &line_number) != 1) {
        g_printerr("No se pudo parsear la línea del resultado\n");
        line_number = -1;
    }

    g_free(output);
    return line_number;
}


// Función para obtener el layout de teclado TTY basado en el idioma principal
char* get_tty_keyboard_layout_from_language() {
    // Obtener el idioma principal desde la API
    char *primary_language = get_primary_language_from_api();
    if (!primary_language) {
        g_printerr("No se pudo obtener el idioma principal.\n");
        return NULL;
    }

    GError *error = NULL;

    // Crear el comando con el idioma principal
    char command[256];
    snprintf(command, sizeof(command),
             "localectl list-keymaps | grep -x '%s'",
             primary_language);

    // Crear un subproceso para ejecutar el comando
    GSubprocess *process = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE,
        &error,
        "sh",
        "-c",
        command,
        NULL
    );

    if (!process) {
        g_printerr("Error al ejecutar el comando: %s\n", error->message);
        g_clear_error(&error);
        g_free(primary_language);
        return NULL;
    }

    // Obtener el flujo de salida estándar
    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(process);

    // Leer la salida del flujo
    gchar *output = read_output_from_stream(stdout_stream, &error);
    g_object_unref(process);
    g_free(primary_language);

    if (!output) {
        g_printerr("Error al leer la salida del comando: %s\n", error->message);
        g_clear_error(&error);
        return NULL;
    }

    // Limpiar el salto de línea al final, si lo hay
    output[strcspn(output, "\n")] = '\0';
    return output;
}


// Función para obtener el número de línea de un layout TTY
int get_tty_keyboard_layout_line(const char *layout) {
    GError *error = NULL;

    // Crear el comando para obtener el número de línea del layout TTY
    char command[256];
    snprintf(command, sizeof(command),
             "localectl list-keymaps | grep -x '%s' -n | awk -F: '{print $1}' | sed '/^$/d'",
             layout);

    // Crear un subproceso para ejecutar el comando
    GSubprocess *process = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE,
        &error,
        "sh",
        "-c",
        command,
        NULL
    );

    if (!process) {
        g_printerr("Error al ejecutar el comando: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Obtener el flujo de salida estándar
    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(process);

    // Leer la salida del flujo
    gchar *output = read_output_from_stream(stdout_stream, &error);
    g_object_unref(process);

    if (!output) {
        g_printerr("Error al leer la salida del comando: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Convertir la salida a un entero (número de línea)
    int line_number = -1;
    if (sscanf(output, "%d", &line_number) != 1) {
        g_printerr("No se pudo parsear la línea del resultado\n");
        line_number = -1;
    }

    g_free(output);
    return line_number;
}


// FUNCION PARA OBTENER EL NUMERO DE LINEA DEL PAIS
int get_locale_pais_line_number(const char *target_locale) {
    // Ejecutar el comando y abrir un pipe para leer su salida
    FILE *fp_locale = popen("cat ./data/locale.gen | grep -v '#  ' | sed 's/#//g' | grep '.UTF-8 UTF-8' | awk '{print $1}'", "r");
    
    if (fp_locale == NULL) {
        perror("popen failed");
        return -1; // Error en la ejecución
    }

    char buffer_locale[50];
    int line_number = 0; // Número de línea actual
    int found_line = -1; // Almacena el número de línea encontrada

    // Leer cada línea y comparar con el locale deseado
    while (fgets(buffer_locale, sizeof(buffer_locale), fp_locale) != NULL) {
        line_number++; // Incrementar número de línea

        // Eliminar el salto de línea al final de la cadena
        buffer_locale[strcspn(buffer_locale, "\n")] = '\0';

        // Comparar con el locale buscado
        if (strcmp(buffer_locale, target_locale) == 0) {
            found_line = line_number; // Guardar el número de línea donde se encontró
            break; // Terminar la búsqueda
        }
    }

    pclose(fp_locale);
    return found_line; // Retorna la línea donde está el locale o -1 si no se encontró
}

int line_number_locale;
char *tty_layout;
int tty_line;
int linea_zona;
int line_number_pais;

// Función para obtener idioma y zona horaria (en thread separado)
static gpointer fetch_language_and_timezone_thread(gpointer user_data) {

    // Obtener idioma
    g_print("Obteniendo idioma...\n");
    char_page1.language = get_primary_language_from_api();
    if (!char_page1.language) {
        g_print("No se pudo obtener el idioma desde la API.\n");
        return user_data;
    }

    // Obtener zona horaria
    g_print("Obteniendo zona horaria...\n");
    char_page1.timezone = get_timezone_from_api();
    if (!char_page1.timezone) {
        g_print("No se pudo obtener la zona horaria desde la API.\n");
        return user_data;
    }

    // Obtener localización
    g_print("Obteniendo localización...\n");
    char_page1.locale = get_locale_from_api();
    if (!char_page1.locale) {
        g_print("No se pudo obtener la localización desde la API.\n");
        return user_data;
    }
    
    // PARA OBTENER EL NUMERO DE LINEA
    line_number_locale = get_keyboard_layout_line_from_language() - 1;
    tty_layout = get_tty_keyboard_layout_from_language();
    tty_line = get_tty_keyboard_layout_line(tty_layout) - 1;
    g_print("%i\n", tty_line);

    // Obtener el número de línea de la zona horaria
    linea_zona = get_timezone_line(char_page1.timezone) - 1;
    g_print("%i\n", linea_zona);
    if (linea_zona == -1) {
        fprintf(stderr, "No se pudo encontrar el numero de zona horaria\n");
        
    }

    // Obtener el número de línea de la zona horaria
    line_number_pais = get_locale_pais_line_number(char_page1.locale) - 1;
    g_print("%i\n", line_number_pais);
    if (line_number_pais == -1) {
        fprintf(stderr, "No se pudo encontrar el numero de zona horaria\n");
        
    }

    return user_data; // Devuelve los datos al hilo principal
}

// Función para actualizar la interfaz (en el hilo principal)
static gboolean update_ui_after_fetch(gpointer user_data) {
    InternetWidgets *ui = (InternetWidgets *)user_data;

    if (!char_page1.language) {
        gtk_label_set_text(GTK_LABEL(ui->internet_label), "Error al obtener el idioma.");
        g_print("No se obtuvo el idioma, actualizando UI con mensaje de error.\n");
        return G_SOURCE_REMOVE;
    }

    gtk_label_set_text(GTK_LABEL(ui->internet_label), "Espere...");

    if (char_page1.locale) {
        gtk_widget_set_visible(ui->start_button, TRUE);
        gtk_widget_set_visible(ui->internet_label, FALSE);
        gtk_widget_set_visible(ui->spinner, FALSE);
        g_print("- Datos obtenidos correctamente\n");
        g_print("Idioma: %s\n", char_page1.language);
        g_print("Zona horaria: %s\n", char_page1.timezone);
        g_print("Localización: %s\n", char_page1.locale);
        // Note: ComboRow selection is now handled by page2.c module

    } else {
        g_print("- No se pudo obtener la localización\n");
    }

    return G_SOURCE_REMOVE;
}


static gboolean schedule_ui_update(gpointer user_data) {
    g_idle_add(update_ui_after_fetch, user_data);
    return G_SOURCE_REMOVE; // Remover el temporizador después de ejecutarlo
}


// Verificación de conexión a Internet
static gboolean check_internet_after_delay(gpointer data) {
    InternetWidgets *ui = (InternetWidgets *)data;

    SoupSession *session = soup_session_new();
    SoupMessage *msg = soup_message_new("HEAD", "http://www.google.com");

    GInputStream *response_stream = soup_session_send(session, msg, NULL, NULL);

    if (response_stream) {
        g_print("Conexión Exitosa\n");
        gtk_label_set_text(GTK_LABEL(ui->internet_label), "Espere...");
        gtk_widget_set_visible(ui->spinner, TRUE);
        g_object_unref(response_stream);

        // Ejecutar en un thread separado
        g_thread_new("fetch-language-timezone", fetch_language_and_timezone_thread, ui);

        // Función para planificar la actualización de la UI después del temporizador
        // Configurar un temporizador para planificar la actualización de la UI
        g_timeout_add_seconds(8, schedule_ui_update, ui);

    } else {
        g_print("Sin conexión a Internet\n");
        gtk_widget_set_visible(ui->no_internet_label, TRUE);
        gtk_widget_set_visible(ui->internet_label, FALSE);
        gtk_label_set_text(GTK_LABEL(ui->no_internet_label), "¡Sin conexión a Internet!");
        gtk_widget_set_visible(ui->spinner, FALSE);
    }

    g_object_unref(msg);
    g_object_unref(session);

    return G_SOURCE_REMOVE; // Remueve la función del bucle principal
}

void check_internet_connection(GtkWidget *internet_label, GtkWidget *spinner, GtkWidget *no_internet_label, GtkWidget *start_button)
{
    gtk_widget_set_visible(internet_label, TRUE);
    gtk_widget_set_visible(spinner, TRUE);
    gtk_widget_set_visible(no_internet_label, FALSE);
    gtk_widget_set_visible(start_button, FALSE);

    InternetWidgets *ui = g_new(InternetWidgets, 1);
    ui->internet_label = internet_label;
    ui->spinner = spinner;
    ui->no_internet_label = no_internet_label;
    ui->start_button = start_button;


    g_timeout_add_seconds(2, check_internet_after_delay, ui);

}



// Note: Functions related to ComboRow handling have been moved to page2.c
// This keeps the functionality properly encapsulated within the page modules
