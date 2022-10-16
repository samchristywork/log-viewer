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

  // Events
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keyPressCallback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);

  gtk_widget_show_all(window);
  gtk_main();
}
