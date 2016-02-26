#ifndef FROG_SRC_CLIENTBASE_H
#define FROG_SRC_CLIENTBASE_H
#include <boost/asio.hpp>
#include <boost/utility/string_ref.hpp>

#include <unordered_map>
#include <map>
#include <random>
namespace frog
{
template <class SocketType>
class ClientBase 
{
 public:
  class Response 
  {
    friend class ClientBase<SocketType>;
   public:
    std::string http_version, status_code;
    std::istream content;
    std::unordered_map<std::string, std::string> headers;
   private:
    Response() : content(&content_buffer_)
    {
    }
    boost::asio::streambuf content_buffer_;
  };

  std::shared_ptr<Response> Request(const std::string &request_type, 
                                    const std::string &path = "/",
                                    boost::string_ref content = "",
                                    const std::map<std::string, std::string> &headers = std::map<std::string, std::string>()) {
    std::string corrected_path = path;
    if (corrected_path == "")
      corrected_path = "/";

    boost::asio::streambuf write_buffer;
    std::ostream write_stream(&write_buffer);
    write_stream << request_type << " " << corrected_path << " HTTP/1.1\r\n";
    write_stream << "Host: " << host << "\r\n";
    for (auto &h : headers) {
      write_stream << h.first << ": " << h.second << "\r\n";
    }

    if (content.size() > 0)
      write_stream << "Content-Length: " << content.size() << "\r\n";
    write_stream << "\r\n";

    try {
      Connect();

      boost::asio::write(*sock, write_buffer);
      if (content.size() > 0)
        boost::asio::write(*sock, boost::asio::buffer(content.data(), content.size()));
    } catch (const std::exception &e) {
      socket_error = true;
      throw std::invalid_argument(e.what());
    }

    return RequestRead();
  }

  std::shared_ptr<Response> Request(const std::string &request_type, 
                                    const std::string &path,
                                    std::iostream &content,
                                    const std::map<std::string, std::string> &headers = std::map<std::string, std::string>()) {
    std::string corrected_path = path;
    if (corrected_path == "")
      corrected_path = "/";

    content.seekp(0, std::ios::end);
    size_t content_length = content.tellp();
    content.seekp(0, std::ios::beg);

    boost::asio::streambuf write_buffer;
    std::ostream write_stream(&write_buffer);
    write_stream << request_type << " " << corrected_path << " HTTP/1.1\r\n";
    write_stream << "Host: " << host << "\r\n";
    for (auto &h : headers) {
      write_stream << h.first << ": " << h.second << "\r\n";
    }
    if (content_length > 0)
      write_stream << "Content-Length: " << content_length << "\r\n";
    write_stream << "\r\n";
    if (content_length > 0)
      write_stream << content.rdbuf();

    try {
      Connect();

      boost::asio::write(*sock, write_buffer);
    } catch (const std::exception &e) {
      socket_error = true;
      throw std::invalid_argument(e.what());
    }

    return RequestRead();
  }
  
 protected:
  ClientBase(const std::string &host_port, unsigned short default_port)
    : the_resolver(the_io_service), socket_error(false)
  {
    size_t host_end = host_port.find(':');
    if (host_end == std::string::npos) {
      host = host_port;
      port = default_port;
    } else {
      host = host_port.substr(0, host_end);
      port = (unsigned short)std::stoul(host_port.substr(host_end + 1));
    }
    the_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
  }

  boost::asio::io_service the_io_service;
  boost::asio::ip::tcp::endpoint the_endpoint;
  boost::asio::ip::tcp::resolver the_resolver;

  std::shared_ptr<SocketType> sock;
  bool socket_error;
  
  std::string host;
  unsigned short port;
  
  virtual void Connect() = 0;
  
  // Reponse format
  // <version> <status> <reason-phrase>
  // <headerss>
  //
  // <entity-body>
  void ParseResponseHeader(std::shared_ptr<Response> response, std::istream &stream) const {
    std::string line;
    getline(stream, line);
    size_t version_end = line.find(' ');
    if (version_end != std::string::npos) {
      if (line.size() > 5) // Means HTTP/ 
        response->http_version = line.substr(5, version_end - 5);
      if ((version_end + 1) < line.size())
        response->status_code = line.substr(version_end + 1, line.size() - (version_end + 1) - 1);

      getline(stream, line);
      size_t param_end;
      while ((param_end = line.find(':')) != std::string::npos) {
        size_t value_start = param_end + 1;
        if (value_start < line.size()) {
          if (line[value_start] == ' ')
            ++value_start;
          if (value_start < line.size())
            response->headers.insert({line.substr(0, param_end), 
                                     line.substr(value_start, line.size() - value_start - 1)});
        }
        getline(stream, line);
      }
    }
  }

  std::shared_ptr<Response> RequestRead() {
    std::shared_ptr<Response> response(new Response());

    try {
      size_t bytes_transferred = boost::asio::read_until(*sock, response->content_buffer_, "\r\n\r\n");

      size_t additional_bytes = response->content_buffer_.size() - bytes_transferred;
      
      ParseResponseHeader(response, response->content);

      if (response->headers.count("Content-Length") > 0) {
        auto content_length = std::stoull(response->headers["Content-Length"]);
        if (content_length > additional_bytes) {
          boost::asio::read(*sock, response->content_buffer_,
                            boost::asio::transfer_exactly(content_length - additional_bytes));
        }
      } else if (response->headers.count("Transfer-Encoding") > 0 && response->headers["Transfer-Encoding"] == "chunked") {
        boost::asio::streambuf streambuf;
        std::ostream content_stream(&streambuf);

        size_t curr_length;
        std::string str_buffer;
        do {
          size_t bytes_transferred = boost::asio::read_until(*sock, response->content_buffer_, "\r\n");
          std::string line;
          getline(response->content, line);
          bytes_transferred -= (line.size() + 1);
          line.pop_back(); // remove the '\r' character
          curr_length = std::stoull(line, 0, 16);

          size_t additional_bytes = response->content_buffer_.size() - bytes_transferred;
          
          // Read the chunked block(size equals curr_length).
          if (additional_bytes < (2 + curr_length)) {
            boost::asio::read(*sock, response->content_buffer_, 
                              boost::asio::transfer_exactly(2 + curr_length - additional_bytes));
          }

          str_buffer.resize(curr_length);
          response->content.read(&str_buffer[0], curr_length);
          content_stream.write(&str_buffer[0], curr_length);

          // Remove "\r\n"
          response->content.get();
          response->content.get();
        } while (curr_length > 0);

        std::ostream response_content_output_stream(&response->content_buffer_);
        response_content_output_stream << content_stream.rdbuf();
      }
    } catch (const std::exception &e) {
      socket_error = true;
      throw std::invalid_argument(e.what());
    }

    return response;
  }
}; // template class ClientBase


} // namespace frog

#endif // FROG_SRC_CLIENTBASE_H
