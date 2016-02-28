#include "HttpServer.h"
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
  
  // A POST-example for the path /json, respond with the "firstname " + "secondname" from the posted json.
  // Respond with an appropriate error message if the posted json is invaild,
  // or if the firstname and secondname is missing.
  // Example posted json:
  // {
  //  "firstname" : "wang"
  //  "lastname" : "mumu"
  //  "age" : 22
  //  "sex" : "male"
  //  }
    server.resource["^/json$"]["POST"] = [](HttpServer::Response &response, shared_ptr<HttpServer::Request> request) {
      try {
        pt::ptree pt;
        pt::read_json(request->content, pt);

        string name = pt.get<string>("firstname") + " " + pt.get<string>("lastname");

        response << "HTTP/1.1 200 OK\r\nContent-Length: " << name.length() << "\r\n\r\n" << name;
      } catch(const exception &e) {
        response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what())
                 << "\r\n\r\n" << e.what();
      }
    };
  
  // GET-example for the /id/<int:id>, respond the content of the id.
  server.resource["^/id/([0-9]+)$"]["GET"] = [](HttpServer::Response &response, shared_ptr<HttpServer::Request> request) {
    string id = request->path_match[1];
    response << "HTTP/1.1 200 OK\r\nContent-Length: " << id.length() << "\r\n\r\n"
             << id;
  };

  // GET-example for the path /reqinfo
  // Responds with request information
  server.resource["^/reqinfo$"]["GET"] = [](HttpServer::Response &response, shared_ptr<HttpServer::Request> request) {
    stringstream content_stream;
    content_stream << "<h2>Request from " << request->remote_endpoint_address << ":" << request->remote_endpoint_port<< "</h2>";
    content_stream << request->method << " " << request->path << " HTTP/" << request->http_version << "<br>";
    for (auto &header : request->headers) {
      content_stream << header.first << ": " << header.second << "<br>";
    }

    content_stream.seekp(0, ios::end);
    response << "HTTP/1.1 200 OK\r\nContent-Length: " << content_stream.tellp()
             << "\r\n\r\n" << content_stream.rdbuf();
  };

  // Default GET-example.  
  server.default_resource["GET"] = [conf_parser](HttpServer::Response &response, shared_ptr<HttpServer::Request> request) {
    string document_root = conf_parser->GetDocumentRoot();
    fs::path web_root_path(document_root);
    if (!fs::exists(web_root_path)) {
      cerr << "Could not find web root." << endl;
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
               << "\r\n\r\n" << content;
  };
    
  server.Start();

}

void PrintLogo(const std::string &logo_path) {
  cout << "logo locate at " << logo_path << endl;
}

