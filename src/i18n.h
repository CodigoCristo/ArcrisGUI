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
 * Looks up the ES string as key in the translation table.
 * Falls back to ES if the key is not found.
 *
 * Accepts 1, 2, or 3 arguments — extra args are ignored during migration:
 *   i18n_t("es")              — new single-arg form
 *   i18n_t("es", "en", "ru") — old form, en/ru ignored (already in table) */
const char* i18n_lookup(const char *es);
#define i18n_t(es, ...) i18n_lookup(es)

#endif /* I18N_H */
