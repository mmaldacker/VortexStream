package FluidConsumer;

import VortexStream.Messages;
import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;

import java.io.IOException;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.TimeoutException;

public class FluidController implements AutoCloseable {
    private final Connection connection;
    private final Channel channel;

    public FluidController() throws IOException, TimeoutException {
        ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("localhost");

        connection = factory.newConnection();
        channel = connection.createChannel();
    }

    public void RequestFrames(int numFrames, int width, int height, List<Source> sources, Callback callback) throws IOException, InterruptedException {
        final String corrId = UUID.randomUUID().toString();

        String replyQueueName = channel.queueDeclare().getQueue();
        AMQP.BasicProperties props = new AMQP.BasicProperties
                .Builder()
                .correlationId(corrId)
                .replyTo(replyQueueName)
                .build();

        System.out.println("Correlation id: " + corrId + ", Queue name: " + replyQueueName);

        var requestBuilder = Messages.Request.newBuilder()
                .setNumFrames(numFrames)
                .setWidth(width)
                .setHeight(height);

        for (var source : sources) {
            requestBuilder.addSources(Messages.Source.newBuilder()
                    .setPosition(Messages.Vec2.newBuilder().setX(source.x).setY(source.y).build())
                    .setDirection(Messages.Vec2.newBuilder().setX(source.dirX).setY(source.dirY).build())
                    .build());
        }

        var request = requestBuilder.build();

        String requestQueueName = "fluid_request";

        System.out.println("Sending num bytes: " + request.toByteArray().length);
        channel.basicPublish("", requestQueueName, props, request.toByteArray());

        channel.basicConsume(replyQueueName, true, (consumerTag, delivery) -> {
            if (delivery.getProperties().getCorrelationId().equals(corrId)) {
                var frame = Messages.Frame.parseFrom(delivery.getBody());
                callback.handle(frame);
            }
        }, consumerTag -> {
            System.out.println("Reply cancelled " + consumerTag);
        });
    }

    public void close() throws IOException {
        connection.close();
    }

    @FunctionalInterface
    public interface Callback {
        void handle(Messages.Frame frame);
    }

    public static class Source {
        public float x, y;
        public float dirX, dirY;
    }
}
