#include <algorithm>
#include <boost/date_time.hpp>
#include <cjson/cJSON.h>
#include <gtk/gtk.h>
#include <iostream>
#include <vector>

#include "filter.hpp"
#include "graphics.hpp"
#include "log_viewer.hpp"

using namespace std;

settings_t settings;

GtkBuilder *builder;
GtkTreeModel *model;
GtkWidget *deleteFilterWindow;
GtkWidget *statusbar;
GtkWidget *window;
GtkWidget *date_range_window;
GtkWidget *spinbutton1;
GtkWidget *spinbutton2;
GtkWidget *spinbutton3;
GtkWidget *spinbutton4;
GtkWidget *calendar1;
GtkWidget *calendar2;

vector<filter_t> refresh_filters;
vector<string> refresh_filenames;

gboolean keypress_callback(GtkWidget *widget, GdkEventKey *event, gpointer data) {
  if (event->keyval == GDK_KEY_Escape) {
    exit(EXIT_SUCCESS);
    return TRUE;
  }
  return FALSE;
}

void reporting_mechanism_renderer_func(GtkTreeViewColumn *col,
                        GtkCellRenderer *renderer,
                        GtkTreeModel *model,
                        GtkTreeIter *iter,
                        gpointer user_data) {
  char *raw_string = 0;
  gtk_tree_model_get(model, iter, 2, &raw_string, -1);
  gchar *escaped_string = g_markup_escape_text(raw_string, -1);
  gchar buf[PATH_MAX];
  snprintf(buf, sizeof(buf), "%s", escaped_string);
  g_object_set(renderer, "editable", TRUE, NULL);
  g_object_set(renderer, "markup", buf, NULL); // Use "text" to exclude markup.
  free(escaped_string);
}

void line_renderer_func(GtkTreeViewColumn *col,
                        GtkCellRenderer *renderer,
                        GtkTreeModel *model,
                        GtkTreeIter *iter,
                        gpointer user_data) {
  char *raw_string = 0;
  gtk_tree_model_get(model, iter, 4, &raw_string, -1);
  if(raw_string){
    gchar *escaped_string = g_markup_escape_text(raw_string, -1);
    gchar buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s", escaped_string); // TODO: Highlight filter matches
    g_object_set(renderer, "editable", TRUE, NULL);
    g_object_set(renderer, "markup", buf, NULL); // Use "text" to exclude markup.
    free(escaped_string);
  }else{
    g_object_set(renderer, "text", "", NULL);
  }
}

GtkTreeModel *build_model(GtkWidget *view) {
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Category", gtk_cell_renderer_text_new(), "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Amount", gtk_cell_renderer_text_new(), "text", 1, NULL);
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(view), -1, "Reporting Mechanism", renderer, reporting_mechanism_renderer_func, NULL, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Line", gtk_cell_renderer_text_new(), "text", 3, NULL);
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(view), -1, "Text", renderer, line_renderer_func, NULL, NULL);
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
  refresh_filters = read_logs(refresh_filters, refresh_filenames, settings);
  gtk_tree_store_clear(GTK_TREE_STORE(model));
  add_data_to_model(model, refresh_filters, refresh_filenames);
}

void date_range_cancel_callback() {
  refresh();
  gtk_widget_hide(date_range_window);
  gtk_widget_show_all(window);
}

void date_range_apply_callback() {
  unsigned int mi1, mi2, h1, h2, d1, d2, m1, m2, y1, y2;
  gtk_calendar_get_date(GTK_CALENDAR(calendar1), &y1, &m1, &d1);
  gtk_calendar_get_date(GTK_CALENDAR(calendar2), &y2, &m2, &d2);
  h1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton1));
  mi1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton2));
  h2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton3));
  mi2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton4));
  m1++;
  m2++;
  cout << y1 << "-" << m1 << "-" << d1 << " ";
  cout << h1 << ":" << mi1 << endl;
  cout << y2 << "-" << m2 << "-" << d2 << " ";
  cout << h2 << ":" << mi2 << endl;

  struct tm tm1;
  tm1.tm_sec = 0;
  tm1.tm_min = mi1;
  tm1.tm_hour = h1;
  tm1.tm_mday = d1;
  tm1.tm_mon = m1 - 1;
  tm1.tm_year = y1 - 1900;
  settings.low_end = mktime(&tm1);

  struct tm tm2;
  tm2.tm_sec = 0;
  tm2.tm_min = mi2;
  tm2.tm_hour = h2;
  tm2.tm_mday = d2;
  tm2.tm_mon = m2 - 1;
  tm2.tm_year = y2 - 1900;
  settings.high_end = mktime(&tm2);

  cout << settings.low_end << endl;
  cout << settings.high_end << endl;

  refresh();
  gtk_widget_hide(date_range_window);
  gtk_widget_show_all(window);
}

gboolean date_range_keypress_callback(GtkWidget *widget, GdkEventKey *event, gpointer data) {
  if (event->keyval == GDK_KEY_Escape) {
    gtk_widget_hide(widget);
    refresh();
    gtk_widget_show_all(window);
    return TRUE;
  }
  return FALSE;
}

void row_activated_callback(GtkTreeView *view, GtkTreePath *path) {
  GtkTreeModel *model = gtk_tree_view_get_model(view);

  GtkTreeIter iter;
  if (gtk_tree_model_get_iter(model, &iter, path)) {

    char *data;
    gtk_tree_model_get(model, &iter, 2, &data, -1);

    puts(data);

    g_free(data);
  }
}

void refresh_callback() {
  refresh();
}

void add_filter_callback() {
  vector<string> s;
  s.push_back("warn");
  refresh_filters = add_filter(refresh_filters, "Label", s, PATTERN_BASIC, false, false);
  std::rotate(refresh_filters.rbegin(), refresh_filters.rbegin() + 1, refresh_filters.rend());
  refresh();
}

void remove_button_callback() {
  gtk_widget_hide(deleteFilterWindow);
  refresh();
}

void cancel_button_callback() {
  gtk_widget_hide(deleteFilterWindow);
}

void delete_filter_callback() {
  deleteFilterWindow = GTK_WIDGET(gtk_builder_get_object(builder, "delete-filter-window"));

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  for (unsigned int i = 0; i < refresh_filters.size(); i++) {
    GtkWidget *check = gtk_check_button_new_with_label(refresh_filters[i].label.c_str());
    gtk_container_add(GTK_CONTAINER(box), check);
  }

  GtkWidget *removeButton = gtk_button_new_with_label("Remove");
  gtk_container_add(GTK_CONTAINER(box), removeButton);

  GtkWidget *cancelButton = gtk_button_new_with_label("Cancel");
  gtk_container_add(GTK_CONTAINER(box), cancelButton);

  gtk_container_add(GTK_CONTAINER(deleteFilterWindow), box);
  gtk_window_set_resizable(GTK_WINDOW(deleteFilterWindow), FALSE);
  gtk_widget_show_all(deleteFilterWindow);

  g_signal_connect(G_OBJECT(removeButton), "clicked", G_CALLBACK(remove_button_callback), NULL);
  g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(cancel_button_callback), NULL);
  g_signal_connect(G_OBJECT(deleteFilterWindow), "key_press_event", G_CALLBACK(keypress_callback), NULL);
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

  boost::posix_time::ptime time = boost::posix_time::second_clock::local_time();

  settings.filter_passthrough = false;
  settings.low_end = 0;
  settings.high_end = 0;

  builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "res/log_viewer.glade", NULL);

  date_range_window = GTK_WIDGET(gtk_builder_get_object(builder, "date-range-window"));
  gtk_window_set_resizable(GTK_WINDOW(date_range_window), FALSE);
  GtkWidget *date_range_cancel_button = GTK_WIDGET(gtk_builder_get_object(builder, "date-range-cancel"));
  GtkWidget *date_range_apply_button = GTK_WIDGET(gtk_builder_get_object(builder, "date-range-apply"));
  calendar1 = GTK_WIDGET(gtk_builder_get_object(builder, "calendar1"));
  gtk_calendar_select_month(GTK_CALENDAR(calendar1), time.date().month() - 1, time.date().year());
  gtk_calendar_select_day(GTK_CALENDAR(calendar1), time.date().day());
  calendar2 = GTK_WIDGET(gtk_builder_get_object(builder, "calendar2"));
  gtk_calendar_select_month(GTK_CALENDAR(calendar2), time.date().month() - 1, time.date().year());
  gtk_calendar_select_day(GTK_CALENDAR(calendar1), time.date().day());
  gtk_widget_grab_focus(GTK_WIDGET(date_range_apply_button));

  GtkAdjustment *adjustment1 = gtk_adjustment_new(0.0, 0.0, 23.0, 1.0, 1.0, 0.0);
  GtkAdjustment *adjustment2 = gtk_adjustment_new(0.0, 0.0, 59.0, 1.0, 1.0, 0.0);
  GtkAdjustment *adjustment3 = gtk_adjustment_new(23.0, 0.0, 59.0, 1.0, 1.0, 0.0);
  GtkAdjustment *adjustment4 = gtk_adjustment_new(59.0, 0.0, 59.0, 1.0, 1.0, 0.0);
  spinbutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "spinbutton1"));
  spinbutton2 = GTK_WIDGET(gtk_builder_get_object(builder, "spinbutton2"));
  spinbutton3 = GTK_WIDGET(gtk_builder_get_object(builder, "spinbutton3"));
  spinbutton4 = GTK_WIDGET(gtk_builder_get_object(builder, "spinbutton4"));
  gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(spinbutton1), adjustment1);
  gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(spinbutton2), adjustment2);
  gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(spinbutton3), adjustment3);
  gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(spinbutton4), adjustment4);

  g_signal_connect(G_OBJECT(date_range_cancel_button), "clicked", G_CALLBACK(date_range_cancel_callback), NULL);
  g_signal_connect(G_OBJECT(date_range_apply_button), "clicked", G_CALLBACK(date_range_apply_callback), NULL);
  g_signal_connect(G_OBJECT(date_range_window), "key_press_event", G_CALLBACK(date_range_keypress_callback), NULL);
  gtk_widget_show_all(date_range_window);

  window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

  const char *buf = "";
  statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);

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

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "res/style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_builder_connect_signals(builder, NULL);

  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(GTK_WIDGET(
                       gtk_builder_get_object(builder, "about"))),
                   "activate", G_CALLBACK(show_about), NULL);
  g_signal_connect(G_OBJECT(GTK_WIDGET(
                       gtk_builder_get_object(builder, "quit"))),
                   "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(GTK_WIDGET(
                       gtk_builder_get_object(builder, "add-filter"))),
                   "clicked", G_CALLBACK(add_filter_callback), NULL);
  g_signal_connect(G_OBJECT(
                       GTK_WIDGET(
                           gtk_builder_get_object(builder, "delete-filter"))),
                   "clicked", G_CALLBACK(delete_filter_callback), NULL);
  g_signal_connect(G_OBJECT(
                       GTK_WIDGET(
                           gtk_builder_get_object(builder, "date-range"))),
                   "clicked", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(
                       GTK_WIDGET(
                           gtk_builder_get_object(builder, "refresh"))),
                   "clicked", G_CALLBACK(refresh_callback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keypress_callback), NULL);
  g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(row_activated_callback), NULL);

  gtk_main();
}
