#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <time.h>

char filenames[256][256];

typedef struct message_t {
  char *filename;
  int line;
  char *text;
  int category;
  time_t timestamp;
} message_t;
message_t messages[10000];
int message_idx = 0;

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

  DIR *dir = opendir("logs");
  if (dir == NULL) {
    perror("opendir");
    return EXIT_FAILURE;
  }
  int i = 0;
  struct dirent *dirent;
  while ((dirent = readdir(dir)) != NULL) {
    if (dirent->d_name[0] != '.') {
      char filename[PATH_MAX];
      sprintf(filename, "logs/%s", dirent->d_name);
      FILE *f = fopen(filename, "rb");
      strcpy(filenames[i], filename);
      i++;

      char *line = NULL;
      size_t len = 0;
      ssize_t read;

      int line_no = 0;
      while ((read = getline(&line, &len, f)) != -1) {

        struct tm tm;
        while (1) {
          char *ret;

          memset(&tm, 0, sizeof(tm));
          ret = strptime(line, "%b %d %H:%M:%S", &tm);
          if (ret) {
            tm.tm_year = 122;
            break;
          }

          memset(&tm, 0, sizeof(tm));
          ret = strptime(line, "%a %d %b %Y %H:%M:%S", &tm);
          if (ret) {
            break;
          }

          printf("Not handled: %s", line);
          break;
        }

        time_t timestamp = mktime(&tm);
        messages[message_idx].filename = filenames[i - 1];
        messages[message_idx].line = line_no;
        messages[message_idx].text = malloc(strlen(line) + 1);
        strcpy(messages[message_idx].text, line);
        messages[message_idx].text[strlen(line) - 1] = 0; // Removes newline
        messages[message_idx].category = 0;
        messages[message_idx].timestamp = timestamp;
        message_idx++;
        line_no++;
      }

      if (line) {
        free(line);
      }

      if (f) {
        fclose(f);
      }
    }
  }
  closedir(dir);

  for (int i = 0; i < message_idx; i++) {
    printf("%s %lu %s\n", messages[i].filename, messages[i].timestamp, messages[i].text);
  }

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
