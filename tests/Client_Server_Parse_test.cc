#include "HttpServer.h"
#include "HttpClient.h"

#include <iostream>
#include <sstream>

using namespace frog;

class ServerTest : public ServerBase<HTTP>
{
 public:
  ServerTest()
    : ServerBase<HTTP>::ServerBase(8888, 1, 5, 300)
  {
  }

  void Accept() {
  }

  bool ParseRequestTest() {
    std::shared_ptr<Request> request(new Request(the_io_service));

    std::stringstream ss;
    ss << "GET /test/ HTTP/1.1\r\n";
    ss << "TestHeader: haha\r\n";
    ss << "TestHeader2: oops\r\n";
    ss << "\r\n";

    ParseRequest(request, ss);

    if (request->method != "GET")
      return false;
    if (request->path != "/test/")
      return false;
    if (request->http_version != "1.1")
      return false;

    if (request->headers.size() != 2)
      return false;
    auto headers_iter = request->headers.find("TestHeader");
    if (headers_iter == request->headers.end() || headers_iter->second != "haha")
      return false;
    headers_iter = request->headers.find("TestHeader2");
    if (headers_iter == request->headers.end() || headers_iter->second != "oops")
      return false;

    return true;
  }
};

class ClientTest : public ClientBase<HTTP> 
{
 public:
  ClientTest(const std::string &server_port_path)
    : ClientBase<HTTP>::ClientBase(server_port_path, 80)
  {
  }

  void Connect() {
  }

  bool ServerPortParseTest1() {
    if (host != "test.com")
      return false;
    if (port != 8888)
      return false;

    return true;
  }

  bool ServerPortParseTest2() {
    if (host != "test.com")
      return false;
    if (port != 80)
      return false;
    
    return true;
  }

  bool ParseResponseHeaderTest() {
    std::shared_ptr<Response> response(new Response());

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "TestHeader: haha\r\n";
    ss << "TestHeader2: oops\r\n";
    ss << "\r\n";

    ParseResponseHeader(response, ss);

    if (response->http_version != "1.1")
      return false;
    if (response->status_code != "200 OK")
      return false;

    if (response->headers.size() != 2)
      return false;
    auto headers_iter = response->headers.find("TestHeader");
    if (headers_iter == response->headers.end() || headers_iter->second != "haha")
      return false;
    headers_iter = response->headers.find("TestHeader2");
    if (headers_iter == response->headers.end() || headers_iter->second != "oops")
      return false;

    return true;
  }
};

int main(const int argc, const char *argv[])
{
  ServerTest server_test;

  if (!server_test.ParseRequestTest()) {
    std::cerr << "ERROR: HttpServer::ParseRequest " << std::endl;
    return 1;
  }

  ClientTest client_test1("test.com:8888");
  if (!client_test1.ServerPortParseTest1()) {
    std::cerr << "ERROR: HttpClient::Client constructor failed " << std::endl;
    return 1;
  }

  ClientTest client_test2("test.com");
  if (!client_test2.ServerPortParseTest2()) {
    std::cerr << "ERROR: HttpClient::Client construtor failed " << std::endl;
    return 1;
  }

  if (!client_test2.ParseResponseHeaderTest()) {
    std::cerr << "ERROR: HttpClient::ParseResponse failed " << std::endl;
    return 1;
  }

  return 0;
}

