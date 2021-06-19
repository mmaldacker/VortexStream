package FluidConsumer;

import VortexStream.Messages;
import com.google.common.collect.Lists;
import javafx.animation.AnimationTimer;
import javafx.scene.image.ImageView;
import javafx.scene.image.PixelFormat;
import javafx.scene.image.WritableImage;
import javafx.scene.paint.Color;

import java.util.List;

public class Player extends AnimationTimer {
    private WritableImage image;
    public ImageView imageView;

    public Player(int width, int height) {
        frames = Lists.newArrayList();
        image = new WritableImage(width, height);
        imageView = new ImageView(image);
    }

    public void addFrame(Messages.Frame frame) {
        frames.add(frame);
    }

    @Override
    public void handle(long now) {
        if (frame_index < frames.size()) {
            var frame = frames.get(frame_index);

            var writer = image.getPixelWriter();
            for (int j = 0; j < frame.getHeight(); j++) {
                for (int i = 0; i < frame.getWidth(); i++) {
                    float c = frame.getPixels(i + j * frame.getWidth());
                    writer.setColor(i, j, Color.color(c, c, c));
                }
            }

            frame_index++;
        }
    }

    private int frame_index = 0;
    private List<Messages.Frame> frames;
}
