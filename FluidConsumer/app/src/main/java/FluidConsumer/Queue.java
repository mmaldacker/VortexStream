package FluidConsumer;

import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;

import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.TimeoutException;

public class Queue implements AutoCloseable {
    private final Connection connection;
    private final Channel channel;

    public Queue() throws IOException, TimeoutException {
        ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("localhost");

        connection = factory.newConnection();
        channel = connection.createChannel();
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

        String requestQueueName = "fluid_request";
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
