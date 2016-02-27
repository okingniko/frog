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
namespace fs = boost::filesystem;

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
  
  unsigned short server_port = conf_parser->GetServerPort();
  int threads_count = conf_parser->GetThreadsCount();
  
  HttpServer server(server_port, threads_count);
  
  // A POST_example, for the path /string, resonds the client with the posted string.
  server.resource["^/string$"]["POST"] = [](HttpServer::Response &response, shared_ptr<HttpServer::Request> request) {
    // Retrieve string;
    auto content = request->content.string();
    response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
  };

  // Default GET-example. 
  server.default_resource["GET"] = [conf_parser](HttpServer::Response &response, shared_ptr<HttpServer::Request> request) {
    string document_root = conf_parser->GetDocumentRoot();
    fs::path web_root_path(document_root);
    if (!fs::exists(web_root_path)) {
      cerr << "Counld not find web root." << endl;
    } else {
      auto path = web_root_path;
      path += request->path;
      if (fs::exists(path)) {
        if (web_root_path <= path) {
          if (fs::is_directory(path))
            path += "/index.html";
          if (fs::is_regular_file(path)) {
            ifstream ifs;
            ifs.open(path.string(), ios::in | ios::binary);

            if (ifs.good()) {
              ifs.seekg(0, ios::end);
              size_t length = ifs.tellg();
              ifs.seekg(0, ios::beg);

              response << "HTTP/1.1 200 OK\r\nContent-Length: " << length 
                       << "\r\n\r\n";

              // read and send 128 KB at a time
              size_t buffer_size = 1 << 17;
              vector<char> buffer;
              buffer.reserve(buffer_size);
              size_t read_length;

              try {
                while ((read_length = ifs.read(&buffer[0], buffer_size).gcount()) > 0) {
                  response.write(&buffer[0], read_length);
                  response.flush();
                }
              } catch (const exception &e) {
                cerr << "MESSAGE: Connection interrupted, closing file " << path.string() << endl;
              }

              ifs.close();
              return;
            }
          }
        }
      }
    }
    string content = "Could not open path " + request->path;
    response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length()
               << "\r\n\r\n";
  };
    
  server.Start();

}

void PrintLogo(const std::string &logo_path) {
  cout << "logo locate at " << logo_path << endl;
}

