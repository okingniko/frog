#include "HttpServer.h"
#include "HttpClient.h"
#include "ConfigParser.h"

// added for the json-example
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;
using namespace frog;
namespace pt = boost::property_tree;

using HttpServer = frog::Server<frog::HTTP>;
using HttpClient = frog::Client<frog::HTTP>;

static void PrintLogo(const std::string &logo_path);

int main(const int argc, const char *argv[])
{
  ConfigParser *conf_parser = nullptr;
  try {
    conf_parser = new ConfigParser("conf/MainConf.xml");
  } catch (const exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  PrintLogo(conf_parser->GetLogoPath());
  
  int server_port = conf_parser->GetServerPort();
  int threads_count = conf_parser->GetThreadsCount();

  cout << server_port << " " << threads_count << endl;
}

void PrintLogo(const std::string &logo_path) {
  cout << "logo locate at " << logo_path << endl;
}

