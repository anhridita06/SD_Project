// file_explorer_win_fixed.c
// Compile with:
// gcc file_explorer_win_fixed.c -o file_explorer.exe $(pkg-config --cflags --libs gtk+-3.0 gdk-pixbuf-2.0) -Wall

#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#define MAX_PATH_LEN 1024

GtkWidget *window;
GtkWidget *file_container;
GtkWidget *scroll;
GtkWidget *folder_label;
GtkWidget *search_entry;
char current_folder[MAX_PATH_LEN] = "";

typedef struct {
    char name[1024];
    char path[1024];
    off_t size;
    time_t mtime;
    gboolean is_file;
} FileItem;

// ------------------- utilities -------------------
GArray* get_files_in_folder(const char *folder, const char *filter) {
    GArray *arr = g_array_new(TRUE, TRUE, sizeof(FileItem));
    DIR *d = opendir(folder);
    if (!d) return arr;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name,"..")==0) continue;

        FileItem fi;
        snprintf(fi.path, sizeof(fi.path), "%s%s%s", folder, PATH_SEP, entry->d_name);
        strncpy(fi.name, entry->d_name, sizeof(fi.name)-1); fi.name[sizeof(fi.name)-1]=0;

        struct stat st;
        if (stat(fi.path, &st) != 0) continue;
        fi.size = st.st_size;
        fi.mtime = st.st_mtime;
#ifdef _WIN32
        fi.is_file = ((st.st_mode & _S_IFREG) != 0);
#else
        fi.is_file = S_ISREG(st.st_mode);
#endif

        if (filter && strlen(filter) > 0) {
            char lname[1024], lfilter[1024];
            strncpy(lname, fi.name, sizeof(lname)-1); lname[sizeof(lname)-1]=0;
            strncpy(lfilter, filter, sizeof(lfilter)-1); lfilter[sizeof(lfilter)-1]=0;
            for (char *p = lname; *p; ++p) *p = tolower(*p);
            for (char *p = lfilter; *p; ++p) *p = tolower(*p);
            if (!strstr(lname, lfilter)) continue;
        }

        g_array_append_val(arr, fi);
    }
    closedir(d);
    return arr;
}

// ------------------- file row -------------------
GtkWidget* make_file_row(FileItem *fi) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl = gtk_label_new(fi->name);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

    char meta[256]; 
    struct tm mt;
#ifdef _WIN32
    localtime_s(&mt, &fi->mtime);
#else
    localtime_r(&fi->mtime, &mt);
#endif
    char datestr[64]; strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M", &mt);
    snprintf(meta, sizeof(meta), "%s — %lld bytes", datestr, (long long)fi->size);
    GtkWidget *meta_lbl = gtk_label_new(meta);
    gtk_label_set_xalign(GTK_LABEL(meta_lbl), 0.0);

    gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), meta_lbl, FALSE, FALSE, 2);

    FileItem *copy = g_new(FileItem, 1); *copy = *fi;
    gtk_widget_add_events(hbox, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(hbox, "button-press-event", G_CALLBACK(
        +[](GtkWidget* w, GdkEventButton* e, gpointer ud) -> gboolean {
            if (e->type == GDK_2BUTTON_PRESS && e->button == 1) {
                FileItem *f = (FileItem*)ud;
                char cmd[2048];
#ifdef _WIN32
                snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", f->path);
#else
                snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", f->path);
#endif
                system(cmd);
                return TRUE;
            }
            return FALSE;
        }
    ), copy);

    return hbox;
}

// ------------------- refresh view -------------------
void refresh_file_view() {
    if (file_container) { gtk_widget_destroy(file_container); file_container = NULL; }

    const char *filter = gtk_entry_get_text(GTK_ENTRY(search_entry));
    GArray *arr = get_files_in_folder(current_folder, filter);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    for (guint i = 0; i < arr->len; i++) {
        FileItem *fi = &g_array_index(arr, FileItem, i);
        GtkWidget *row = make_file_row(fi);
        gtk_box_pack_start(GTK_BOX(vbox), row, FALSE, FALSE, 0);
    }

    file_container = vbox;
    gtk_container_add(GTK_CONTAINER(scroll), file_container);
    gtk_widget_show_all(scroll);
    g_array_free(arr, TRUE);
}

// ------------------- folder selection -------------------
void on_folder_selected(GtkFileChooserButton *chooser, gpointer d) {
    char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    if (!folder) return;
    strncpy(current_folder, folder, sizeof(current_folder)-1); current_folder[sizeof(current_folder)-1]=0;
    gtk_label_set_text(GTK_LABEL(folder_label), current_folder);
    g_free(folder);
    refresh_file_view();
}

// ------------------- activate -------------------
void activate(GtkApplication *app, gpointer d) {
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Explorer");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    GtkWidget *top_h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    folder_label = gtk_label_new("Select a folder...");
    gtk_box_pack_start(GTK_BOX(top_h), folder_label, TRUE, TRUE, 6);

    GtkWidget *folder_btn = gtk_file_chooser_button_new("Select Folder", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    g_signal_connect(folder_btn, "file-set", G_CALLBACK(on_folder_selected), NULL);
    gtk_box_pack_start(GTK_BOX(top_h), folder_btn, FALSE, FALSE, 6);

    gtk_box_pack_start(GTK_BOX(main_vbox), top_h, FALSE, FALSE, 6);

    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search...");
    g_signal_connect(search_entry, "changed", G_CALLBACK(refresh_file_view), NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), search_entry, FALSE, FALSE, 6);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(main_vbox), scroll, TRUE, TRUE, 6);

    file_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(scroll), file_container);

    gtk_widget_show_all(window);
}

// ------------------- main -------------------
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.example.fileexplorer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
