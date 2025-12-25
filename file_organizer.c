#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define TRASH_FOLDER "Trash"
#define TAG_FILE "tags.txt"
GtkWidget *window;
GtkWidget *file_list_box;
GtkWidget *search_entry;
GtkWidget *folder_label;
GtkWidget *filter_combo;
char current_folder[1024] = "";
char search_text[256] = "";
char filter_ext[50] = ""; 
void ensure_trash() {
    struct stat st = {0};
    if (stat(TRASH_FOLDER, &st) == -1) {
        mkdir(TRASH_FOLDER);
    }
}
void save_tag(const char *filename, const char *tag) {   
    FILE *fp = fopen(TAG_FILE, "a");
    if (!fp) return;
    fprintf(fp, "%s|%s\n", filename, tag);
    fclose(fp);
}

void on_add_tag(GtkButton *btn, gpointer user_data) { 
    const char *filename = (const char *)user_data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Add Tag", GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        "Save", GTK_RESPONSE_OK,
        "Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );

    GtkWidget *entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(
        gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *tag = gtk_entry_get_text(GTK_ENTRY(entry));
        save_tag(filename, tag);
    }

    gtk_widget_destroy(dialog);
}
void populate_file_list(const char *folder) {
    gtk_container_foreach(GTK_CONTAINER(file_list_box), (GtkCallback)gtk_widget_destroy, NULL);
    DIR *d = opendir(folder);
    if (!d) return;
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        if (search_text[0] != '\0') {
            if (!g_strrstr(dir->d_name, search_text))
                continue;
        }
        if (filter_ext[0] != '\0' && strcmp(filter_ext, "All") != 0) {
            if (!g_str_has_suffix(dir->d_name, filter_ext))
                continue;
        }
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
        GtkWidget *btn_tag = gtk_button_new_with_label("Tag");
        g_signal_connect(btn_tag, "clicked",                
                         G_CALLBACK(on_add_tag),           
                         g_strdup(dir->d_name));        
        gtk_box_pack_start(GTK_BOX(hbox), btn_tag, FALSE, FALSE, 0); 
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
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    strncpy(search_text, text, sizeof(search_text));
    populate_file_list(current_folder);  
}
void on_filter_changed(GtkComboBoxText *combo, gpointer user_data) {
    const char *ext = gtk_combo_box_text_get_active_text(combo);
    if (ext)
        strncpy(filter_ext, ext, sizeof(filter_ext));
    populate_file_list(current_folder);  
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
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search file..."); 
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);
    filter_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(filter_combo), "All");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(filter_combo), ".txt");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(filter_combo), ".c");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(filter_combo), ".pdf");
    gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo), 0);
    g_signal_connect(filter_combo, "changed",
                     G_CALLBACK(on_filter_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), filter_combo, FALSE, FALSE, 0);
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
