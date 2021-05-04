package cs5004.animator.view;

import java.io.PrintStream;
import java.util.Map;

import cs5004.animator.model.Model;
import cs5004.animator.model.MotionBlock;
import cs5004.animator.model.ShapeShell;
import cs5004.animator.shape.Shape;

/**
 * Implementation of the TextView.  Reads data from the model and outputs a text representation.
 * Shapes are listed with their data below them and their commands below that.
 */
public class TextViewImpl implements ToFileView {
  /**
   * Constructor for the text view.  Takes in the model and the output destination.
   *
   * @param model   model where we get our data from.
   * @param outFile PrintStream that tells us where to print out our text.
   */
  public TextViewImpl(Model model, PrintStream outFile) {
    //TODO: make this a parameter

    System.setOut(outFile);

    printBounds(model.getBounds());
    System.out.println("---------------------------------");
    for (Map.Entry<String, ShapeShell> entry : model.getShapesShells().entrySet()) {
      model.setToFrame(entry.getValue().getAppearDisappearTimes()[0]);
      printShape(entry);
      System.out.println("\tCommands:");
      for (MotionBlock command : entry.getValue().getCommandList()) {
        printCommand(command.toString());
      }
      System.out.println("---------------------------------");
    }
  }

  /**
   * Prints the predetermined bounds if this were to be animated in a window.
   *
   * @param bounds int array with canvas offset [0][1] and size [2][3]
   */
  private void printBounds(int[] bounds) {
    String boundStr = String.format("Canvas size: %d x %d\n"
                    + "x/y offset: (%d, %d)\n",
            bounds[2], bounds[3], bounds[0], bounds[1]);
    System.out.println(boundStr);
  }

  /**
   * Prints the shape information when first initialized.
   *
   * @param entry map entry with shape name and the ShapeShell containing the data.
   * @throws IllegalArgumentException If the shape type is not recognized.
   */
  private void printShape(Map.Entry<String, ShapeShell> entry) throws IllegalArgumentException {
    String reference;
    String xScale;
    String yScale;
    String type;
    Shape shape = entry.getValue().getShape();
    switch (shape.getType()) {
      case OVAL:
        reference = "Center";
        xScale = "Diameter X";
        yScale = "Diameter Y";
        type = "Ellipse";
        break;
      case RECTANGLE:
        reference = "Top left corner";
        xScale = "Width";
        yScale = "Height";
        type = "Rectangle";
        break;
      default:
        throw new IllegalArgumentException(String.format("Unknown shape type \"%s\"",
                shape.getType().toString()));
    }

    String shapeStr = String.format("Shape Name: %s\n"
                    + "\tType: %s\n"
                    + "\tTime Range: t=%d to t=%d\n"
                    + "\tInitial parameters:\n"
                    + "\t\t%s: (%.2f, %.2f)\n"
                    + "\t\tSize: %s - %.2f, %s - %.2f\n"
                    + "\t\tColor: r=%d, g=%d, b=%d\n",
            entry.getKey(), type,
            entry.getValue().getAppearDisappearTimes()[0],
            entry.getValue().getAppearDisappearTimes()[1],
            reference, shape.getReference().getX(), shape.getReference().getY(),
            xScale, shape.getSizeX(), yScale, shape.getSizeY(),
            shape.getColor().getRed(), shape.getColor().getGreen(), shape.getColor().getBlue());

    System.out.println(shapeStr);
  }

  /**
   * This simply prints out the string to the file.  The reason I am using this as a separate method
   * rather than a simple print because I plan to revisit the way I handled storing commands and
   * reimplement this.  As is now, I am relying on the model's output to print to the viewer.  This
   * doesn't seem to be in the spirit of MVC with overlap between the Model and View.  What I would
   * like to do if time allows is store the commands in a struct-like object with the parameters and
   * do away with the strings.  Unfortunately due to time restraints and that change requiring me to
   * rewrite a lot of code, I am leaving it for now.
   *
   * @param command String based command to process.
   */
  private void printCommand(String command) {
    System.out.println("\t\t" + command);
  }
}
