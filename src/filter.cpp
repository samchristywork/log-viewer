#include <boost/filesystem.hpp>
#include <iostream>
#include <map>
#include <vector>

#include "filter.hpp"

using namespace std;

vector<filter_t> add_filter(vector<filter_t> filters, const char *label, vector<string> patterns, int pattern_type, bool discard, bool sample) {
  filter_t filter;

  filter.count = 0;
  filter.label = label;
  filter.patterns = patterns;
  filter.sample = sample;
  filter.pattern_type = pattern_type;

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

bool invalid(char c) {
  return c == 27;
}

vector<filter_t> read_logs(vector<filter_t> filters, vector<string> filenames, settings_t settings) {

  boost::filesystem::create_directories("output/");
  ofstream o;
  o.open("output/histogram");
  FILE *f = fopen("output/errors", "wb");
  map<string, int> map;

  for (unsigned int i = 0; i < filters.size(); i++) {
    filters[i].count = 0;
    filters[i].matches.clear();
  }

  for (unsigned int i = 0; i < filenames.size(); i++) {

    boost::filesystem::path p(filenames[i]);
    if (boost::filesystem::exists(p)) {
      time_t t = boost::filesystem::last_write_time(p);
      if ((t > settings.low_end || settings.low_end == 0) &&
          (t < settings.high_end || settings.high_end == 0)) {
        cout << "Reading file: " << filenames[i] << endl;
      } else {
        cout << "Skipping: " << filenames[i] << endl;
        continue;
      }
    }

    boost::filesystem::ifstream handler(filenames[i]);
    string line;
    int lineno = 0;
    while (getline(handler, line)) {
      for (unsigned int j = 0; j < filters.size(); j++) {
        bool match = false;
        if (filters[j].pattern_type == PATTERN_BASIC) {
          bool allmatch = true;
          for (unsigned int k = 0; k < filters[j].patterns.size(); k++) {
            if (strcasestr(line.c_str(), filters[j].patterns[k].c_str()) == 0) {
              allmatch = false;
              break;
            }
          }
          if (allmatch) {
            match = true;
          }
        }

        if (match) {
          /*
           * NOTE: I cull messages here in order to save memory. This has the
           * down-side that for some operations (sort, adding/removing filters,
           * changing the time window) the log files have to be re-ingested.
           *
           * I could alternatively cull elsewhere, but only at the cost of
           * finding a way to store (potentially) several gigabytes of log
           * files, which I don't want to do if I can help it.
           */
          if (filters[j].sample) {
            if (random() % 10 == 0) {
              fprintf(f, "%s\n", line.c_str());

              string token;
              stringstream stream(line);
              while (getline(stream, token, ' ')) {
                if (map.find(token) != map.end()) {
                  map.at(token)++;
                } else {
                  map.insert(pair<string, int>(token, 1));
                }
              }
            }
          }
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
          if (!settings.filter_passthrough) {
            break;
          }
        }
      }
      lineno++;
    }
  }
  for (const auto &pair : map) {
    string str = pair.first.c_str();
    str.erase(remove_if(str.begin(), str.end(), invalid), str.end());
    o << map.at(pair.first) << "\t" << str << endl;
  }
  o.close();
  for (filter_t filter : filters) {
    //if(filter.isregex){
    //  regfree(&filter.compiled_regex);
    //}
  }
  return filters;
}
