#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>  // For MoveFile, DeleteFile

GtkWidget *window;
GtkWidget *tree_view;
GtkWidget *image_preview;
GtkTreeStore *tree_store;
GtkWidget *grid_button;
GtkWidget *list_button;

// Function to populate directory recursively
void populate_tree_store(const char *path, GtkTreeIter *parent_iter) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        GtkTreeIter iter;
        gtk_tree_store_append(tree_store, &iter, parent_iter);
        gtk_tree_store_set(tree_store, &iter,
                           0, entry->d_name,
                           1, g_strdup(path),
                           -1);

       char full_path[1024];
snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

DWORD attr = GetFileAttributes(full_path);
if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
    populate_tree_store(full_path, &iter);
}

    }
    closedir(dir);
}

// Callback to show image preview
void on_tree_selection_changed(GtkTreeSelection *selection, gpointer data) {
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *filename;
        gchar *filepath;
        gtk_tree_model_get(model, &iter, 0, &filename, 1, &filepath, -1);

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", filepath, filename);

        // Show image preview
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(fullpath, 300, 300, TRUE, NULL);
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_preview), pixbuf);
            g_object_unref(pixbuf);
        }

        g_free(filename);
        g_free(filepath);
    }
}

// Drag & Drop setup
void setup_drag_and_drop(GtkWidget *widget) {
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_MOVE);
}

// Batch operations: Rename
void rename_selected_file(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection = GTK_TREE_SELECTION(data);
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *filename;
        gchar *filepath;
        gtk_tree_model_get(model, &iter, 0, &filename, 1, &filepath, -1);

        GtkWidget *dialog = gtk_dialog_new_with_buttons("Rename File",
                                                        GTK_WINDOW(window),
                                                        GTK_DIALOG_MODAL,
                                                        "_OK", GTK_RESPONSE_OK,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        NULL);
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), filename);
        gtk_container_add(GTK_CONTAINER(content_area), entry);
        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
            const gchar *new_name = gtk_entry_get_text(GTK_ENTRY(entry));
            char old_path[1024], new_path[1024];
            snprintf(old_path, sizeof(old_path), "%s\\%s", filepath, filename);
            snprintf(new_path, sizeof(new_path), "%s\\%s", filepath, new_name);
            MoveFile(old_path, new_path);
            gtk_tree_store_set(tree_store, &iter, 0, new_name, -1);
        }
        gtk_widget_destroy(dialog);

        g_free(filename);
        g_free(filepath);
    }
}

// Batch operations: Delete
void delete_selected_file(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection = GTK_TREE_SELECTION(data);
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *filename;
        gchar *filepath;
        gtk_tree_model_get(model, &iter, 0, &filename, 1, &filepath, -1);

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s\\%s", filepath, filename);
        if (MessageBox(NULL, "Are you sure you want to delete?", "Delete File", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            DeleteFile(fullpath);
            gtk_tree_store_remove(tree_store, &iter);
        }

        g_free(filename);
        g_free(filepath);
    }
}

// View mode buttons (placeholders)
void switch_to_grid(GtkWidget *widget, gpointer data) {
    g_print("Grid view mode selected\n");
    // Implement GtkFlowBox or GtkIconView later
}

void switch_to_list(GtkWidget *widget, gpointer data) {
    g_print("List view mode selected\n");
    // Implement GtkTreeView list view
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
    gtk_window_set_title(GTK_WINDOW(window), "File Organizer System");

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // TreeStore: 2 columns (Name, Path)
    tree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    populate_tree_store(".", NULL);

    // TreeView
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tree_store));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    g_signal_connect(selection, "changed", G_CALLBACK(on_tree_selection_changed), NULL);

    // Drag & Drop
    setup_drag_and_drop(tree_view);

    // Image preview
    image_preview = gtk_image_new();

    // View mode buttons
    grid_button = gtk_button_new_with_label("Grid View");
    list_button = gtk_button_new_with_label("List View");
    g_signal_connect(grid_button, "clicked", G_CALLBACK(switch_to_grid), NULL);
    g_signal_connect(list_button, "clicked", G_CALLBACK(switch_to_list), NULL);

    // Batch operation buttons
    GtkWidget *rename_button = gtk_button_new_with_label("Rename");
    GtkWidget *delete_button = gtk_button_new_with_label("Delete");
    g_signal_connect(rename_button, "clicked", G_CALLBACK(rename_selected_file), selection);
    g_signal_connect(delete_button, "clicked", G_CALLBACK(delete_selected_file), selection);

    // Layout
    GtkWidget *vbox_buttons = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox_buttons), grid_button, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox_buttons), list_button, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox_buttons), rename_button, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox_buttons), delete_button, FALSE, FALSE, 2);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), tree_view, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), image_preview, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox_buttons, FALSE, FALSE, 5);

    gtk_container_add(GTK_CONTAINER(window), hbox);
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}
