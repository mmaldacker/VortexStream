package FluidConsumer;

import javafx.animation.AnimationTimer;
import javafx.scene.image.PixelFormat;
import javafx.scene.image.WritableImage;

import java.util.List;

public class Player extends AnimationTimer {
    private WritableImage image;

    public Player(WritableImage image) {
        this.image = image;
    }

    @Override
    public void handle(long now) {
        if (frame_index < frames.size()) {
            Frame frame = frames.get(frame_index);
            image.getPixelWriter().setPixels(0,
                    0,
                    frame.width,
                    frame.height,
                    PixelFormat.getByteRgbInstance(),
                    frame.buffer,
                    0,
                    frame.width);

            frame_index++;
        }
    }

    private int frame_index = 0;
    private List<Frame> frames;
}
