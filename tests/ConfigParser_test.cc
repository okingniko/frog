#include "ConfigParser.h"

#include <string>
#include <iostream>

using namespace frog;

int main(const int argc, const char *argv[])
{
  if (argc != 2) {
    std::cerr << "Usage:\n\t" << argv[0] << " configfilename" << std::endl;  
    return 1;
  }
  std::string conf_path(argv[1]);
  ConfigParser conf(conf_path);

  std::string logo_path = conf.GetLogoPath();
  std::cout << "Logo file locate at: " << logo_path << '\n'
            << "Document root locate at: " << conf.GetDocumentRoot() << '\n'
            << "Server port equals: " << conf.GetServerPort() << '\n'
            << "Threads Count equals: " << conf.GetThreadsCount() << '\n'
            << std::endl;
  return 0;
}


