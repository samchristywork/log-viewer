#define _XOPEN_SOURCE
#include <boost/filesystem.hpp>
#include <gtk/gtk.h>
#include <iomanip>
#include <regex.h>

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

typedef struct filter_t {
  GtkTreeIter iter;
  regex_t compiled_regex;
  int count;
  char label[256];
  char regex[1024];
} filter_t;
struct filter_t *filters;
int num_filters = 0;

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

      char *str = (char *)malloc(len + 1);
      fread(str, 1, len, f);
      str[len] = 0;
      fclose(f);

      puts(str);
      free(str);
    }
    g_free(filename);
  }
  gtk_widget_destroy(dialog);
}

void add_filter(const char *label, const char *regex) {
  filters = (filter_t *)realloc(filters, (num_filters + 1) * sizeof(filter_t));

  strcpy(filters[num_filters].regex, regex);
  strcpy(filters[num_filters].label, label);
  regcomp(&filters[num_filters].compiled_regex, filters[num_filters].regex, REG_ICASE);
  filters[num_filters].count = 0;
  num_filters++;
}

char *strptime2(const char *s, const char *format, struct tm *tm) {
  std::istringstream input(s);
  input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
  input >> std::get_time(tm, format);
  if (input.fail()) {
    return nullptr;
  }
  return (char *)(s + input.tellg());
}

GtkTreeModel *create_and_fill_model() {

  GtkTreeStore *store;
  store = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);


  filters = (filter_t *)malloc(0);

  add_filter("Tracker Miner", "tracker.miner");
  add_filter("Pipewire", "pipewire");
  add_filter("Errors", "error");
  add_filter("Warnings", "warn");
  add_filter("Uncategorized", "");

  for (int i = 0; i < num_filters; i++) {
    gtk_tree_store_append(store, &filters[i].iter, NULL);
  }

  boost::filesystem::directory_iterator iter{"sample_data"};

  int i = 0;
  while (iter != boost::filesystem::directory_iterator{}) {
    std::string filepath = (*iter++).path().string();
    char filename[PATH_MAX];
    sprintf(filename, "%s", filepath.c_str());
    puts(filename);
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
        ret = strptime2(line, "%b %d %H:%M:%S", &tm);
        if (ret) {
          tm.tm_year = 122;
          break;
        }

        memset(&tm, 0, sizeof(tm));
        ret = strptime2(line, "%a %d %b %Y %H:%M:%S", &tm);
        if (ret) {
          break;
        }

        printf("Timestamp not handled: %s", line);
        break;
      }

      time_t timestamp = mktime(&tm);
      line[strlen(line) - 1] = 0;
      messages[message_idx].reporting_mechanism = filenames[i - 1];
      messages[message_idx].line = line_no;
      messages[message_idx].text = (char *)malloc(strlen(line) + 1);
      strcpy(messages[message_idx].text, line);
      messages[message_idx].text[strlen(line) - 1] = 0; // Removes newline
      messages[message_idx].category = 0;
      messages[message_idx].timestamp = timestamp;
      message_idx++;
      line_no++;
      GtkTreeIter j;
      for (int i = 0; i < num_filters; i++) {
        if (regexec(&filters[i].compiled_regex, line, 0, NULL, 0) == 0) {
          gtk_tree_store_append(store, &j, &filters[i].iter);
          filters[i].count++;
          break;
        }
      }

      char number[256];
      sprintf(number, "%d", line_no);
      gtk_tree_store_set(store, &j, 0, "", 1, "", 2, filenames[i - 1], 3, number, 4, line, -1);
    }

    if (line) {
      free(line);
    }

    if (f) {
      fclose(f);
    }
  }
  int total = 0;
  char buf[256];
  for (int i = 0; i < num_filters; i++) {
    total += filters[i].count;
    sprintf(buf, "%d", filters[i].count);
    gtk_tree_store_set(store, &filters[i].iter, 0, filters[i].label, 1, buf, 2, "", 3, "", -1);
  }

  sprintf(buf, "%d messages tracked.", total);
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);

  return GTK_TREE_MODEL(store);
}

GtkWidget *create_view_and_model(GtkWidget *view) {
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Category", gtk_cell_renderer_text_new(), "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Amount", gtk_cell_renderer_text_new(), "text", 1, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Reporting Mechanism", gtk_cell_renderer_text_new(), "text", 2, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Line", gtk_cell_renderer_text_new(), "text", 3, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Text", gtk_cell_renderer_text_new(), "text", 4, NULL);

  GtkTreeModel *model = create_and_fill_model();
  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
  g_object_unref(model);

  return view;
}


void show_about() {
  GdkPixbuf *pbuf = gdk_pixbuf_new_from_file("image.png", NULL);
  const char *authors[] = {
      "Sam Christy",
      NULL};
  gtk_show_about_dialog(NULL,
                        "authors", authors,
                        "comments", "\"There are two ways of constructing a software design: One way is to make it so simple that there are obviously no deficiencies, and the other way is to make it so complicated that there are no obvious deficiencies. The first method is far more difficult.\" - C. A. R. Hoare",
                        "copyright", "©2022 Sam Christy",
                        "license", "GNU General Public License version 3 (GPLv3)",
                        "license-type", GTK_LICENSE_GPL_3_0,
                        "logo", pbuf,
                        "program-name", "Log Viewer",
                        "title", "About Log Viewer",
                        "version", "v1.0.0",
                        "website", "https://github.com/samchristywork",
                        "website-label", "github.com/samchristywork",
                        NULL);
}

int main(int argc, char *argv[]) {

  gtk_init(&argc, &argv);

  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "log_viewer.glade", NULL);

  GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

  statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
  GtkWidget *about = GTK_WIDGET(gtk_builder_get_object(builder, "about"));

  GtkWidget *treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview"));
  create_view_and_model(treeview);

  GtkWidget *quit = GTK_WIDGET(gtk_builder_get_object(builder, "quit"));

  // CSS
  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_builder_connect_signals(builder, NULL);

  // Events
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keyPressCallback), NULL);
  g_signal_connect(G_OBJECT(about), "activate", G_CALLBACK(show_about), NULL);

  g_object_unref(builder);

  gtk_widget_show_all(window);
  gtk_main();
}
