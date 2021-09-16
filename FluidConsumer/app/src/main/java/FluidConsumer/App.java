package FluidConsumer;

import javafx.application.Application;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.fxml.FXMLLoader;

public class App extends Application  {
    public App() {
    }

    @Override
    public void start(Stage stage) throws Exception {
        var loader = new FXMLLoader(getClass().getClassLoader().getResource("app.fxml"));
        var controller = new Controller();
        loader.setController(controller);
        Parent root =  loader.load();

        Scene scene = new Scene(root);
        stage.setScene(scene);
        stage.setResizable(false);

        controller.setStage(stage);

        stage.show();
    }

    public static void main(String[] argv) {
        launch();
    }

}
