#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

GtkWidget *window;
GtkWidget *file_list_box;
GtkWidget *folder_label;
char current_folder[1024] = "";

void populate_file_list(const char *folder) {
    // Clear previous items
    gtk_container_foreach(GTK_CONTAINER(file_list_box), (GtkCallback)gtk_widget_destroy, NULL);

    DIR *d = opendir(folder);
    if (!d) return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {

        // Skip hidden files
        if (dir->d_name[0] == '.') continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", folder, dir->d_name);

        struct stat st;
        if (stat(path, &st) != 0) continue;

        // Only show regular files
        if (!S_ISREG(st.st_mode)) continue;

        GtkWidget *label = gtk_label_new(dir->d_name);
        gtk_box_pack_start(GTK_BOX(file_list_box), label, FALSE, FALSE, 2);
    }

    closedir(d);
    gtk_widget_show_all(file_list_box);
}

void on_folder_selected(GtkFileChooserButton *chooser, gpointer user_data) {
    char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    if (!folder) return;

    strncpy(current_folder, folder, sizeof(current_folder));
    gtk_label_set_text(GTK_LABEL(folder_label), folder);
    populate_file_list(current_folder);
    g_free(folder);
}

void activate(GtkApplication *app, gpointer user_data) {
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Minimal File Explorer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    folder_label = gtk_label_new("Select a folder...");
    gtk_box_pack_start(GTK_BOX(vbox), folder_label, FALSE, FALSE, 5);

    GtkWidget *folder_btn = gtk_file_chooser_button_new("Select Folder", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    g_signal_connect(folder_btn, "file-set", G_CALLBACK(on_folder_selected), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), folder_btn, FALSE, FALSE, 5);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 5);

    file_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(scroll), file_list_box);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.example.minimalfileexplorer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
