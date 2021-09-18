
#include "Queue.hpp"

#include <google/protobuf/arena.h>
#include "messages.pb.h"

#include <Vortex/Vortex.h>

#include <memory>

amqp_bytes_t MakeFrame(int id, int width, int height, const std::vector<glm::u8vec4>& data)
{
  VortexStream::Frame frame;
  frame.set_width(width);
  frame.set_height(height);
  frame.set_frame_id(id);

  for (auto& v : data)
  {
    frame.add_pixels(v.r / 255.0);
  }

  auto output = amqp_bytes_malloc(frame.ByteSizeLong());
  frame.SerializePartialToArray(output.bytes, output.len);

  return output;
}

struct Source
{
  glm::vec2 pos;
  glm::vec2 dir;
};

class Simulation
{
private:
  Vortex::Renderer::Instance instance;
  Vortex::Renderer::Device device;

public:
  Simulation() : instance("FluidProducer", {}, false), device(instance) {}

  void Simulate(int width,
                int height,
                int num_frames,
                std::vector<Source> sources,
                std::function<void(int, const std::vector<glm::u8vec4>&)> callback)
  {
    Vortex::Fluid::SmokeWorld world(
        device, {width, height}, 0.033, Vortex::Fluid::Velocity::InterpolationMode::Linear);
    auto density = std::make_shared<Vortex::Fluid::Density>(
        device, glm::vec2{width, height}, vk::Format::eR8G8B8A8Unorm);
    world.FieldBind(*density);

    Vortex::Renderer::RenderTexture target(device, width, height, vk::Format::eR8G8B8A8Unorm);
    Vortex::Renderer::Texture localTexture(
        device, width, height, vk::Format::eR8G8B8A8Unorm, VMA_MEMORY_USAGE_CPU_ONLY);
    std::vector<glm::u8vec4> data(width * height);

    auto renderer = target.Record({density});

    // Draw liquid boundaries
    auto area = std::make_shared<Vortex::Renderer::Rectangle>(
        device, glm::ivec2(width, height) - glm::ivec2(4));
    area->Colour = glm::vec4(-1);
    area->Position = glm::vec2(2.0f);

    auto clearLiquid = std::make_shared<Vortex::Renderer::Clear>(glm::vec4{1.0f, 0.0f, 0.0f, 0.0f});

    world.RecordLiquidPhi({clearLiquid, area}).Submit().Wait();

    std::vector<Vortex::Renderer::DrawablePtr> fluidSources;
    std::vector<Vortex::Renderer::DrawablePtr> fluidForces;

    // Draw input

    for (auto& source : sources)
    {
      auto fluidSource = std::make_shared<Vortex::Renderer::Rectangle>(device, glm::vec2(20.0f));
      auto fluidForce = std::make_shared<Vortex::Renderer::Rectangle>(device, glm::vec2(20.0f));

      fluidSource->Position = fluidForce->Position = source.pos;
      fluidSource->Anchor = fluidForce->Anchor = glm::vec2(10.0);

      fluidSource->Colour = {1.0f, 1.0f, 1.0f, 1.0f};
      fluidForce->Colour = {source.dir.x, source.dir.y, 0.0f, 0.0f};

      fluidForces.push_back(std::move(fluidForce));
      fluidSources.push_back(std::move(fluidSource));
    }

    // Draw sources and forces
    auto velocityRender = world.RecordVelocity(fluidForces, Vortex::Fluid::VelocityOp::Set);
    auto densityRender = density->Record(fluidSources);

    for (int i = 0; i < num_frames; i++)
    {
      velocityRender.Submit();
      densityRender.Submit();

      auto params = Vortex::Fluid::FixedParams(12);
      world.Step(params);
      renderer.Submit();

      device.Execute([&](vk::CommandBuffer cmd) { localTexture.CopyFrom(cmd, target); });
      localTexture.CopyTo(data);

      callback(i, data);
    }
  }
};

int main(int argc, char const* const* argv)
{
  const char* hostname = "localhost";
  int port = 5672;  // default port

  const std::string queue_name = "fluid_request";

  google::protobuf::Arena arena;
  Simulation simulation;
  try
  {
    Queue queue(hostname, port);
    queue.Create(queue_name);
    queue.Consume(
        [&](amqp_envelope_t envelope)
        {
          auto* request = google::protobuf::Arena::CreateMessage<VortexStream::Request>(&arena);
          request->ParseFromArray(envelope.message.body.bytes, envelope.message.body.len);

          std::cout << "Requested " << request->num_frames() << " num frames" << std::endl;

          std::vector<Source> sources;
          for (int i = 0; i < request->sources_size(); i++)
          {
            const auto& pos = request->sources(i).position();
            const auto& dir = request->sources(i).direction();

            sources.push_back({glm::vec2{pos.x(), pos.y()}, glm::vec2{dir.x(), dir.y()}});
          }

          simulation.Simulate(request->width(),
                              request->height(),
                              request->num_frames(),
                              sources,
                              [&](int i, const std::vector<glm::u8vec4>& data)
                              {
                                auto f = MakeFrame(i, request->width(), request->height(), data);
                                queue.Publish(envelope.message.properties.reply_to,
                                              envelope.message.properties.correlation_id,
                                              f);
                              });

          std::cout << "Published" << std::endl;
        });
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
  }
}
