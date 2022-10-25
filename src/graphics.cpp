#include <algorithm>
#include <cjson/cJSON.h>
#include <gtk/gtk.h>
#include <iostream>
#include <vector>

#include "filter.hpp"
#include "graphics.hpp"
#include "log_viewer.hpp"

using namespace std;

settings_t settings = {
    false};

GtkWidget *statusbar;
GtkTreeModel *model;

vector<filter_t> refresh_filters;
vector<string> refresh_filenames;
settings_t refresh_settings;
GtkBuilder *builder;

gboolean keypress_callback(GtkWidget *widget, GdkEventKey *event, gpointer data) {
  if (event->keyval == GDK_KEY_Escape) {
    exit(EXIT_SUCCESS);
    return TRUE;
  }
  return FALSE;
}

GtkTreeModel *build_model(GtkWidget *view) {
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Category", gtk_cell_renderer_text_new(), "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Amount", gtk_cell_renderer_text_new(), "text", 1, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Reporting Mechanism", gtk_cell_renderer_text_new(), "text", 2, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Line", gtk_cell_renderer_text_new(), "text", 3, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Text", gtk_cell_renderer_text_new(), "text", 4, NULL);
  gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 0), true);
  gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 1), true);
  gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 2), true);
  gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 3), true);
  gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 4), true);

  GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

  return model;
}

void add_data_to_model(GtkTreeModel *model, vector<filter_t> filters, vector<string> filenames) {
  for (unsigned int i = 0; i < filters.size(); i++) {
    gtk_tree_store_append(GTK_TREE_STORE(model), &(filters[i].iter), NULL);
  }

  for (unsigned int i = 0; i < filters.size(); i++) {
    GtkTreeIter k;
    for (unsigned int j = 0; j < filters[i].matches.size(); j++) {
      gtk_tree_store_append(GTK_TREE_STORE(model), &k, &filters[i].iter);
      char number[256];
      sprintf(number, "%d", filters[i].matches[j].lineno);
      string reporting_mechanism = "file://" + filenames[filters[i].matches[j].file_index];
      gtk_tree_store_set(GTK_TREE_STORE(model), &k, 0, "", 1, "", 2, reporting_mechanism.c_str(), 3, number, 4, filters[i].matches[j].text.c_str(), -1);
    }
  }

  int total = 0;
  for (unsigned int i = 0; i < filters.size(); i++) {
    total += filters[i].count;
    char buf[256];
    sprintf(buf, "%lu", filters[i].count);
    gtk_tree_store_set(GTK_TREE_STORE(model), &(filters[i].iter), 0, filters[i].label.c_str(), 1, buf, 2, "", 3, "", -1);
  }

  char buf[256];
  sprintf(buf, "%d messages tracked.", total);
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);
}

void refresh() {
  refresh_filters = read_logs(refresh_filters, refresh_filenames, refresh_settings);
  gtk_tree_store_clear(GTK_TREE_STORE(model));
  add_data_to_model(model, refresh_filters, refresh_filenames);
}

void refresh_callback() {
  refresh();
}

void add_filter_callback() {
  vector<string> s;
  s.push_back("warn");
  refresh_filters = add_filter(refresh_filters, "Label", s, PATTERN_BASIC, false, false);
  std::rotate(refresh_filters.rbegin(), refresh_filters.rbegin() + 1, refresh_filters.rend());
}

void delete_filter_callback() {
  GtkWidget *deleteFilterWindow = GTK_WIDGET(gtk_builder_get_object(builder, "delete-filter-window"));
  gtk_window_set_default_size(GTK_WINDOW(deleteFilterWindow), 100, 100);
  gtk_window_set_resizable(GTK_WINDOW(deleteFilterWindow), FALSE);
  gtk_widget_show_all(deleteFilterWindow);
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

cJSON *read_json() {
  FILE *f = fopen("filters.json", "rb");
  if (!f) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  rewind(f);

  char *buffer = (char *)malloc(size + 1);
  buffer[size] = 0;
  int ret = fread(buffer, 1, size, f);
  if (ret != size) {
    fprintf(stderr, "Could not read the expected number of bytes.\n");
    exit(EXIT_FAILURE);
  }

  fclose(f);

  cJSON *cjson = cJSON_Parse(buffer);
  if (!cjson) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr) {
      fprintf(stderr, "Error before: %s\n", error_ptr);
    }
    cJSON_Delete(cjson);
    exit(EXIT_FAILURE);
  }
  free(buffer);

  return cjson;
}

cJSON *find(cJSON *tree, const char *str) {
  cJSON *node = NULL;

  if (tree) {
    node = tree->child;
    while (1) {
      if (!node) {
        break;
      }
      if (strcmp(str, node->string) == 0) {
        break;
      }
      node = node->next;
    }
  }
  return node;
}

void graphics_main(vector<string> filenames) {

  builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "res/log_viewer.glade", NULL);

  GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

  const char *buf = "";
  statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);
  GtkWidget *about = GTK_WIDGET(gtk_builder_get_object(builder, "about"));

  vector<filter_t> filters;

  cJSON *cjson = read_json();
  cJSON *cjson_filter = find(cjson, "filters");
  cJSON *node = cjson_filter->child;
  while (1) {
    vector<string> s;
    if (!node) {
      break;
    }
    cJSON *label = find(node, "label");
    cJSON *sample = find(node, "sample");
    cJSON *patterns = find(node, "patterns");
    cJSON *pattern = patterns->child;
    while (1) {
      if (!pattern) {
        break;
      }
      s.push_back(pattern->valuestring);
      pattern = pattern->next;
    }
    filters = add_filter(filters, label->valuestring, s, PATTERN_BASIC, false, cJSON_IsTrue(sample));
    node = node->next;
  }

  GtkWidget *treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview"));
  model = build_model(treeview);

  refresh_filters = filters;
  refresh_filenames = filenames;
  refresh();

  GtkWidget *quit = GTK_WIDGET(gtk_builder_get_object(builder, "quit"));
  GtkWidget *addFilter = GTK_WIDGET(gtk_builder_get_object(builder, "add-filter"));
  GtkWidget *deleteFilter = GTK_WIDGET(gtk_builder_get_object(builder, "delete-filter"));
  GtkWidget *dateRange = GTK_WIDGET(gtk_builder_get_object(builder, "date-range"));
  GtkWidget *refresh = GTK_WIDGET(gtk_builder_get_object(builder, "refresh"));

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "res/style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_builder_connect_signals(builder, NULL);

  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(about), "activate", G_CALLBACK(show_about), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(addFilter), "clicked", G_CALLBACK(add_filter_callback), NULL);
  g_signal_connect(G_OBJECT(deleteFilter), "clicked", G_CALLBACK(delete_filter_callback), NULL);
  g_signal_connect(G_OBJECT(dateRange), "clicked", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(refresh), "clicked", G_CALLBACK(refresh_callback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keypress_callback), NULL);
  //g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);
  //g_signal_connect(G_OBJECT(treeview), "cursor-changed", G_CALLBACK(foo), NULL);

  //g_object_unref(builder);

  gtk_widget_show_all(window);
  gtk_main();
}
