#ifndef WINDOW_REPOS_H
#define WINDOW_REPOS_H

#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct _WindowReposData {
    GtkWindow *window;
    GtkBuilder *builder;

    GtkButton *close_button;
    GtkButton *save_button;

    AdwSwitchRow *chaotic_aur_switch;
    AdwSwitchRow *archlinuxcn_switch;
    AdwSwitchRow *cachyos_switch;

    GtkToggleButton *auto_button;
    GtkToggleButton *manual_button;
    GtkListBoxRow *repos_manual_row;
    GtkListBoxRow *repos_textview_row;
    GtkTextView *mirrorlist_textview;

    gboolean is_initialized;
} WindowReposData;

WindowReposData* window_repos_new(void);
void window_repos_init(WindowReposData *data);
void window_repos_show(WindowReposData *data, GtkWindow *parent);
WindowReposData* window_repos_get_instance(void);
void window_repos_init_defaults(void);

void on_repos_close_button_clicked(GtkButton *button, gpointer user_data);
void on_repos_save_button_clicked(GtkButton *button, gpointer user_data);

#endif /* WINDOW_REPOS_H */
