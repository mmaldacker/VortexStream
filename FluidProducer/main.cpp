#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>
#include <rabbitmq-c/tcp_socket.h>

void check_result(amqp_rpc_reply_t x, char const* context)
{
  switch (x.reply_type)
  {
    case AMQP_RESPONSE_NORMAL:
      return;

    case AMQP_RESPONSE_NONE:
      std::cerr << context << ": missing RPC reply type!\n";
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      std::cerr << context << ": " << amqp_error_string2(x.library_error) << "\n";
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (x.reply.id)
      {
        case AMQP_CONNECTION_CLOSE_METHOD:
        {
          auto* m = (amqp_connection_close_t*)x.reply.decoded;
          std::string reply_msg((char*)m->reply_text.bytes, m->reply_text.len);
          std::cerr << context << ": server connection error " << m->reply_code
                    << ", message: " << reply_msg << "\n";
          break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD:
        {
          auto* m = (amqp_channel_close_t*)x.reply.decoded;
          std::string reply_msg((char*)m->reply_text.bytes, m->reply_text.len);
          std::cerr << context << ": server channel error " << m->reply_code
                    << ", message: " << reply_msg << "\n";
          break;
        }
        default:
          std::cerr << context << ": unknown server error, method id " << x.reply.id << "\n";
          break;
      }
      break;
  }

  throw std::runtime_error("fatal rabbitmq error");
}

amqp_bytes_t to_bytes(const std::string& s)
{
  amqp_bytes_t bytes = amqp_bytes_malloc(s.size());
  std::memcpy(bytes.bytes, s.data(), s.size());
  bytes.len = s.length();

  return bytes;
}

// Simple wrapper for a single connection with a single channel and a single queue
class Queue
{
public:
  Queue(const char* hostname, int port)
  {
    conn_ = amqp_new_connection();
    socket_ = amqp_tcp_socket_new(conn_);
    if (!socket_)
    {
      throw std::runtime_error("creating TCP socket");
    }

    int status = amqp_socket_open(socket_, hostname, port);
    if (status)
    {
      throw std::runtime_error("opening TCP socket");
    }

    auto result = amqp_login(conn_, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    check_result(result, "Logging in");

    amqp_channel_open(conn_, 1);
    result = amqp_get_rpc_reply(conn_);
    check_result(result, "Opening channel");
  }

  void Bind(const std::string& queue_name = "")
  {
    amqp_bytes_t queue_names_bytes = amqp_empty_bytes;
    if (!queue_name.empty())
    {
      queue_names_bytes = to_bytes(queue_name);
    }

    auto* r = amqp_queue_declare(conn_, 1, queue_names_bytes, 0, 0, 0, 1, amqp_empty_table);
    amqp_bytes_free(queue_names_bytes);

    auto result = amqp_get_rpc_reply(conn_);
    check_result(result, "Declaring queue");
    queuename_ = amqp_bytes_malloc_dup(r->queue);
    if (queuename_.bytes == nullptr)
    {
      throw std::runtime_error("Out of memory while copying queue name");
    }

    // Queue is automatically bound to default exchange.
  }

  void Consume()
  {
    amqp_basic_consume(conn_, 1, queuename_, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    auto result = amqp_get_rpc_reply(conn_);
    check_result(result, "Consuming");

    while (true)
    {
      amqp_rpc_reply_t res;
      amqp_envelope_t envelope;

      amqp_maybe_release_buffers(conn_);

      res = amqp_consume_message(conn_, &envelope, NULL, 0);

      if (AMQP_RESPONSE_NORMAL != res.reply_type)
      {
        break;
      }

      std::string exchange((char*)envelope.exchange.bytes, (int)envelope.exchange.len);
      std::string routing_key((char*)envelope.routing_key.bytes, (int)envelope.routing_key.len);

      std::cout << "Delivery " << (unsigned)envelope.delivery_tag << ", exchange " << exchange
                << " routingkey " << routing_key << "\n";

      if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
      {
        std::string content_type((char*)envelope.message.properties.content_type.bytes,
                                 (int)envelope.message.properties.content_type.len);
        std::cout << "Content-type: " << content_type << "\n";
      }

      std::cout << "----\n";

      // amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

      amqp_destroy_envelope(&envelope);
    }
  }

  void Publish()
  {
    amqp_bytes_t message_bytes;
    auto type = amqp_cstring_bytes("amq.direct");
    int result = amqp_basic_publish(conn_, 1, type, queuename_, 0, 0, NULL, message_bytes);
    if (result)
    {
      std::runtime_error("error publishing");
    }
  }

  ~Queue()
  {
    amqp_bytes_free(queuename_);

    auto result = amqp_channel_close(conn_, 1, AMQP_REPLY_SUCCESS);
    check_result(result, "Closing channel");
    result = amqp_connection_close(conn_, AMQP_REPLY_SUCCESS);
    check_result(result, "Closing connection");
    int status = amqp_destroy_connection(conn_);
    if (status)
    {
      std::cerr << "Error destroying connection\n";
    }
  }

  amqp_socket_t* socket_ = nullptr;
  amqp_connection_state_t conn_;
  amqp_bytes_t queuename_;
};

int main(int argc, char const* const* argv)
{
  const char* hostname = "localhost";
  int port = 5672;

  const std::string queue_name = "fluid_request";

  try
  {
    Queue queue(hostname, port);
    queue.Bind(queue_name);
    queue.Consume();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
  }
}
