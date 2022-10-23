#ifndef LOG_VIEWER_HPP
#define LOG_VIEWER_HPP

#include <regex.h>
#include <string>

using namespace std;

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

#endif
