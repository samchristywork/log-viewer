#ifndef FILTER_HPP
#define FILTER_HPP

#include <gtk/gtk.h>
#include <iomanip>
#include <regex.h>
#include <string>

using namespace std;

enum {
  PATTERN_NONE = 0,
  PATTERN_BASIC,
  PATTERN_REGEX
};

struct settings_t {
  bool filter_passthrough;
  time_t low_end;
  time_t high_end;
};

typedef struct match_t {
  string text;
  int file_index;
  int lineno;
} match_t;

typedef struct filter_t {
  GtkTreeIter iter;
  bool discard;
  bool sample;
  int pattern_type;
  string label;
  unsigned long count;
  vector<match_t> matches;
  vector<regex_t> compiled_regexes;
  vector<string> patterns;
} filter_t;

vector<filter_t> add_filter(vector<filter_t> filters, const char *label, vector<string> patterns, int pattern_type, bool discard, bool sample);
vector<filter_t> read_logs(vector<filter_t> filters, vector<string> filenames, settings_t settings);

#endif
