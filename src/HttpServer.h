#ifndef FROG_SRC_HTTPSERVER_H
#define FROG_SRC_HTTPSERVER_H
#include "ServerBase.h"

namespace frog
{
template<class SocketType>
class Server : public ServerBase<SocketType>
{
};

using HTTP = boost::asio::ip::tcp::socket;

template<>
class Server<HTTP> : public ServerBase<HTTP>
{
 public:
  Server(unsigned short port, size_t num_threads = 1, 
         size_t timeout_request = 5 /*seconds*/, size_t timeout_content = 300 /*seconds*/)
    : ServerBase<HTTP>::ServerBase(port, num_threads, timeout_request, timeout_content)
  {
  }

 private:
  void Accept() override {
    // Create new socket for this connection
    // Shared_ptr is used to pass temporary objects to the asynchrous functions
    std::shared_ptr<HTTP> sock(new HTTP(the_io_service));

    the_acceptor.async_accept(*sock, 
        [this, sock](const boost::system::error_code &ec) {
      //Immediately start accepting a new connection.
      Accept();

      if (!ec) {
        // Formid  Nagle algorithm.
        boost::asio::ip::tcp::no_delay option(true);
        sock->set_option(option);

        ReadRequestAndContent(sock);
      }
    });
  }
}; // class Server<HTTP>
} // namespace frog
#endif // FROG_SRC_HTTPSERVER_H
