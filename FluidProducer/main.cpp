
#include "Queue.hpp"

#include "messages.pb.h"

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
      VortexStream::Request request;
      request.ParseFromArray(envelope.message.body.bytes, envelope.message.body.len);

      std::cout << "Requested " << request.num_frames() << " num frames" << std::endl;

      VortexStream::Frame frame;
      frame.set_width(100);
      frame.set_height(100);
      frame.set_frame_id(1);

      for (int i = 0; i < 100; i++)
      {
        for (int j = 0; j < 100; j++)
        {
          frame.add_pixels(1.0);
        }
      }

      auto data = amqp_bytes_malloc(frame.ByteSizeLong());
      frame.SerializePartialToArray(data.bytes, data.len);

      queue.Publish(
          envelope.message.properties.reply_to, envelope.message.properties.correlation_id, data);
    });
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
  }
}
