package cs5004.animator.shape;

import java.awt.Color;

import cs5004.animator.model.ShapeTypes;

/**
 * Implements Shape interface with code that will be common to any Shape class.  A shape knows its
 * name, it's Point2D reference point, and its color.  Further parameters are known to individual
 * shape classes.
 */
public abstract class AbstractShape implements Shape {
  protected Point2D reference;
  protected Color color;
  private ShapeTypes type;
  private boolean visible;

  /**
   * Abstract shape constructor which defaults the color to black and accepts the name and reference
   * point.
   *
   * @param reference Point2D point to help locate a shape.
   * @throws IllegalArgumentException up to subclass implementation.
   */
  public AbstractShape(Point2D reference, ShapeTypes type) throws IllegalArgumentException {
    if (reference == null) {
      throw new IllegalArgumentException("Somehow you got a null Point2D object in here");
    }
    this.type = type;
    this.reference = reference;
    this.visible = false;
    color = new Color(0f, 0f, 0f);
  }

  /**
   * Abstract shape constructor which accepts the name, color, and reference point.
   *
   * @param reference Point2D point to help locate a shape.
   * @param color     Color object containing initial color of shape.
   * @throws IllegalArgumentException up to subclass implementation.
   */
  public AbstractShape(Point2D reference, Color color, ShapeTypes type)
          throws IllegalArgumentException {
    if (reference == null) {
      throw new IllegalArgumentException("Somehow you got a null Point2D object in here");
    } else if (color == null) {
      throw new IllegalArgumentException("Color cannot be null");
    }
    this.type = type;
    this.reference = reference;
    this.color = color;
    this.visible = false;
  }

  @Override
  public Point2D getReference() {
    return reference;
  }

  @Override
  public Color getColor() {
    return color;
  }

  @Override
  public ShapeTypes getType() {
    return type;
  }

  @Override
  public boolean isVisible() {
    return this.visible;
  }

  @Override
  public void move(double x, double y) {
    this.reference = new Point2D(x, y);
  }

  @Override
  public void move(Point2D destination) throws IllegalArgumentException {
    if (destination == null) {
      throw new IllegalArgumentException("Null Point2D object");
    }
    this.reference = destination;
  }

  @Override
  public void changeColor(Color color) {
    this.color = color;
  }

  @Override
  public void setVisibility(boolean visible) {
    this.visible = visible;
  }

}
