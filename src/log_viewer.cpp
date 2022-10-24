#include <boost/filesystem.hpp>
#include <gtk/gtk.h>
#include <iomanip>
#include <iostream>

#include "filter.hpp"
#include "graphics.hpp"
#include "log_viewer.hpp"

using namespace std;

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

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);
  vector<string> filenames = get_filenames("data");

  graphics_main(filenames);
}
