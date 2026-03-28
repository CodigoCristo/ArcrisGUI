#include "variables_utils.h"
#include "config.h"

void vars_upsert(GString *content, const gchar *name, const gchar *value)
{
    gchar **lines = g_strsplit(content->str, "\n", -1);
    GString *result = g_string_new("");
    gboolean found = FALSE;
    gchar *needle = g_strdup_printf("%s=", name);

    for (int i = 0; lines[i] != NULL; i++) {
        gchar *stripped = g_strstrip(g_strdup(lines[i]));
        if (g_str_has_prefix(stripped, needle)) {
            g_string_append_printf(result, "%s=\"%s\"\n", name, value);
            found = TRUE;
        } else {
            g_string_append_printf(result, "%s\n", lines[i]);
        }
        g_free(stripped);
    }

    if (!found)
        g_string_append_printf(result, "%s=\"%s\"\n", name, value);

    g_free(needle);
    g_strfreev(lines);
    g_string_assign(content, result->str);
    g_string_free(result, TRUE);
}

void vars_upsert_after(GString *content, const gchar *name, const gchar *value,
                       const gchar *after_name)
{
    gchar **lines = g_strsplit(content->str, "\n", -1);
    GString *result = g_string_new("");
    gboolean found = FALSE;
    gchar *needle       = g_strdup_printf("%s=", name);
    gchar *after_needle = g_strdup_printf("%s=", after_name);

    for (int i = 0; lines[i] != NULL; i++) {
        gchar *stripped = g_strstrip(g_strdup(lines[i]));
        if (g_str_has_prefix(stripped, needle)) {
            if (!found) {
                /* Primera vez que vemos la variable: actualizar en el lugar. */
                g_string_append_printf(result, "%s=\"%s\"\n", name, value);
                found = TRUE;
            }
            /* Si found ya es TRUE la variable ya fue insertada tras el ancla;
             * descartamos esta línea antigua para no duplicarla. */
        } else {
            g_string_append_printf(result, "%s\n", lines[i]);
            if (!found && g_str_has_prefix(stripped, after_needle)) {
                g_string_append_printf(result, "%s=\"%s\"\n", name, value);
                found = TRUE;
            }
        }
        g_free(stripped);
    }

    if (!found)
        g_string_append_printf(result, "%s=\"%s\"\n", name, value);

    g_free(needle);
    g_free(after_needle);
    g_strfreev(lines);
    g_string_assign(content, result->str);
    g_string_free(result, TRUE);
}

void vars_upsert_after_with_comment(GString *content, const gchar *name,
                                    const gchar *value, const gchar *after_name,
                                    const gchar *comment)
{
    /* Si la variable ya existe, actualizar en lugar: el comentario que está
     * encima en el archivo se conserva intacto y no se crea ningún duplicado. */
    gchar *needle = g_strdup_printf("%s=", name);
    gboolean already_exists = strstr(content->str, needle) != NULL;
    g_free(needle);

    if (already_exists) {
        vars_upsert(content, name, value);
        return;
    }

    /* La variable no existe aún: insertarla con su comentario justo tras el ancla. */
    gchar **lines = g_strsplit(content->str, "\n", -1);
    GString *result = g_string_new("");
    gboolean inserted = FALSE;
    gchar *after_needle = g_strdup_printf("%s=", after_name);

    for (int i = 0; lines[i] != NULL; i++) {
        gchar *stripped = g_strstrip(g_strdup(lines[i]));
        g_string_append_printf(result, "%s\n", lines[i]);
        if (!inserted && g_str_has_prefix(stripped, after_needle)) {
            if (comment)
                g_string_append_printf(result, "\n# %s\n", comment);
            g_string_append_printf(result, "%s=\"%s\"\n", name, value);
            inserted = TRUE;
        }
        g_free(stripped);
    }

    if (!inserted) {
        if (comment)
            g_string_append_printf(result, "\n# %s\n", comment);
        g_string_append_printf(result, "%s=\"%s\"\n", name, value);
    }

    g_free(after_needle);
    g_strfreev(lines);
    g_string_assign(content, result->str);
    g_string_free(result, TRUE);
}

void vars_remove(GString *content, const gchar *name)
{
    gchar **lines = g_strsplit(content->str, "\n", -1);
    GString *result = g_string_new("");
    gchar *needle = g_strdup_printf("%s=", name);

    for (int i = 0; lines[i] != NULL; i++) {
        gchar *stripped = g_strstrip(g_strdup(lines[i]));
        if (!g_str_has_prefix(stripped, needle))
            g_string_append_printf(result, "%s\n", lines[i]);
        g_free(stripped);
    }

    g_free(needle);
    g_strfreev(lines);
    g_string_assign(content, result->str);
    g_string_free(result, TRUE);
}

void vars_trim_trailing_newlines(GString *content)
{
    /* Quita saltos de línea consecutivos al final, dejando como máximo uno. */
    while (content->len >= 2 &&
           content->str[content->len - 1] == '\n' &&
           content->str[content->len - 2] == '\n') {
        g_string_truncate(content, content->len - 1);
    }
}

gboolean vars_update(void (*apply)(GString *, gpointer), gpointer user_data)
{
    GError *error = NULL;
    gchar *file_content = NULL;

    if (!g_file_get_contents(VARIABLES_FILE_PATH, &file_content, NULL, &error)) {
        LOG_ERROR("No se pudo leer variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }

    GString *content = g_string_new(file_content);
    g_free(file_content);

    apply(content, user_data);
    vars_trim_trailing_newlines(content);

    gboolean ok = g_file_set_contents(VARIABLES_FILE_PATH, content->str, -1, &error);
    if (!ok) {
        LOG_ERROR("Error guardando variables.sh: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    }

    g_string_free(content, TRUE);
    return ok;
}
