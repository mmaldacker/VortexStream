
#include "Queue.hpp"

#include "messages.pb.h"

#include <Vortex/Vortex.h>

amqp_bytes_t MakeFrame(int id, std::vector<glm::u8vec4>& data)
{
  VortexStream::Frame frame;
  frame.set_width(100);
  frame.set_height(100);
  frame.set_frame_id(id);

  for (auto& v : data)
  {
    frame.add_pixels(v.r / 255.0);
  }

  auto output = amqp_bytes_malloc(frame.ByteSizeLong());
  frame.SerializePartialToArray(output.bytes, output.len);

  return output;
}

int main(int argc, char const* const* argv)
{
  const char* hostname = "localhost";
  int port = 5672;  // default port

  const std::string queue_name = "fluid_request";

  Vortex::Renderer::Instance instance("FluidProducer", {}, true);
  Vortex::Renderer::Device device(instance);

  Vortex::Fluid::SmokeWorld world(
      device, {100, 100}, 0.033, Vortex::Fluid::Velocity::InterpolationMode::Linear);
  Vortex::Fluid::Density density(device, {100, 100}, vk::Format::eR8G8B8A8Unorm);
  world.FieldBind(density);

  Vortex::Renderer::RenderTexture target(device, 100, 100, vk::Format::eR8G8B8A8Unorm);
  Vortex::Renderer::Texture localTexture(
      device, 100, 100, vk::Format::eR8G8B8A8Unorm, VMA_MEMORY_USAGE_CPU_ONLY);
  std::vector<glm::u8vec4> data(100 * 100);

  auto renderer = target.Record({density});

  // Draw liquid boundaries
  Vortex::Renderer::Rectangle area(device, glm::ivec2(256) - glm::ivec2(4));
  area.Colour = glm::vec4(-1);
  area.Position = glm::vec2(2.0f);

  Vortex::Renderer::Clear clearLiquid({1.0f, 0.0f, 0.0f, 0.0f});

  world.RecordLiquidPhi({clearLiquid, area}).Submit().Wait();

  // Draw input
  Vortex::Renderer::Rectangle source(device, glm::vec2(20.0f));
  Vortex::Renderer::Rectangle force(device, glm::vec2(20.0f));

  source.Position = force.Position = {50.0f, 25.0f};
  source.Anchor = force.Anchor = glm::vec2(10.0);

  source.Colour = {1.0f, 1.0f, 1.0f, 1.0f};
  force.Colour = {0.0f, 30.0f, 0.0f, 0.0f};

  // Draw sources and forces
  auto velocityRender = world.RecordVelocity({force}, Vortex::Fluid::VelocityOp::Set);
  auto densityRender = density.Record({source});

  try
  {
    Queue queue(hostname, port);
    queue.Create(queue_name);
    queue.Consume(
        [&](amqp_envelope_t envelope)
        {
          VortexStream::Request request;
          request.ParseFromArray(envelope.message.body.bytes, envelope.message.body.len);

          std::cout << "Requested " << request.num_frames() << " num frames" << std::endl;

          for (int i = 0; i < request.num_frames(); i++)
          {
            velocityRender.Submit();
            densityRender.Submit();

            auto params = Vortex::Fluid::FixedParams(12);
            world.Step(params);
            renderer.Submit();

            device.Execute([&](vk::CommandBuffer cmd) { localTexture.CopyFrom(cmd, target); });
            localTexture.CopyTo(data);

            std::cout << "Sending frame " << i << std::endl;

            auto f = MakeFrame(i, data);
            queue.Publish(envelope.message.properties.reply_to,
                          envelope.message.properties.correlation_id,
                          f);
          }
        });
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
  }
}
