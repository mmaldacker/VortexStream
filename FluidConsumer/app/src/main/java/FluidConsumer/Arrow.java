package FluidConsumer;

import javafx.scene.paint.Color;
import javafx.scene.shape.LineTo;
import javafx.scene.shape.MoveTo;
import javafx.scene.shape.Path;

public class Arrow extends Path{
    private static final double defaultArrowHeadSize = 5.0;
    private double startX, startY, arrowHeadSize;
    private MoveTo start;
    private LineTo end;
    private LineTo headLeft;
    private LineTo headRight;
    private LineTo head;

    public Arrow(double startX, double startY, double endX, double endY, double arrowHeadSize){
        super();

        this.startX = startX;
        this.startY = startY;
        this.arrowHeadSize = arrowHeadSize;

        strokeProperty().bind(fillProperty());
        setFill(Color.BLACK);

        start = new MoveTo(startX, startY);
        end = new LineTo(endX, endY);

        getElements().add(start);
        getElements().add(end);

        headLeft = new LineTo(endX, endY);
        headRight = new LineTo(endX, endY);
        head = new LineTo(endX, endY);

        getElements().add(headLeft);
        getElements().add(headRight);
        getElements().add(head);

        setStart(startX, startY);
        setEnd(endX, endY);
    }

    public Arrow(double startX, double startY, double endX, double endY){
        this(startX, startY, endX, endY, defaultArrowHeadSize);
    }

    public void setStart(double startX, double startY) {
        this.startX = startX;
        this.startY = startY;

        start.setX(startX);
        start.setY(startY);
    }

    public void setEnd(double endX, double endY) {
        //ArrowHead
        double angle = Math.atan2((endY - startY), (endX - startX)) - Math.PI / 2.0;
        double sin = Math.sin(angle);
        double cos = Math.cos(angle);
        //point1
        double x1 = (- 1.0 / 2.0 * cos + Math.sqrt(3) / 2 * sin) * arrowHeadSize + endX;
        double y1 = (- 1.0 / 2.0 * sin - Math.sqrt(3) / 2 * cos) * arrowHeadSize + endY;
        //point2
        double x2 = (1.0 / 2.0 * cos + Math.sqrt(3) / 2 * sin) * arrowHeadSize + endX;
        double y2 = (1.0 / 2.0 * sin - Math.sqrt(3) / 2 * cos) * arrowHeadSize + endY;

        end.setX(endX); end.setY(endY);
        headLeft.setX(x1); headLeft.setY(y1);
        headRight.setX(x2); headRight.setY(y2);
        head.setX(endX); head.setY(endY);
    }
}