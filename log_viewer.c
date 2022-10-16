#include <gtk/gtk.h>

// Event
gboolean keyPressCallback(GtkWidget *widget, GdkEventKey *event, gpointer data) {
  if (event->keyval == GDK_KEY_Escape) {
    exit(EXIT_SUCCESS);
    return TRUE;
  }
  return FALSE;
}

void open_file() {
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
                                                  NULL,
                                                  GTK_FILE_CHOOSER_ACTION_OPEN,
                                                  "_Cancel",
                                                  GTK_RESPONSE_CANCEL,
                                                  "_Open",
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);

  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  if (res == GTK_RESPONSE_ACCEPT) {
    char *filename;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    filename = gtk_file_chooser_get_filename(chooser);

    FILE *f = fopen(filename, "rb");
    if (f) {
      fseek(f, 0L, SEEK_END);
      size_t len = ftell(f);
      rewind(f);

      char str[len + 1];
      fread(str, 1, len, f);
      str[len] = 0;
      fclose(f);

      puts(str);
    }
    g_free(filename);
  }
  gtk_widget_destroy(dialog);
}

GtkTreeModel *create_and_fill_model() {

  GtkTreeStore *store;
  store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  GtkTreeIter iter;
  for(int i=0;i<5;i++){
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, "file://test", 1, "", 2, "", -1);

    GtkTreeIter j;
    gtk_tree_store_append(store, &j, &iter);
    gtk_tree_store_set(store, &j, 0, "there", 1, "2", 2, "b", -1);
  }

  return GTK_TREE_MODEL(store);
}

GtkWidget *create_view_and_model() {
  GtkWidget *view = gtk_tree_view_new();

  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Reporting Mechanism",
                                              renderer,
                                              "text", 0,
                                              NULL);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Line",
                                              renderer,
                                              "text", 1,
                                              NULL);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Text",
                                              renderer,
                                              "text", 2,
                                              NULL);

  GtkTreeModel *model = create_and_fill_model();

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

  g_object_unref(model);

  return view;
}

int main(int argc, char *argv[]) {

  gtk_init(&argc, &argv);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  GtkWidget *all = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), all);

  GtkWidget *menubar = gtk_menu_bar_new();
  GtkWidget *fileMenu = gtk_menu_new();

  GtkWidget *file = gtk_menu_item_new_with_label("File");
  GtkWidget *open = gtk_menu_item_new_with_label("Open");
  GtkWidget *quit = gtk_menu_item_new_with_label("Quit");
  GtkWidget *example = gtk_menu_item_new_with_label("Example");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), fileMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), open);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quit);

  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), example);
  gtk_box_pack_start(GTK_BOX(all), menubar, FALSE, FALSE, 0);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(0, 0);
  gtk_container_add(GTK_CONTAINER(all), scrolled_window);
  GtkWidget *view = create_view_and_model();
  gtk_container_add(GTK_CONTAINER(scrolled_window), view);
  gtk_widget_set_vexpand(scrolled_window, TRUE);

  // Events
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keyPressCallback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);

  gtk_widget_show_all(window);
  gtk_main();
}
