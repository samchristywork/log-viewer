#ifndef FILTER_HPP
#define FILTER_HPP

#include <gtk/gtk.h>
#include <string>
#include <regex.h>
#include <iomanip>

using namespace std;

struct settings_t {
  bool filter_passthrough;
};

typedef struct match_t {
  string text;
  int file_index;
  int lineno;
} match_t;

typedef struct filter_t {
  GtkTreeIter iter;
  bool discard;
  regex_t compiled_regex;
  string label;
  string regex;
  unsigned long count;
  vector<match_t> matches;
} filter_t;


vector<filter_t> read_logs(vector<string> filenames, settings_t settings);

#endif
