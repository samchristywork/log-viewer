#include <boost/filesystem.hpp>
#include <iostream>
#include <vector>

#include "filter.hpp"

using namespace std;

vector<filter_t> add_filter(vector<filter_t> filters, const char *label, const char *regex, bool discard) {
  filter_t filter;

  filter.count = 0;
  filter.regex = regex;
  filter.label = label;
  regcomp(&filter.compiled_regex, filter.regex.c_str(), REG_ICASE);
  filter.discard = discard;

  filters.push_back(filter);
  return filters;
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

vector<filter_t> read_logs(vector<string> filenames, settings_t settings) {
  vector<filter_t> filters;

  filters=add_filter(filters, "Errors", "error", false);
  filters=add_filter(filters, "Warnings", "warn", false);

  filters=add_filter(filters, "Unmatched", "", false); // This should always exist
  for (unsigned int i = 0; i < filenames.size(); i++) {
    cout << "Reading file: " << filenames[i] << endl;
    boost::filesystem::ifstream handler(filenames[i]);
    string line;
    int lineno = 0;
    while (getline(handler, line)) {
      for (unsigned int j = 0; j < filters.size(); j++) {
        if (regexec(&filters[j].compiled_regex, line.c_str(), 0, NULL, 0) == 0) {

          /*
           * NOTE: I cull messages here in order to save memory. This has the
           * down-side that for some operations (sort, adding/removing filters,
           * changing the time window) the log files have to be re-ingested.
           *
           * I could alternatively cull elsewhere, but only at the cost of
           * finding a way to store (potentially) several gigabytes of log
           * files, which I don't want to do if I can help it.
           */
          filters[j].count++;
          if (filters[j].count < 1000) {
            match_t match;
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.cend());
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.cend());
            match.file_index = i;
            match.text = line;
            match.lineno = lineno;
            filters[j].matches.push_back(match);
          }
          if(!settings.filter_passthrough){
            break;
          }
        }
      }
      lineno++;
    }
  }
  for (filter_t filter : filters) {
    regfree(&filter.compiled_regex);
  }
  return filters;
}

