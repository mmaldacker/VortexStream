package FluidConsumer;

import VortexStream.Messages;
import javafx.beans.binding.Bindings;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ProgressBar;
import javafx.scene.control.Slider;
import javafx.scene.text.Text;

import java.io.IOException;

public class Controller {
    private FluidController fluidController;
    private FluidRenderer fluidRenderer;

    @FXML
    public Text frameText;

    @FXML
    public Slider frameSlider;

    @FXML
    public Button submit;

    @FXML
    public ProgressBar progress;

    @FXML
    public void initialize() {
        frameText.textProperty().bind(Bindings.format("Num frames: %.0f", frameSlider.valueProperty()));
    }

    @FXML
    public void onSubmit() {
            try {
                fluidController.RequestFrames((int) frameSlider.getValue(), (Messages.Frame frame) -> {
                    fluidRenderer.addFrame(frame);
                });
            } catch (IOException | InterruptedException e) {
                e.printStackTrace();
            }
    }
}
