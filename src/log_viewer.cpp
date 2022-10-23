#include <boost/filesystem.hpp>
#include <gtk/gtk.h>
#include <iomanip>
#include <iostream>

#include "log_viewer.hpp"
#include "graphics.hpp"
#include "filter.hpp"

using namespace std;

settings_t settings={
  false
};

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

void present_logs(vector<filter_t> filters) {
  for (filter_t filter : filters) {
    cout << filter.label << " " << filter.count << endl;
  }
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);
  vector<string> filenames = get_filenames("data");

  vector<filter_t> filters = read_logs(filenames, settings);
  //present_logs();
  graphics_main(filters, filenames);
}
