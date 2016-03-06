#ifndef FROG_SRC_SERVERBASE_H
#define FROG_SRC_SERVERBASE_H
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <unordered_map>
#include <thread>
#include <regex>
#include <functional>
#include <iostream>
#include <sstream>

namespace frog
{
template<class SocketType>
class ServerBase {
 public:
  class Response : public std::ostream {
    friend class ServerBase<SocketType>;
   public:
    size_t size() {
      return stream_buf_.size();
    }

    void flush() {
      boost::system::error_code ec;
      // Use coroutine to replace the error handler.
      boost::asio::async_write(sock_, stream_buf_, yield_[ec]);

      if (ec) {
        throw std::runtime_error(ec.message());
      }
    }

   private:
    Response(SocketType &sock, boost::asio::yield_context &yield)
      : std::ostream(&stream_buf_), yield_(yield), sock_(sock)
    {
    }

    boost::asio::streambuf stream_buf_;
    boost::asio::yield_context &yield_;
    SocketType &sock_;
  };
  
  class Content : public std::istream {
    friend class ServerBase<SocketType>;
   public:
    size_t size() {
      return stream_buf_.size();
    }

    std::string string() {
      std::stringstream ss;
      ss << this->rdbuf();
      return ss.str();
    }

   private:
    Content(boost::asio::streambuf &stream_buf)
      : std::istream(&stream_buf), stream_buf_(stream_buf)
    {
    }

    boost::asio::streambuf &stream_buf_;
  };
  
  // http request format:
  // <method> <request-URL> <version>
  // <headers>
  //
  // <entity-body>
  class Request {
    friend class ServerBase<SocketType>;
   public:
    std::string method, path, http_version;

    Content content;
    
    std::unordered_multimap<std::string, std::string> headers;

    std::smatch path_match;

    std::string remote_endpoint_address;
    unsigned short remote_endpoint_port;

   private:
    Request(boost::asio::io_service &the_io_service)
      : content(stream_buf_), strand_(the_io_service)
    {
    }

    void ReadRemoteEndpointData(SocketType &sock) {
      try {
        remote_endpoint_address = sock.lowest_layer().remote_endpoint().address().to_string();
        remote_endpoint_port = sock.lowest_layer().remote_endpoint().port();
      } catch (const std::exception &e) {
        std::cerr << "exception caught: " << e.what() << '\n';
        return;
      }
    }

    boost::asio::streambuf stream_buf_;
    boost::asio::strand strand_;
  };
  
  class Config {
    friend class ServerBase<SocketType>;
   public:
    // IPv4 address(In dotted format) or IPv6(In hexadecimal notation).
    // If empty, the address will be any address.
    std::string address;
    
    // Set this flag to false to avoid binding the socket to an address that
    // is already in use.
    bool reuse_address;

   private:
    Config(unsigned short port, size_t num_threads)
      : reuse_address(true), port_(port), num_threads_(num_threads) 
    {
    }

    unsigned short port_;
    size_t num_threads_;
  };

  // The follwing should be set before calling Start().
  Config config;
  
  using ResourceMethod =  std::function<void(typename ServerBase<SocketType>::Response&, 
                                        std::shared_ptr<typename ServerBase<SocketType>::Request>)>;

  std::unordered_map<std::string /*url_path*/, std::unordered_map<std::string/*http_method*/, ResourceMethod>> resource;
  
  std::unordered_map<std::string/*http_method*/, ResourceMethod> default_resource;
  
 protected:
  boost::asio::io_service the_io_service;
  boost::asio::ip::tcp::acceptor the_acceptor;
  std::vector<std::thread> threads;

  size_t timeout_request;
  size_t timeout_content;

 private:
  // E.g. "GET" "path_regex" 
  std::vector<std::pair<std::string/*http_method*/, std::vector<std::pair<std::regex/*url_path*/, ResourceMethod>>>> opt_resource_;

 public:
  void Start() {
    // Copy the resources to opt_resource_ for more efficient request processing.
    opt_resource_.clear();
    for (auto &res : resource) {
      // res : std::pair<std::string, std::unordered_map<std::string, ResouceMethod>>
      for (auto &res_method : res.second) {
        // res_method : std::pair<std::string, ResourceMethod>
        auto iter = opt_resource_.end();

        for (auto opt_iter = opt_resource_.begin(); opt_iter != opt_resource_.end(); ++opt_iter) {
          if (res_method.first == opt_iter->first) {
            iter = opt_iter;
            break;
          }
        }
          if (iter == opt_resource_.end()) {
            opt_resource_.emplace_back();
            iter = opt_resource_.begin() + (opt_resource_.size() - 1);
            iter->first = res_method.first;
          }
          // iter->second : std::vector<std::pair<std::regex, ResouceMethod>>
          iter->second.emplace_back(std::regex(res.first), res_method.second);
      }
    }
    
    if (the_io_service.stopped())
      the_io_service.reset();
    
    boost::asio::ip::tcp::endpoint the_endpoint;
    if (config.address.size() > 0)
      the_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(config.address), config.port_);
    else 
      the_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port_);
    
    the_acceptor.open(the_endpoint.protocol());
    the_acceptor.set_option(boost::asio::socket_base::reuse_address(config.reuse_address));
    the_acceptor.bind(the_endpoint);
    the_acceptor.listen();
    
    Accept();
    
    //if num_threads > 1, start m_io_service.run in (num_threads - 1) for thread polling.
    threads.clear();
    for (size_t i = 1; i < config.num_threads_; ++i) {
      threads.emplace_back([this]() {
          the_io_service.run();
      });
    }

    // Main thread
    the_io_service.run();

    // Wait for the rest of the threads, if any, to finish as well.
    for (auto &t : threads) {
      t.join();
    }
  }

  void Stop() {
    the_acceptor.close();
    the_io_service.stop();
  }
  
 protected:
  ServerBase(unsigned short port, size_t num_threads, size_t timeout_request, size_t timeout_send_or_receive)
    : config(port, num_threads), the_acceptor(the_io_service), 
      timeout_request(timeout_request), timeout_content(timeout_send_or_receive)
  {
  }

  virtual void Accept() = 0;

  std::shared_ptr<boost::asio::deadline_timer> SetTimeoutOnSocket(std::shared_ptr<SocketType> sock, size_t seconds) {
    std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(the_io_service));
    timer->expires_from_now(boost::posix_time::seconds(seconds));
    timer->async_wait([sock](const boost::system::error_code &ec) {
        if (!ec) {
          boost::system::error_code ec;
          sock->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
          sock->lowest_layer().close();
        }
    });
    return timer;
  }

  std::shared_ptr<boost::asio::deadline_timer> SetTimeoutOnSocket(std::shared_ptr<SocketType> sock, 
      std::shared_ptr<Request> request, size_t seconds) {
    std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(the_io_service));
    timer->expires_from_now(boost::posix_time::seconds(seconds));
    timer->async_wait(request->strand_.wrap([sock](const boost::system::error_code &ec) {
        if (!ec) {
          boost::system::error_code ec;
          sock->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
          sock->lowest_layer().close();
          }
    }));
    return timer;
  } 
  
  void ReadRequestAndContent(std::shared_ptr<SocketType> sock) {
    // Create new streambuf (Request::streambuf) for async_read_until()
    // shared_ptr is used to pass temporary objects to the asynchronus functions.
    std::shared_ptr<Request> request(new Request(the_io_service));
    request->ReadRemoteEndpointData(*sock);

    // Set timeout on the following boost::asio::async-read or write function
    std::shared_ptr<boost::asio::deadline_timer> timer;
    if (timeout_request > 0) {
      timer = SetTimeoutOnSocket(sock, timeout_request);
    }

    boost::asio::async_read_until(*sock, request->stream_buf_, "\r\n\r\n",
        [this, sock, request, timer](const boost::system::error_code &ec, size_t bytes_transferred) {
      if (timeout_request > 0) 
        timer->cancel();

      if (!ec) {
        // request->stream_buf_.size() is not neccessarilly the same as bytes_transferred, 
        // Boost doc remarks that : After a successful async_read_until operation, the streambuf may
        // contain additional data beyond that which matched the regular expression. An application 
        // will typically leave that data in the streambuf for a subsequent async_read_until operation to examine.
        size_t additional_bytes = request->stream_buf_.size() - bytes_transferred;

        ParseRequest(request, request->content);

        // If content, read that as well
        const auto iter = request->headers.find("Content-Length");
        if (iter != request->headers.end()) {
          // Set timeout on the following boost::asio::async_read or async_write function
          std::shared_ptr<boost::asio::deadline_timer> timer;
          if (timeout_content > 0) {
            timer = SetTimeoutOnSocket(sock, timeout_content);
          }
          unsigned long long content_length;
          try {
            content_length = std::stoull(iter->second);
          } catch (const std::exception &e) {
            std::cerr << "exception caught: " << e.what() << '\n';
            return;
          }

          if (content_length > additional_bytes) {
            boost::asio::async_read(*sock, request->stream_buf_,
                boost::asio::transfer_exactly(content_length - additional_bytes),
                [this, sock, request, timer](const boost::system::error_code &ec, size_t /* bytes_transferred*/) {
              if (timeout_content > 0)
                timer->cancel();
              if (!ec)
                FindResource(sock, request);
               });
          } else {
            if (timeout_content > 0)
              timer->cancel();
            FindResource(sock, request);
          }
        } else {
          FindResource(sock, request);
        }
      }
    });
  } // Function ReadRequestAndContent
  
  void ParseRequest(std::shared_ptr<Request> request, std::istream &stream) const {
    std::string line;
    //request first line: <method> <request-URL> <version>
    getline(stream, line); 
    size_t method_end;
    if ((method_end = line.find(' ')) != std::string::npos) {
      size_t path_end;
      if ((path_end = line.find(' ', method_end + 1)) != std::string::npos) {
        request->method = line.substr(0, method_end);
        request->path = line.substr(method_end + 1, path_end - method_end - 1);
        if ((path_end + 6) < line.size())
          request->http_version = line.substr(path_end + 6, line.size() - (path_end + 6) - 1);
        else 
          request->http_version = "1.0";

        getline(stream, line);
        size_t param_end;
        while ((param_end = line.find(':')) != std::string::npos) {
          size_t value_start = param_end + 1;
          if (value_start < line.size()) {
            if (line[value_start] == ' ')
              ++value_start;
            if (value_start < line.size())
              // you may also use std::make_pair
              request->headers.insert({line.substr(0, param_end), 
                                       line.substr(value_start, line.size() - value_start - 1)});
           }
          getline(stream, line);
        }
      }
    }
  } 

  void FindResource(std::shared_ptr<SocketType> sock, std::shared_ptr<Request> request) {
    // Find path and method match, and call WriteResponse
    for (auto &res : opt_resource_) {
      // res : std::pair<std::string, std::unordered_map<std::string, ResourceMethod>>
      if (request->method == res.first) {
        for (auto &res_path : res.second) {
        // res_path : std::pair<std::string, ResourceMethod>
          std::smatch sm_res;
          if (std::regex_match(request->path, sm_res, res_path.first)) {
            request->path_match = std::move(sm_res);
            WriteResponse(sock, request, res_path.second);
            return;
          }
        }
      }
    }
    auto method_iter = default_resource.find(request->method);
    if (method_iter != default_resource.end()) {
      WriteResponse(sock, request, method_iter->second);
    }
  }

  void WriteResponse(std::shared_ptr<SocketType> sock, std::shared_ptr<Request> request,
                     ResourceMethod &resource_function) {
    //Set timeout on the following boost::asio::async_read or async_write function.
    std::shared_ptr<boost::asio::deadline_timer> timer;
    if (timeout_content > 0)
      timer = SetTimeoutOnSocket(sock, request, timeout_content);

    boost::asio::spawn(request->strand_, 
        [this, &resource_function, sock, request, timer](boost::asio::yield_context yield) {
      Response response(*sock, yield);

      try {
        resource_function(response, request);
      } catch (const std::exception &e) {
        std::cerr << "exception caught: " << e.what() << '\n';
        return;
      }

      if (response.size() > 0) {
        try {
          response.flush();
        } catch (const std::exception &e) {
          std::cerr << "exception caught: " << e.what() << '\n';
          return;
        }
      }

      if (timeout_content > 0) 
        timer->cancel();
      float http_version;
      try {
        http_version = std::stof(request->http_version);
      } catch (const std::exception &e) {
        std::cerr << "exception caught: " << e.what() << '\n';
        return;
      }
      if (http_version > 1.05)
        ReadRequestAndContent(sock);
      });
  }

}; // class ServerBase 

} // namespace frog
#endif // FROG_SRC_SERVERBASE_H 
