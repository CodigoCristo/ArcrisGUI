#ifndef I18N_H
#define I18N_H

/* Supported application languages */
typedef enum {
    LANG_ES = 0,
    LANG_EN = 1,
    LANG_RU = 2,
    LANG_PT = 3,
    LANG_FR = 4,
    LANG_DE = 5
} AppLang;

/* Set / get the current language */
void     i18n_set_lang(AppLang lang);
AppLang  i18n_get_lang(void);

/* Return the string for the current language.
 * Always returns a non-NULL pointer (falls back to es).
 * For PT/FR/DE, looks up the translation table using the ES string as key.
 * Falls back to EN if the key is not found in the table. */
const char* i18n_t(const char *es, const char *en, const char *ru);

#endif /* I18N_H */
