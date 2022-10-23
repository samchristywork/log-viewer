#include <boost/filesystem.hpp>
#include <gtk/gtk.h>
#include <iomanip>
#include <iostream>

#include "log_viewer.hpp"
#include "graphics.hpp"

vector<filter_t> filters;

using namespace std;

void add_filter(const char *label, const char *regex, bool discard) {
  filter_t filter;

  filter.count = 0;
  filter.regex = regex;
  filter.label = label;
  regcomp(&filter.compiled_regex, filter.regex.c_str(), REG_ICASE);
  filter.discard = discard;

  filters.push_back(filter);
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

void read_logs(vector<string> filenames) {

  add_filter("Errors", "error", false);
  add_filter("Warnings", "warn", false);
  add_filter("Unmatched", "", false);
  for (unsigned int i = 0; i < filenames.size(); i++) {
    cout << "Reading file: " << filenames[i] << endl;
    boost::filesystem::ifstream handler(filenames[i]);
    string line;
    int lineno = 0;
    while (getline(handler, line)) {
      for (unsigned int j = 0; j < filters.size(); j++) {
        if (regexec(&filters[j].compiled_regex, line.c_str(), 0, NULL, 0) == 0) {
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
        }
      }
      lineno++;
    }
  }
  for (filter_t filter : filters) {
    regfree(&filter.compiled_regex);
  }
}

vector<string> get_filenames(string directory) {
  vector<string> filenames;
  boost::filesystem::recursive_directory_iterator iter{directory};
  while (iter != boost::filesystem::recursive_directory_iterator{}) {
    boost::filesystem::path path = (*iter++).path();
    if (boost::filesystem::is_directory(path)) {
      continue;
    }
    filenames.push_back(path.string());
  }
  return filenames;
}

void present_logs() {
  for (filter_t filter : filters) {
    cout << filter.label << " " << filter.count << endl;
  }
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);
  vector<string> filenames = get_filenames("data");
  read_logs(filenames);
  present_logs();
  graphics_main(filters, filenames);
}
