package cs5004.animator.shape;

import java.awt.Color;

import cs5004.animator.model.ShapeTypes;

/**
 * Rectangle shape that knows its height and width as well as all params known to super.
 */
public class Rectangle extends AbstractShape {
  double width;
  double height;

  /**
   * Two parameter input, follows protocol of super.  Default to a 10x10 square.
   *
   * @param reference Point2D reference point.
   */
  public Rectangle(Point2D reference) {
    super(reference, ShapeTypes.RECTANGLE);
    width = 10;
    height = 10;
  }

  /**
   * Two parameter input, follows protocol of super.  Default to a 10x10 square.
   *
   * @param x initial double x coordinate for reference.
   * @param y initial double y coordinate for reference.
   */
  public Rectangle(double x, double y) {
    super(new Point2D(x, y), ShapeTypes.RECTANGLE);
    width = 10;
    height = 10;
  }

  /**
   * Full constructor that takes all relevant parameters for an Rectangle.
   *
   * @param x      initial double x coordinate for reference.
   * @param y      initial double y coordinate for reference.
   * @param width  double width of current rectangle's initial state.
   * @param height double height of current rectangle's initial state.
   * @param color  initial Color for current Rectangle.
   * @throws IllegalArgumentException if radii are 0 or negative.
   */
  public Rectangle(double x, double y, double width, double height, Color color) {
    super(new Point2D(x, y), color, ShapeTypes.RECTANGLE);
    if (width <= 0 || height <= 0) {
      throw new IllegalArgumentException("Size must be non-zero and positive");
    }
    this.width = width;
    this.height = height;
  }

  @Override
  public double getSizeX() {
    return width;
  }

  @Override
  public double getSizeY() {
    return height;
  }

  @Override
  public void scale(double x, double y) throws IllegalArgumentException {
    if (x <= 0 || y <= 0) {
      throw new IllegalArgumentException("Size must be non-zero and positive");
    }
    this.width = x;
    this.height = y;
  }

  @Override
  public String scaleToString(double x, double y) {
    return String.format("Width: %.2f, Height: %.2f", x, y);
  }

  @Override
  public String toString() {
    return "Center: " + super.reference + "\nHeight: " + width + ", Width: " + height
            + String.format("\nColor: (r=%d,g=%d,b=%d)",
            color.getRed(), color.getGreen(), color.getBlue());
  }

}
