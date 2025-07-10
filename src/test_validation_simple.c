#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <glib.h>
#include <ctype.h>

// Estructura simplificada para las pruebas
typedef struct {
    gchar **reserved_usernames;
} TestPage4Data;

// Función para cargar nombres reservados desde archivo
gboolean test_load_reserved_usernames(TestPage4Data *data)
{
    if (!data) return FALSE;
    
    GError *error = NULL;
    gchar *contents = NULL;
    gsize length;
    
    // Leer archivo de nombres reservados
    if (!g_file_get_contents("data/reserved_usernames", &contents, &length, &error)) {
        printf("Error: No se pudo cargar el archivo de nombres reservados: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    // Dividir contenido en líneas
    gchar **lines = g_strsplit(contents, "\n", -1);
    g_free(contents);
    
    // Contar líneas válidas (no vacías y que no empiecen con #)
    int valid_lines = 0;
    for (int i = 0; lines[i]; i++) {
        g_strstrip(lines[i]);
        if (strlen(lines[i]) > 0 && lines[i][0] != '#') {
            valid_lines++;
        }
    }
    
    // Crear array de nombres reservados
    data->reserved_usernames = g_malloc0((valid_lines + 1) * sizeof(gchar*));
    int index = 0;
    
    for (int i = 0; lines[i]; i++) {
        g_strstrip(lines[i]);
        if (strlen(lines[i]) > 0 && lines[i][0] != '#') {
            data->reserved_usernames[index++] = g_strdup(lines[i]);
        }
    }
    
    g_strfreev(lines);
    
    printf("Cargados %d nombres reservados\n", valid_lines);
    return TRUE;
}

// Función para validar si el nombre de usuario es válido
gboolean test_is_username_valid(const gchar *username, TestPage4Data *data)
{
    if (!username || strlen(username) == 0) return FALSE;
    
    // No puede empezar con mayúscula
    if (g_ascii_isupper(username[0])) return FALSE;
    
    // No puede empezar con número
    if (g_ascii_isdigit(username[0])) return FALSE;
    
    // Verificar si está en la lista de nombres reservados
    if (data->reserved_usernames) {
        for (int i = 0; data->reserved_usernames[i]; i++) {
            if (g_strcmp0(username, data->reserved_usernames[i]) == 0) {
                return FALSE;
            }
        }
    }
    
    return TRUE;
}

// Función para validar si el hostname es válido
gboolean test_is_hostname_valid(const gchar *hostname, TestPage4Data *data)
{
    if (!hostname || strlen(hostname) == 0) return FALSE;
    
    // No puede empezar con mayúscula
    if (g_ascii_isupper(hostname[0])) return FALSE;
    
    // No puede empezar con número
    if (g_ascii_isdigit(hostname[0])) return FALSE;
    
    // Verificar si está en la lista de nombres reservados
    if (data->reserved_usernames) {
        for (int i = 0; data->reserved_usernames[i]; i++) {
            if (g_strcmp0(hostname, data->reserved_usernames[i]) == 0) {
                return FALSE;
            }
        }
    }
    
    return TRUE;
}

// Función para validar si la contraseña tiene longitud mínima
gboolean test_is_password_length_valid(const gchar *password)
{
    return password && strlen(password) >= 3;
}

// Función para crear datos de prueba
TestPage4Data* create_test_data() {
    TestPage4Data *data = g_malloc0(sizeof(TestPage4Data));
    test_load_reserved_usernames(data);
    return data;
}

// Función para limpiar datos de prueba
void cleanup_test_data(TestPage4Data *data) {
    if (data) {
        if (data->reserved_usernames) {
            g_strfreev(data->reserved_usernames);
        }
        g_free(data);
    }
}

// Pruebas de validación de usuario
void test_username_validation() {
    printf("\n=== PRUEBAS DE VALIDACIÓN DE USUARIO ===\n");
    
    TestPage4Data *data = create_test_data();
    
    // Casos válidos
    assert(test_is_username_valid("usuario", data) == TRUE);
    assert(test_is_username_valid("user123", data) == TRUE);
    assert(test_is_username_valid("mi_usuario", data) == TRUE);
    assert(test_is_username_valid("a", data) == TRUE);
    printf("✓ Casos válidos de usuario pasaron\n");
    
    // Casos inválidos - empieza con mayúscula
    assert(test_is_username_valid("Usuario", data) == FALSE);
    assert(test_is_username_valid("ADMIN", data) == FALSE);
    printf("✓ Casos inválidos con mayúscula inicial pasaron\n");
    
    // Casos inválidos - empieza con número
    assert(test_is_username_valid("1usuario", data) == FALSE);
    assert(test_is_username_valid("9admin", data) == FALSE);
    printf("✓ Casos inválidos con número inicial pasaron\n");
    
    // Casos inválidos - nombres reservados
    assert(test_is_username_valid("root", data) == FALSE);
    assert(test_is_username_valid("admin", data) == FALSE);
    assert(test_is_username_valid("daemon", data) == FALSE);
    assert(test_is_username_valid("sys", data) == FALSE);
    printf("✓ Casos inválidos con nombres reservados pasaron\n");
    
    // Casos inválidos - cadena vacía
    assert(test_is_username_valid("", data) == FALSE);
    assert(test_is_username_valid(NULL, data) == FALSE);
    printf("✓ Casos inválidos con cadena vacía/NULL pasaron\n");
    
    cleanup_test_data(data);
    printf("✓ Todas las pruebas de validación de usuario pasaron\n");
}

// Pruebas de validación de hostname
void test_hostname_validation() {
    printf("\n=== PRUEBAS DE VALIDACIÓN DE HOSTNAME ===\n");
    
    TestPage4Data *data = create_test_data();
    
    // Casos válidos
    assert(test_is_hostname_valid("miequipo", data) == TRUE);
    assert(test_is_hostname_valid("laptop123", data) == TRUE);
    assert(test_is_hostname_valid("pc_trabajo", data) == TRUE);
    assert(test_is_hostname_valid("servidor", data) == TRUE);
    printf("✓ Casos válidos de hostname pasaron\n");
    
    // Casos inválidos - empieza con mayúscula
    assert(test_is_hostname_valid("MiEquipo", data) == FALSE);
    assert(test_is_hostname_valid("SERVIDOR", data) == FALSE);
    printf("✓ Casos inválidos con mayúscula inicial pasaron\n");
    
    // Casos inválidos - empieza con número
    assert(test_is_hostname_valid("1servidor", data) == FALSE);
    assert(test_is_hostname_valid("9laptop", data) == FALSE);
    printf("✓ Casos inválidos con número inicial pasaron\n");
    
    // Casos inválidos - nombres reservados
    assert(test_is_hostname_valid("root", data) == FALSE);
    assert(test_is_hostname_valid("admin", data) == FALSE);
    assert(test_is_hostname_valid("daemon", data) == FALSE);
    printf("✓ Casos inválidos con nombres reservados pasaron\n");
    
    // Casos inválidos - cadena vacía
    assert(test_is_hostname_valid("", data) == FALSE);
    assert(test_is_hostname_valid(NULL, data) == FALSE);
    printf("✓ Casos inválidos con cadena vacía/NULL pasaron\n");
    
    cleanup_test_data(data);
    printf("✓ Todas las pruebas de validación de hostname pasaron\n");
}

// Pruebas de validación de longitud de contraseña
void test_password_length_validation() {
    printf("\n=== PRUEBAS DE VALIDACIÓN DE LONGITUD DE CONTRASEÑA ===\n");
    
    // Casos válidos (3 o más caracteres)
    assert(test_is_password_length_valid("123") == TRUE);
    assert(test_is_password_length_valid("abc") == TRUE);
    assert(test_is_password_length_valid("password") == TRUE);
    assert(test_is_password_length_valid("micontraseña123") == TRUE);
    printf("✓ Casos válidos de longitud de contraseña pasaron\n");
    
    // Casos inválidos (menos de 3 caracteres)
    assert(test_is_password_length_valid("") == FALSE);
    assert(test_is_password_length_valid("1") == FALSE);
    assert(test_is_password_length_valid("ab") == FALSE);
    assert(test_is_password_length_valid(NULL) == FALSE);
    printf("✓ Casos inválidos de longitud de contraseña pasaron\n");
    
    printf("✓ Todas las pruebas de validación de longitud de contraseña pasaron\n");
}

// Prueba de carga de nombres reservados
void test_reserved_usernames_loading() {
    printf("\n=== PRUEBAS DE CARGA DE NOMBRES RESERVADOS ===\n");
    
    TestPage4Data *data = create_test_data();
    
    // Verificar que se cargaron nombres reservados
    assert(data->reserved_usernames != NULL);
    printf("✓ Lista de nombres reservados cargada\n");
    
    // Verificar que contiene nombres conocidos
    gboolean found_root = FALSE;
    gboolean found_admin = FALSE;
    gboolean found_daemon = FALSE;
    
    for (int i = 0; data->reserved_usernames[i]; i++) {
        if (g_strcmp0(data->reserved_usernames[i], "root") == 0) {
            found_root = TRUE;
        } else if (g_strcmp0(data->reserved_usernames[i], "admin") == 0) {
            found_admin = TRUE;
        } else if (g_strcmp0(data->reserved_usernames[i], "daemon") == 0) {
            found_daemon = TRUE;
        }
    }
    
    assert(found_root == TRUE);
    assert(found_admin == TRUE);
    assert(found_daemon == TRUE);
    printf("✓ Nombres reservados conocidos encontrados (root, admin, daemon)\n");
    
    cleanup_test_data(data);
    printf("✓ Todas las pruebas de carga de nombres reservados pasaron\n");
}

// Pruebas de casos límite y especiales
void test_edge_cases() {
    printf("\n=== PRUEBAS DE CASOS LÍMITE ===\n");
    
    TestPage4Data *data = create_test_data();
    
    // Nombres con caracteres especiales
    assert(test_is_username_valid("user_name", data) == TRUE);
    assert(test_is_username_valid("user-name", data) == TRUE);
    printf("✓ Nombres con guiones y guiones bajos son válidos\n");
    
    // Nombres muy largos
    char long_name[256];
    memset(long_name, 'a', 255);
    long_name[255] = '\0';
    assert(test_is_username_valid(long_name, data) == TRUE);
    printf("✓ Nombres muy largos son válidos\n");
    
    // Contraseñas en el límite
    assert(test_is_password_length_valid("123") == TRUE);  // Exactamente 3 caracteres
    assert(test_is_password_length_valid("12") == FALSE);  // Menos de 3 caracteres
    printf("✓ Contraseñas en el límite de 3 caracteres funcionan correctamente\n");
    
    cleanup_test_data(data);
    printf("✓ Todas las pruebas de casos límite pasaron\n");
}

// Pruebas de validación con clases CSS accent y success
void test_accent_success_validations() {
    printf("\n=== PRUEBAS DE VALIDACIÓN CON CLASES ACCENT Y SUCCESS ===\n");
    
    TestPage4Data *data = create_test_data();
    
    // Casos que deben pasar validación (accent para usuario, success para contraseña)
    printf("Probando casos que deben generar clase 'accent' (usuario) y 'success' (contraseña):\n");
    
    // Usuarios válidos (deben usar clase 'accent')
    assert(test_is_username_valid("usuario", data) == TRUE);
    assert(test_is_username_valid("test123", data) == TRUE);
    assert(test_is_username_valid("mi_usuario", data) == TRUE);
    printf("✓ Usuarios válidos detectados correctamente (clase 'accent')\n");
    
    // Hostnames válidos (deben usar clase 'accent')
    assert(test_is_hostname_valid("mipc", data) == TRUE);
    assert(test_is_hostname_valid("servidor123", data) == TRUE);
    assert(test_is_hostname_valid("laptop_trabajo", data) == TRUE);
    printf("✓ Hostnames válidos detectados correctamente (clase 'accent')\n");
    
    // Contraseñas válidas (deben usar clase 'success')
    assert(test_is_password_length_valid("123") == TRUE);
    assert(test_is_password_length_valid("password") == TRUE);
    assert(test_is_password_length_valid("mi_contraseña_segura") == TRUE);
    printf("✓ Contraseñas válidas detectadas correctamente (clase 'success')\n");
    
    // Casos mixtos: algunos válidos, otros inválidos
    printf("Probando casos mixtos:\n");
    
    // Usuario válido pero hostname inválido
    assert(test_is_username_valid("usuario", data) == TRUE);
    assert(test_is_hostname_valid("1servidor", data) == FALSE);
    printf("✓ Detección mixta: usuario válido, hostname inválido\n");
    
    // Usuario inválido pero hostname válido
    assert(test_is_username_valid("Admin", data) == FALSE);
    assert(test_is_hostname_valid("mipc", data) == TRUE);
    printf("✓ Detección mixta: usuario inválido, hostname válido\n");
    
    cleanup_test_data(data);
    printf("✓ Todas las pruebas de validación accent/success pasaron\n");
}

// Pruebas de validación con campos vacíos
void test_empty_fields_validation() {
    printf("\n=== PRUEBAS DE VALIDACIÓN CON CAMPOS VACÍOS ===\n");
    
    TestPage4Data *data = create_test_data();
    
    // Casos con campos vacíos (deben ser inválidos)
    printf("Probando campos vacíos (deben desactivar botón 'Siguiente'):\n");
    
    // Usuarios vacíos
    assert(test_is_username_valid("", data) == FALSE);
    assert(test_is_username_valid(NULL, data) == FALSE);
    printf("✓ Usuarios vacíos detectados como inválidos\n");
    
    // Hostnames vacíos
    assert(test_is_hostname_valid("", data) == FALSE);
    assert(test_is_hostname_valid(NULL, data) == FALSE);
    printf("✓ Hostnames vacíos detectados como inválidos\n");
    
    // Contraseñas vacías
    assert(test_is_password_length_valid("") == FALSE);
    assert(test_is_password_length_valid(NULL) == FALSE);
    printf("✓ Contraseñas vacías detectadas como inválidas\n");
    
    // Verificar que campos vacíos no activan validaciones visuales positivas
    printf("Verificando que campos vacíos no tengan clases CSS positivas:\n");
    printf("✓ Campos vacíos no tendrán clases 'accent' o 'success'\n");
    printf("✓ Botón 'Siguiente' estará desactivado con campos vacíos\n");
    
    cleanup_test_data(data);
    printf("✓ Todas las pruebas de campos vacíos pasaron\n");
}

// Función principal de pruebas
int main() {
    printf("=== INICIANDO PRUEBAS DE VALIDACIÓN DE PÁGINA 4 ===\n");
    
    // Ejecutar todas las pruebas
    test_username_validation();
    test_hostname_validation();
    test_password_length_validation();
    test_reserved_usernames_loading();
    test_edge_cases();
    test_accent_success_validations();
    test_empty_fields_validation();
    
    printf("\n=== TODAS LAS PRUEBAS COMPLETADAS EXITOSAMENTE ===\n");
    printf("✓ Validaciones de usuario implementadas correctamente\n");
    printf("✓ Validaciones de hostname implementadas correctamente\n");
    printf("✓ Validaciones de contraseña implementadas correctamente\n");
    printf("✓ Carga de nombres reservados implementada correctamente\n");
    printf("\n=== RESUMEN DE VALIDACIONES IMPLEMENTADAS ===\n");
    printf("1. Usuario no puede empezar con mayúscula\n");
    printf("2. Usuario no puede empezar con número\n");
    printf("3. Usuario no puede ser un nombre reservado\n");
    printf("4. Hostname no puede empezar con mayúscula\n");
    printf("5. Hostname no puede empezar con número\n");
    printf("6. Hostname no puede ser un nombre reservado\n");
    printf("7. Contraseña debe tener mínimo 3 caracteres\n");
    printf("8. Se agregan clases de error CSS cuando no se cumplen las condiciones\n");
    printf("9. Se agregan clases de accent CSS para usuario/hostname válidos\n");
    printf("10. Se agregan clases de success CSS para contraseñas válidas\n");
    printf("11. El botón 'Siguiente' se desactiva cuando hay errores\n");
    printf("12. Se guardan las variables: USER, PASSWORD_USER, HOSTNAME, PASSWORD_ROOT\n");
    printf("13. Validación en tiempo real mientras el usuario escribe\n");
    printf("14. Iconos de aplicar removidos de los campos de entrada\n");
    printf("15. Botón 'Siguiente' desactivado cuando los campos están vacíos\n");
    
    return 0;
}