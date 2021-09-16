package FluidConsumer;

import VortexStream.Messages;
import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;

import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.TimeoutException;

public class FluidController implements AutoCloseable {
    private final Connection connection;
    private final Channel channel;

    @FunctionalInterface
    public interface Callback
    {
        void handle(Messages.Frame frame);
    }

    public FluidController() throws IOException, TimeoutException {
        ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("localhost");

        connection = factory.newConnection();
        channel = connection.createChannel();
    }

    public void RequestFrames(int numFrames, Callback callback) throws IOException, InterruptedException {
        final String corrId = UUID.randomUUID().toString();

        String replyQueueName = channel.queueDeclare().getQueue();
        AMQP.BasicProperties props = new AMQP.BasicProperties
                .Builder()
                .correlationId(corrId)
                .replyTo(replyQueueName)
                .build();

        System.out.println("Correlation id: " + corrId + ", Queue name: " + replyQueueName);

        var request = Messages.Request.newBuilder().setNumFrames(numFrames).build();

        String requestQueueName = "fluid_request";
        channel.basicPublish("", requestQueueName, props, request.toByteArray());

        channel.basicConsume(replyQueueName, true, (consumerTag, delivery) -> {
            if (delivery.getProperties().getCorrelationId().equals(corrId)) {
                var frame = Messages.Frame.parseFrom(delivery.getBody());
                System.out.println("Received reply! Id " + frame.getFrameId());
                callback.handle(frame);
            }
        }, consumerTag -> {
            System.out.println("Reply cancelled " +consumerTag);
        });
    }

    public void close() throws IOException {
        connection.close();
    }
}