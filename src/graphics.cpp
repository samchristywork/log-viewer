#include <gtk/gtk.h>
#include <iostream>
#include <vector>

#include "log_viewer.hpp"

GtkWidget *statusbar;

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

  GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

  return model;
}

void add_data_to_model(GtkTreeModel *model, vector<filter_t> filters, vector<string>filenames) {
  for (unsigned int i = 0; i < filters.size(); i++) {
    gtk_tree_store_append(GTK_TREE_STORE(model), &(filters[i].iter), NULL);
  }

  for (unsigned int i = 0; i < filters.size(); i++) {
    GtkTreeIter k;
    for (unsigned int j=0;j<filters[i].matches.size();j++) {
      gtk_tree_store_append(GTK_TREE_STORE(model), &k, &filters[i].iter);
      char number[256];
      sprintf(number, "%d", filters[i].matches[j].lineno);
      gtk_tree_store_set(GTK_TREE_STORE(model), &k, 0, "", 1, "", 2, filenames[filters[i].matches[j].file_index].c_str(), 3, number, 4, filters[i].matches[j].text.c_str(), -1);
      //gtk_tree_store_set(GTK_TREE_STORE(model), &k, 0, "", 1, "", 2, "Filename", 3, number, 4, filters[i].matches[j].text.c_str(), -1);
    }
  }

  int total=0;
  for (unsigned int i = 0; i < filters.size(); i++) {
    total+=filters[i].count;
    char buf[256];
    sprintf(buf, "%lu", filters[i].count);
    gtk_tree_store_set(GTK_TREE_STORE(model), &(filters[i].iter), 0, filters[i].label.c_str(), 1, buf, 2, "", 3, "", -1);
  }

  char buf[256];
  sprintf(buf, "%d messages tracked.", total);
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);
}

void foo(vector<filter_t> filters, vector<string> filenames) {
  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "res/log_viewer.glade", NULL);

  GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

  const char *buf = "Hello, World!";
  statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);
  //GtkWidget *about = GTK_WIDGET(gtk_builder_get_object(builder, "about"));

  GtkWidget *treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview"));
  GtkTreeModel *model = build_model(treeview);
  add_data_to_model(model, filters, filenames);

  GtkWidget *quit = GTK_WIDGET(gtk_builder_get_object(builder, "quit"));

  // CSS
  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "res/style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_builder_connect_signals(builder, NULL);

  // Events
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  //g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  //g_signal_connect(G_OBJECT(treeview), "cursor-changed", G_CALLBACK(foo), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keypress_callback), NULL);
  //g_signal_connect(G_OBJECT(about), "activate", G_CALLBACK(show_about), NULL);

  g_object_unref(builder);

  gtk_widget_show_all(window);
  gtk_main();
}
