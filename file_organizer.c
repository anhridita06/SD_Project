#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define TRASH_FOLDER "Trash"
GtkWidget *window;
GtkWidget *file_list_box;
GtkWidget *search_entry;
GtkWidget *folder_label;
char current_folder[1024] = "";
void ensure_trash() {
    struct stat st = {0};
    if (stat(TRASH_FOLDER, &st) == -1) {
        mkdir(TRASH_FOLDER);
    }
}
void populate_file_list(const char *folder) {
    gtk_container_foreach(GTK_CONTAINER(file_list_box), (GtkCallback)gtk_widget_destroy, NULL);
    DIR *d = opendir(folder);
    if (!d) return;
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", folder, dir->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (!S_ISREG(st.st_mode)) continue;
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget *label = gtk_label_new(dir->d_name);
        gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
        GtkWidget *btn_open = gtk_button_new_with_label("Open");
        gtk_box_pack_start(GTK_BOX(hbox), btn_open, FALSE, FALSE, 0);
        GtkWidget *btn_delete = gtk_button_new_with_label("Delete");
        gtk_box_pack_start(GTK_BOX(hbox), btn_delete, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(file_list_box), hbox, FALSE, FALSE, 0);
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
void on_search_changed(GtkEditable *editable, gpointer user_data) {
}
void activate(GtkApplication *app, gpointer user_data) {
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Organizer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    folder_label = gtk_label_new("Select folder...");
    gtk_box_pack_start(GTK_BOX(vbox), folder_label, FALSE, FALSE, 0);
    GtkWidget *folder_btn = gtk_file_chooser_button_new("Select Folder", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    g_signal_connect(folder_btn, "file-set", G_CALLBACK(on_folder_selected), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), folder_btn, FALSE, FALSE, 0);
    search_entry = gtk_entry_new();
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    file_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(scroll), file_list_box);
    ensure_trash();
    gtk_widget_show_all(window);
}
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.example.fileorganizer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}