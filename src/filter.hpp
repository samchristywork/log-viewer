#ifndef FILTER_HPP
#define FILTER_HPP

#include <gtk/gtk.h>
#include <iomanip>
#include <regex.h>
#include <string>

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

vector<filter_t> add_filter(vector<filter_t> filters, const char *label, const char *regex, bool discard);
vector<filter_t> read_logs(vector<filter_t> filters, vector<string> filenames, settings_t settings);

#endif
