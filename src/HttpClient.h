#ifndef FROG_SRC_HTTPCLIENT_H
#define FROG_SRC_HTTPCLIENT_H
#include "ClientBase.h"

namespace frog
{
template<class SocketType>
class Client : public ClientBase<SocketType>
{
};

typedef boost::asio::ip::tcp::socket HTTP;

template<>
class Client<HTTP> : public ClientBase<HTTP>
{
 public:
  Client(const std::string &server_port_path)
    : ClientBase<HTTP>::ClientBase(server_port_path, 80)
  {
    sock = std::make_shared<HTTP>(the_io_service);
  }

 private:
  void Connect() {
    if (socket_error || !sock->is_open()) {
      boost::asio::ip::tcp::resolver::query the_query(host, std::to_string(port));
      boost::asio::connect(*sock, the_resolver.resolve(the_query));

      boost::asio::ip::tcp::no_delay option(true);
      sock->set_option(option);

      socket_error = false;
    }
  }
};

} // namespace frog

#endif // FROG_SRC_HTTPCLIENT_H
