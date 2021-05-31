#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>
#include <rabbitmq-c/tcp_socket.h>

amqp_bytes_t to_bytes(const std::string& s)
{
  amqp_bytes_t bytes = amqp_bytes_malloc(s.size());
  std::memcpy(bytes.bytes, s.data(), s.size());
  bytes.len = s.length();

  return bytes;
}

std::string to_string(amqp_bytes_t b)
{
  return {(char*)b.bytes, b.len};
}

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
          std::cerr << context << ": server connection error " << m->reply_code
                    << ", message: " << to_string(m->reply_text) << "\n";
          break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD:
        {
          auto* m = (amqp_channel_close_t*)x.reply.decoded;
          std::cerr << context << ": server channel error " << m->reply_code
                    << ", message: " << to_string(m->reply_text) << "\n";
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

    amqp_channel_open(conn_, channel_);
    result = amqp_get_rpc_reply(conn_);
    check_result(result, "Opening channel");
  }

  void Create(const std::string& queue_name = "")
  {
    amqp_bytes_t queue_names_bytes = amqp_empty_bytes;
    if (!queue_name.empty())
    {
      queue_names_bytes = to_bytes(queue_name);
    }

    auto* r = amqp_queue_declare(conn_, channel_, queue_names_bytes, 0, 0, 0, 1, amqp_empty_table);
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

  void Consume(const std::function<void(amqp_envelope_t)>& callback)
  {
    amqp_basic_consume(conn_, channel_, queuename_, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    auto result = amqp_get_rpc_reply(conn_);
    check_result(result, "Consuming");

    while (true)
    {
      amqp_maybe_release_buffers(conn_);

      amqp_envelope_t envelope;
      auto result = amqp_consume_message(conn_, &envelope, nullptr, 0);
      if (AMQP_RESPONSE_NORMAL != result.reply_type)
      {
        break;
      }

      std::cout << "Delivery " << (unsigned)envelope.delivery_tag << ", exchange "
                << to_string(envelope.exchange) << " routingkey " << to_string(envelope.routing_key)
                << "\n";

      if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
      {
        std::cout << "Content-type: " << to_string(envelope.message.properties.content_type)
                  << "\n";
      }

      if (envelope.message.properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG)
      {
        std::cout << "Correlation id: " << to_string(envelope.message.properties.correlation_id)
                  << "\n";
      }

      if (envelope.message.properties._flags & AMQP_BASIC_REPLY_TO_FLAG)
      {
        std::cout << "Reply to: " << to_string(envelope.message.properties.reply_to) << "\n";
      }

      std::cout << "----\n";

      callback(envelope);

      amqp_destroy_envelope(&envelope);
    }
  }

  void Publish(amqp_bytes_t queue_name, amqp_bytes_t correlation_id, amqp_bytes_t message_bytes)
  {
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CORRELATION_ID_FLAG;
    props.correlation_id = correlation_id;

    int result = amqp_basic_publish(
        conn_, channel_, amqp_empty_bytes, queue_name, 0, 0, &props, message_bytes);
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

  const int channel_ = 1;
  amqp_socket_t* socket_ = nullptr;
  amqp_connection_state_t conn_;
  amqp_bytes_t queuename_;
};

int main(int argc, char const* const* argv)
{
  const char* hostname = "localhost";
  int port = 5672;  // default port

  const std::string queue_name = "fluid_request";

  try
  {
    Queue queue(hostname, port);
    queue.Create(queue_name);
    queue.Consume([&](amqp_envelope_t envelope) {
      queue.Publish(envelope.message.properties.reply_to,
                    envelope.message.properties.correlation_id,
                    amqp_empty_bytes);
    });
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
  }
}
