#pragma once

#include <iostream>
#include <string>

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
