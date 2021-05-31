package FluidConsumer;

import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeoutException;

public class App implements AutoCloseable {
    private Connection connection;
    private Channel channel;
    private String requestQueueName = "fluid_request";

    public App() throws IOException, TimeoutException {
        ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("localhost");

        connection = factory.newConnection();
        channel = connection.createChannel();
    }

    public static void main(String[] argv) {
        try (App app = new App()) {
            app.call();
        } catch (IOException | InterruptedException | TimeoutException e) {
            e.printStackTrace();
        }
    }

    public void call() throws IOException, InterruptedException {
        final String corrId = UUID.randomUUID().toString();

        String replyQueueName = channel.queueDeclare().getQueue();
        AMQP.BasicProperties props = new AMQP.BasicProperties
                .Builder()
                .correlationId(corrId)
                .replyTo(replyQueueName)
                .build();

        System.out.println("Correlation id: " + corrId + ", Queue name: " + replyQueueName);

        channel.basicPublish("", requestQueueName, props, null);

        String ctag = channel.basicConsume(replyQueueName, true, (consumerTag, delivery) -> {
            if (delivery.getProperties().getCorrelationId().equals(corrId)) {
                System.out.println("Received reply!");
            }
        }, consumerTag -> {
        });

        channel.basicCancel(ctag);
    }

    public void close() throws IOException {
        connection.close();
    }
}
