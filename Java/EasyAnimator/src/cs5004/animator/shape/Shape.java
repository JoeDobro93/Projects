package cs5004.animator.shape;

import java.awt.Color;

import cs5004.animator.model.ShapeTypes;

/**
 * Interface to outline all methods that are relevant to all Shape classes.  There will be handled
 * either by the AbstractShape or by individual Shape classes depending on specificity per shape.
 */
public interface Shape {

  /**
   * Getter for current Shape's reference.  Implemented in the AbstractShape.
   *
   * @return reference Point2D point.
   */
  Point2D getReference();

  /**
   * Returns the size of a shape on the x axis.  Implemented in individual Shape classes.
   *
   * @return double value height of current shape.
   */
  double getSizeX();

  /**
   * Returns the size of a shape on the y axis.  Implemented in individual Shape classes.
   *
   * @return double value width of current shape.
   */
  double getSizeY();

  Color getColor();

  ShapeTypes getType();

  /**
   * Return true if visible, false if not.
   *
   * @return true if visible, false if not.
   */
  boolean isVisible();

  void move(double x, double y);

  void move(Point2D destination);

  void scale(double x, double y);

  void changeColor(Color color);

  void setVisibility(boolean visible);

  String scaleToString(double x, double y);
}
