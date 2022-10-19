#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <regex.h>
#include <time.h>

char filenames[1024][1024];
GtkWidget *statusbar;

typedef struct message_t {
  char *reporting_mechanism;
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
  store = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  DIR *dir = opendir("logs");
  if (dir == NULL) {
    perror("opendir");
    return NULL;
  }

  GtkTreeIter aiter;
  GtkTreeIter biter;
  GtkTreeIter citer;

  regex_t error_regex;
  regex_t warning_regex;

  regcomp(&error_regex, "error", REG_ICASE);
  regcomp(&warning_regex, "warn", REG_ICASE);

  gtk_tree_store_append(store, &aiter, NULL);
  gtk_tree_store_append(store, &biter, NULL);
  gtk_tree_store_append(store, &citer, NULL);

  int amt_e=0;
  int amt_w=0;
  int amt_u=0;

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
        line[strlen(line)-1]=0;
        messages[message_idx].reporting_mechanism = filenames[i - 1];
        messages[message_idx].line = line_no;
        messages[message_idx].text = malloc(strlen(line) + 1);
        strcpy(messages[message_idx].text, line);
        messages[message_idx].text[strlen(line) - 1] = 0; // Removes newline
        messages[message_idx].category = 0;
        messages[message_idx].timestamp = timestamp;
        message_idx++;
        line_no++;
        GtkTreeIter j;
        if(regexec(&error_regex, line, 0, NULL, 0)==0){
          gtk_tree_store_append(store, &j, &aiter);
          amt_e++;
        }else if(regexec(&warning_regex, line, 0, NULL, 0)==0){
          gtk_tree_store_append(store, &j, &biter);
          amt_w++;
        }else{
          gtk_tree_store_append(store, &j, &citer);
          amt_u++;
        }
        char number[256];
        sprintf(number, "%d", line_no);
        gtk_tree_store_set(store, &j, 0, "", 1, "", 2, filenames[i-1], 3, number, 4, line, -1);
      }

      if (line) {
        free(line);
      }

      if (f) {
        fclose(f);
      }
    }
  }
  int total=0;
  char buf[256];
  sprintf(buf, "%d", amt_e);
  gtk_tree_store_set(store, &aiter, 0, "Errors", 1, buf, 2, "", 3, "", -1);
  sprintf(buf, "%d", amt_w);
  gtk_tree_store_set(store, &biter, 0, "Warnings", 1, buf, 2, "", 3, "", -1);
  sprintf(buf, "%d", amt_u);
  gtk_tree_store_set(store, &citer, 0, "Uncategorized", 1, buf, 2, "", 3, "", -1);
  closedir(dir);

  sprintf(buf, "%d messages tracked.", total);
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);

  return GTK_TREE_MODEL(store);
}

GtkWidget *create_view_and_model() {
  GtkWidget *view = gtk_tree_view_new();

  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Category",
                                              renderer,
                                              "text", 0,
                                              NULL);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Amount",
                                              renderer,
                                              "text", 1,
                                              NULL);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Reporting Mechanism",
                                              renderer,
                                              "text", 2,
                                              NULL);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Line",
                                              renderer,
                                              "text", 3,
                                              NULL);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1,
                                              "Text",
                                              renderer,
                                              "text", 4,
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

  GtkWidget *menu_file = gtk_menu_item_new_with_label("File");
  GtkWidget *menu_edit = gtk_menu_item_new_with_label("Edit");
  GtkWidget *menu_view = gtk_menu_item_new_with_label("View");
  GtkWidget *menu_tools = gtk_menu_item_new_with_label("Tools");
  GtkWidget *menu_help = gtk_menu_item_new_with_label("Help");

  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_file);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_edit);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_view);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_tools);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_help);

  GtkWidget *file_submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_file), file_submenu);
  GtkWidget *open = gtk_menu_item_new_with_label("Open");
  GtkWidget *quit = gtk_menu_item_new_with_label("Quit");
  gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), open);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), quit);

  GtkWidget *edit_submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_edit), edit_submenu);
  GtkWidget *foo = gtk_menu_item_new_with_label("Foo");
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_submenu), foo);
  statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
  GtkWidget *about = GTK_WIDGET(gtk_builder_get_object(builder, "about"));

  gtk_box_pack_start(GTK_BOX(all), menubar, FALSE, FALSE, 0);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(0, 0);
  gtk_container_add(GTK_CONTAINER(all), scrolled_window);
  GtkWidget *view = create_view_and_model();
  gtk_container_add(GTK_CONTAINER(scrolled_window), view);
  gtk_widget_set_vexpand(scrolled_window, TRUE);

  // CSS
  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  // Events
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keyPressCallback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);

  gtk_widget_show_all(window);
  gtk_main();
}
