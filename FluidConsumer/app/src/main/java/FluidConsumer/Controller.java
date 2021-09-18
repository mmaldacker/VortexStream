package FluidConsumer;

import VortexStream.Messages;
import com.google.common.collect.Lists;
import javafx.beans.binding.Bindings;
import javafx.beans.value.ObservableValue;
import javafx.fxml.FXML;
import javafx.scene.control.*;
import javafx.scene.image.ImageView;
import javafx.scene.input.DragEvent;
import javafx.scene.input.MouseDragEvent;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.Pane;
import javafx.scene.shape.Path;
import javafx.scene.text.Text;
import javafx.stage.Stage;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.TimeoutException;

public class Controller {
    private final FluidController fluidController;
    @FXML
    public Text frameText;
    @FXML
    public Slider frameSlider;
    @FXML
    public ProgressBar progress;
    @FXML
    public RadioButton small;
    @FXML
    public RadioButton medium;
    @FXML
    public RadioButton large;
    @FXML
    public ToggleGroup sizeGroup;
    @FXML
    public ImageView imageView;
    @FXML
    public Pane pane;
    private Stage stage;
    private FluidRenderer fluidRenderer;

    public Controller() throws IOException, TimeoutException {
        fluidController = new FluidController();
    }

    public void setStage(Stage stage) {
        this.stage = stage;
    }

    @FXML
    public void initialize() {
        this.fluidRenderer = new FluidRenderer(imageView, 250, 250);

        frameText.textProperty().bind(Bindings.format("Num frames: %.0f", frameSlider.valueProperty()));
        sizeGroup.selectedToggleProperty().addListener((ObservableValue<? extends Toggle> ov, Toggle old_toggle, Toggle new_toggle) -> {
                    if (new_toggle == small) {
                        fluidRenderer.resize(250, 250);
                        stage.sizeToScene();
                    } else if (new_toggle == medium) {
                        fluidRenderer.resize(500, 500);
                        stage.sizeToScene();
                    } else if (new_toggle == large) {
                        fluidRenderer.resize(1000, 1000);
                        stage.sizeToScene();
                    }
                }
        );
    }

    @FXML
    public void onSubmit() {
        try {
            progress.setProgress(0.0);
            fluidRenderer.clear();
            fluidRenderer.stop();
            pane.getChildren().removeIf((node) -> node instanceof Path);

            var startTime = java.lang.System.currentTimeMillis();
            var numFrames = frameSlider.getValue();

            fluidController.RequestFrames(
                    (int) numFrames,
                    (int) imageView.getFitWidth(),
                    (int) imageView.getFitHeight(),
                    sources,
                    (Messages.Frame frame) -> {
                        fluidRenderer.addFrame(frame);
                        progress.setProgress(frame.getFrameId() / frameSlider.getValue());
                        if (frame.getFrameId() == frameSlider.getValue() - 1.0) {
                            var endTime = java.lang.System.currentTimeMillis();
                            double ms_per_frame = (endTime - startTime) / numFrames;
                            System.out.println("Time per frame: " + ms_per_frame);
                            fluidRenderer.start();
                        }
                    });

            sources.clear();
        } catch (IOException | InterruptedException e) {
            e.printStackTrace();
        }
    }

    private Arrow currentArrow;
    private double startX, startY;
    private List<FluidController.Source> sources = Lists.newArrayList();

    @FXML
    public void onDragEntered(MouseEvent event) {
        currentArrow = new Arrow(
                event.getX(),
                event.getY(),
                event.getX(),
                event.getY());
        startX = event.getX();
        startY = event.getY();
        pane.getChildren().add(currentArrow);
    }

    @FXML
    public void onDragOver(MouseEvent event) {
        currentArrow.setEnd(event.getX(), event.getY());
    }

    @FXML
    public void onDragDropped(MouseEvent event) {
        var source  = new FluidController.Source();

        source.x = (float)startX;
        source.y = (float)startY;
        source.dirX = (float)(event.getX() - startX);
        source.dirY = (float)(event.getY() - startY);

        sources.add(source);
    }
}
