#pragma once

#include "Utils.hpp"

#include <functional>

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

      callback(envelope);

      std::cout << "----\n";

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
      throw std::runtime_error("error publishing");
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
