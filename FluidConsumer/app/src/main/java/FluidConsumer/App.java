package FluidConsumer;

import javafx.application.Application;
import javafx.beans.binding.Bindings;
import javafx.event.ActionEvent;
import javafx.geometry.HPos;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ProgressBar;
import javafx.scene.control.Slider;
import javafx.scene.image.ImageView;
import javafx.scene.image.WritableImage;
import javafx.scene.layout.GridPane;
import javafx.stage.Stage;

import java.io.IOException;
import java.util.concurrent.TimeoutException;

public class App extends Application  {
    public App() {
    }

    @Override
    public void start(Stage stage) throws Exception {
        var root = new GridPane();
        root.setHgap(8);
        root.setVgap(8);
        root.setPadding(new Insets(5));

        var writableImage = new WritableImage(900, 900);
        var image = new ImageView(writableImage);
        root.add(image, 0, 0, 5, 1);

        var arrow1 = new Arrow(0, 0, 200, 200);
        root.add(arrow1, 0, 0, 5,  1);

        var arrow2 = new Arrow(300, 200, 100, 400);
        root.add(arrow2, 0, 0, 5,  1);

        var slider = new Slider(0, 100, 0);
        slider.setBlockIncrement(1);
        slider.setMajorTickUnit(1);
        slider.setMinorTickCount(0);
        slider.setSnapToTicks(true);
        slider.setPrefWidth(200);
        root.add(slider, 1, 1);

        var numFrameLabel = new Label();
        numFrameLabel.textProperty().bind(Bindings.format("Num frames: %.0f", slider.valueProperty()));
        root.add(numFrameLabel, 0, 1);

        var requestBtn = new Button("Request");
        requestBtn.setOnAction((ActionEvent event) -> {
            call();
        });
        GridPane.setHalignment(requestBtn, HPos.RIGHT);
        root.add(requestBtn, 2, 1);

        var progressLabel = new Label("Loaded: ");
        root.add(progressLabel, 3, 1);

        var progress = new ProgressBar(0);
        progress.setPrefWidth(150);
        root.add(progress, 4, 1);

        Scene scene = new Scene(root, 1000, 1000);
        stage.setScene(scene);
        stage.show();
    }

    public static void main(String[] argv) {
        launch();
    }

    public void call() {
        try (Queue queue = new Queue()) {
            queue.RequestFrames(3);
        } catch (IOException | InterruptedException | TimeoutException e) {
            e.printStackTrace();
        }
    }
}
