package cs5004.animator.view;

import java.io.PrintStream;
import java.util.Map;

import cs5004.animator.model.Model;
import cs5004.animator.model.MotionBlock;
import cs5004.animator.model.ShapeShell;
import cs5004.animator.model.ShapeTypes;
import cs5004.animator.shape.Shape;

/**
 * Implementation of SVG View. This class generates an svg file of the animation based on the
 * model.
 */
public class SVGViewImpl implements ToFileView {
  private Model model;
  private int timeMultiplier;

  /**
   * Constructor for the SVG view.  Reads all the ShapeShells from the model as well as the bounds
   * information and parses it into svg formatted text.
   *
   * @param model          model with all the animation data.
   * @param outFile        where all parsed data is printed out.
   * @param timeMultiplier multiplier for number of milliseconds between each action.
   */
  public SVGViewImpl(Model model, PrintStream outFile, int timeMultiplier) {
    this.timeMultiplier = timeMultiplier; //number of milliseconds per "frame"

    System.setOut(outFile);
    this.model = model;
    printBounds(model.getBounds());

    for (Map.Entry<String, ShapeShell> entry : model.getShapesShells().entrySet()) {
      // commands are printed within the shape printer to "close" the shape
      printShape(entry);
    }

    System.out.println("</svg>");
  }

  /**
   * Sets the svg window with parameters from the model. Boundaries to be used: bounds[0] = x offset
   * bounds[1] = y offset bounds[2] = width bounds[3] = height
   *
   * @param bounds int array of boundary parameters
   */
  private void printBounds(int[] bounds) {
    String boundStr;
    boundStr = String.format("<svg width=\"%d\" height=\"%d\" version=\"1.1\" "
                    + "viewBox=\"%d %d %d %d\"\n"
                    + "\txmlns=\"http://www.w3.org/2000/svg\">",
            bounds[2], bounds[3], bounds[0], bounds[1], bounds[2], bounds[3]);
    System.out.println(boundStr + "\n");
  }

  private void printShape(Map.Entry<String, ShapeShell> entry) throws IllegalArgumentException {
    String shapeStr;
    String visibility = "";

    String reference;
    String xScale;
    String yScale;
    String type;
    int xSize;
    int ySize;
    int xPos;
    int yPos;
    Shape shape = entry.getValue().getShape();

    // always set to first appearance before initializing a shape.
    model.setToFrame(entry.getValue().getAppearDisappearTimes()[0]);

    switch (shape.getType()) {
      case OVAL:
        type = "ellipse";
        reference = "c";
        xScale = "rx";
        yScale = "ry";
        xSize = (int) shape.getSizeX() / 2;
        ySize = (int) shape.getSizeY() / 2;
        xPos = (int) shape.getReference().getX() + xSize;
        yPos = (int) shape.getReference().getY() + ySize;
        break;
      case RECTANGLE:
        type = "rect";
        reference = "";
        xScale = "width";
        yScale = "height";
        xSize = (int) shape.getSizeX();
        ySize = (int) shape.getSizeY();
        xPos = (int) shape.getReference().getX();
        yPos = (int) shape.getReference().getY();
        break;
      default:
        throw new IllegalArgumentException(String.format("Unknown shape type \"%s\"",
                shape.getType().toString()));
    }

    shapeStr = String.format("<%s id=\"%s\" %sx=\"%d\" %sy=\"%d\" %s=\"%d\" %s=\"%d\" "
                    + "fill=\"rgb(%d,%d,%d)\" visibility=\"hidden\" >",
            type, entry.getKey(),
            reference, xPos, reference, yPos, xScale, xSize, yScale, ySize,
            shape.getColor().getRed(), shape.getColor().getGreen(), shape.getColor().getBlue());

    visibility = String.format("\t<set attributeName=\"visibility\" to=\"visible\" "
                    + "begin=\"%dms\" dur=\"%dms\" fill=\"freeze\" />",
            entry.getValue().getAppearDisappearTimes()[0] * timeMultiplier,
            entry.getValue().getAppearDisappearTimes()[1] * timeMultiplier);

    System.out.println(shapeStr);
    System.out.println(visibility);

    for (MotionBlock command : entry.getValue().getCommandList()) {
      printCommand(command, reference, xScale, yScale, shape);
    }

    System.out.println(String.format("</%s>", type));
  }

  private void printCommand(MotionBlock command, String reference, String xScale, String yScale,
                            Shape shape) throws IllegalArgumentException {
    String stringStart = "\t<animate";
    int xOffset = 0;
    int yOffset = 0;

    // borrowed my ways of getting numbers from the command from my ModelImpl.  As mentioned
    // throughout, this will be unnecessary once I change commands from strings to struct-like
    // objects and would help avoid duplicate code.
    int[] startEnd = command.getTimes();
    stringStart += " " + String.format("attributeType=\"xml\" begin=\"%.1fms\" dur=\"%.1fms\"",
            (double) startEnd[0] * timeMultiplier,
            (double) (startEnd[1] - startEnd[0]) * timeMultiplier);

    switch (command.getType()) {
      case SCALE:
        model.setToFrame(startEnd[0]);
        int w1 = (int) shape.getSizeX();
        int h1 = (int) shape.getSizeY();
        model.setToFrame(startEnd[1]);
        int w2 = (int) shape.getSizeX();
        int h2 = (int) shape.getSizeY();

        if (shape.getType() == ShapeTypes.OVAL) {
          w1 = w1 / 2;
          h1 = h1 / 2;
          w2 = w2 / 2;
          h2 = h2 / 2;
        }

        System.out.println(stringStart + String.format(
                " attributeName=\"%s\" from=\"%d\" to=\"%d\" fill=\"freeze\" />",
                xScale, w1, w2));

        System.out.println(stringStart + String.format(
                " attributeName=\"%s\" from=\"%d\" to=\"%d\" fill=\"freeze\" />",
                yScale, h1, h2));
        break;
      case MOVE:
        model.setToFrame(startEnd[0]);
        if (shape.getType() == ShapeTypes.OVAL) {
          xOffset = (int) (shape.getSizeX() / 2);
          yOffset = (int) (shape.getSizeY() / 2);
        }
        int x1 = (int) shape.getReference().getX() + xOffset;
        int y1 = (int) shape.getReference().getY() + yOffset;
        model.setToFrame(startEnd[1]);
        if (shape.getType() == ShapeTypes.OVAL) {
          xOffset = (int) (shape.getSizeX() / 2);
          yOffset = (int) (shape.getSizeY() / 2);
        }
        int x2 = (int) shape.getReference().getX() + xOffset;
        int y2 = (int) shape.getReference().getY() + yOffset;

        System.out.println(stringStart + String.format(
                " attributeName=\"%sx\" from=\"%d\" to=\"%d\" fill=\"freeze\" />",
                reference, x1, x2));

        System.out.println(stringStart + String.format(
                " attributeName=\"%sy\" from=\"%d\" to=\"%d\" fill=\"freeze\" />",
                reference, y1, y2));
        break;
      case RECOLOR:
        model.setToFrame(startEnd[0]);
        int r1 = shape.getColor().getRed();
        int g1 = shape.getColor().getGreen();
        int b1 = shape.getColor().getBlue();
        model.setToFrame(startEnd[1]);
        int r2 = shape.getColor().getRed();
        int g2 = shape.getColor().getGreen();
        int b2 = shape.getColor().getBlue();

        System.out.println(stringStart + String.format(
                " attributeName=\"fill\" from=\"rgb(%s,%s,%s)\" to=\"rgb(%s,%s,%s)\" "
                        + "fill=\"freeze\" />",
                r1, g1, b1, r2, g2, b2));
        break;
      default:
        throw new IllegalArgumentException(String.format("\"%s\" invalid command.",
                command.getType().toString()));
    }
  }
}