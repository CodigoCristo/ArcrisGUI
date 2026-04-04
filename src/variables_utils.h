#ifndef VARIABLES_UTILS_H
#define VARIABLES_UTILS_H

#include <glib.h>

#define VARIABLES_FILE_PATH "./data/bash/variables.sh"

/* Upsert variable in file content (in-memory).
 * If the variable exists, the line is updated in place.
 * If not found, it is appended at the end. */
void vars_upsert(GString *content, const gchar *name, const gchar *value);

/* Like vars_upsert, but if the variable is not found it is inserted on the
 * line immediately after the line that starts with after_name=.
 * Falls back to appending at the end if after_name is not present either. */
void vars_upsert_after(GString *content, const gchar *name, const gchar *value,
                       const gchar *after_name);

/* Like vars_upsert_after, but when inserting a new variable (not already
 * present) it also prepends "# comment\n" before the variable line.
 * When the variable already exists it is updated in place without touching
 * whatever comment line precedes it. */
void vars_upsert_after_with_comment(GString *content, const gchar *name,
                                    const gchar *value, const gchar *after_name,
                                    const gchar *comment);

/* Remove a variable line from file content (in-memory). */
void vars_remove(GString *content, const gchar *name);

/* Read VARIABLES_FILE_PATH into a GString, call apply(content, user_data),
 * then write the result back.  Returns FALSE on I/O error. */
gboolean vars_update(void (*apply)(GString *, gpointer), gpointer user_data);

/* Remove consecutive trailing blank lines, leaving at most one final newline. */
void vars_trim_trailing_newlines(GString *content);

#endif /* VARIABLES_UTILS_H */
