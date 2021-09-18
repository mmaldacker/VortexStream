package FluidConsumer;

import VortexStream.Messages;
import com.google.common.collect.Lists;
import javafx.animation.AnimationTimer;
import javafx.scene.image.ImageView;
import javafx.scene.image.PixelFormat;
import javafx.scene.image.WritableImage;
import java.util.List;

public class FluidRenderer extends AnimationTimer {
    private WritableImage image;
    public ImageView imageView;

    public FluidRenderer(ImageView imageView, int width, int height) {
        frames = Lists.newArrayList();
        image = new WritableImage(width, height);
        this.imageView = imageView;
        this.imageView.setImage(image);
    }

    public void resize(int width, int height) {
        image = new WritableImage(width, height);
        imageView.setFitWidth(width);
        imageView.setFitHeight(height);
        imageView.setImage(image);
    }

    public void clear() {
        frame_index = 0;
        frames.clear();
    }

    public void addFrame(Messages.Frame frame) {
        frames.add(frame);
    }

    @Override
    public void handle(long now) {
        if (frame_index < frames.size()) {
            var frame = frames.get(frame_index);

            int[] pixels = new int[frame.getWidth() * frame.getHeight()];

            for (int j = 0; j < frame.getHeight(); j++) {
                for (int i = 0; i < frame.getWidth(); i++) {
                    int index = i + j * frame.getWidth();
                    if (index < frame.getPixelsCount()) {
                        float c = frame.getPixels(index);
                        int value = 255 - (int)(c * 255.0);
                        pixels[index] = ((255 - value) << 24) | (value << 16) | (value << 8) | value;
                    }
                }
            }

            var pixelFormat = PixelFormat.getIntArgbInstance();
            var writer = image.getPixelWriter();
            writer.setPixels(
                    0,
                    0,
                    frame.getWidth(),
                    frame.getHeight(),
                    pixelFormat,
                    pixels,
                    0,
                    frame.getWidth());

            frame_index++;
        }
    }

    private int frame_index = 0;
    private List<Messages.Frame> frames;
}
