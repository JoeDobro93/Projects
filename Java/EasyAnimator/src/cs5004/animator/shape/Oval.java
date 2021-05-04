package cs5004.animator.shape;

import java.awt.Color;

import cs5004.animator.model.ShapeTypes;

/**
 * Oval shape that knows its x and y diameter plus all other parameters in the super.
 */
public class Oval extends AbstractShape {
  double diamX;
  double diamY;
  double xOffset;
  double yOffset;


  /**
   * Two parameter input, follows protocol of super.  Default to a 10x10 circle.
   *
   * @param reference Point2D reference point.
   */
  public Oval(Point2D reference) {
    super(new Point2D(reference.getX() + 10, reference.getY() + 10), ShapeTypes.OVAL);
    diamX = 10;
    diamY = 10;
  }

  /**
   * Two parameter input, follows protocol of super.  Default to a 10x10 circle.
   *
   * @param x initial double x coordinate for reference.
   * @param y initial double y coordinate for reference.
   */
  public Oval(double x, double y) {
    super(new Point2D(x + 10, y + 10), ShapeTypes.OVAL);
    diamX = 10;
    diamY = 10;
  }

  /**
   * Full constructor that takes all relevant parameters for an Oval.
   *
   * @param x     initial double x coordinate for reference.
   * @param y     initial double y coordinate for reference.
   * @param diamX double x diameter of current oval's initial state.
   * @param diamY double y diameter of current oval's initial state.
   * @param color initial Color for current oval.
   * @throws IllegalArgumentException if radii are 0 or negative.
   */
  public Oval(double x, double y, double diamX, double diamY, Color color)
          throws IllegalArgumentException {
    super(new Point2D(x, y), color, ShapeTypes.OVAL);
    // Wht can't this come before the super?
    if (diamX <= 1 || diamY <= 1) {
      throw new IllegalArgumentException("Diameter must be 2 or higher");
    }

    this.diamX = diamX;
    this.diamY = diamY;
    this.xOffset = diamX;
    this.yOffset = diamY;
  }

  @Override
  public void move(double x, double y) {
    super.move(x, y);
  }

  @Override
  public void move(Point2D destination) throws IllegalArgumentException {
    // assuming input is based on top corner.
    super.move(new Point2D(destination.getX(), destination.getY()));
  }

  @Override
  public double getSizeX() {
    return diamX;
  }

  @Override
  public double getSizeY() {
    return diamY;
  }

  public double getRadiusX() {
    return diamX / 2;
  }

  public double getRadiusY() {
    return diamY / 2;
  }

  public Point2D getCenter() {
    return new Point2D(this.reference.getX() - this.getRadiusX(),
            this.reference.getY() - this.getRadiusY());
  }

  @Override
  public void scale(double x, double y) throws IllegalArgumentException {
    if ((int) x <= 0 || (int) y <= 0) {
      throw new IllegalArgumentException("Diameter must be 1 or more");
    }
    this.diamX = x;
    this.diamY = y;
  }

  @Override
  public String scaleToString(double x, double y) {
    return String.format("X Diameter: %.2f, Y Diameter: %.2f", x, y);
  }

  @Override
  public String toString() {
    return "\tCenter: " + super.reference + "\n\tX diam: " + diamX + ", Y diam: " + diamY
            + String.format("\n\tColor: (r=%d,g=%d,b=%d)",
            color.getRed(), color.getGreen(), color.getBlue());
  }

  @Override
  public void changeColor(Color color) {
    super.changeColor(color);
  }
}
