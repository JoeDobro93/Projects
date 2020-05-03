package cs5004.animator.shape;

import java.util.Objects;

/**
 * This class represents a 2D point. This point is denoted in Cartesian coordinates as (x,y).
 */
public class Point2D {
  double x;
  double y;

  /**
   * Construct a 2d point with the given coordinates.
   *
   * @param x the x-coordinate of this point.
   * @param y the y-coordinate of this point.
   */
  public Point2D(double x, double y) {
    this.x = x;
    this.y = y;
  }

  /**
   * Allows us to change this point without the need for creating a new Point2D object each time.
   *
   * @param x int x coordinate
   * @param y int y coordinate
   */
  public void changePoint(double x, double y) {
    this.x = x;
    this.y = y;
  }

  /**
   * Getter for the x-coordinate of this point.
   *
   * @return x-coordinate of this point.
   */
  public double getX() {
    return x;
  }

  /**
   * Getter for the the y-coordinate of this point.
   *
   * @return y-coordinate of this point.
   */
  public double getY() {
    return y;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof Point2D)) {
      return false;
    } else {
      return ((Point2D) obj).getX() == this.x && ((Point2D) obj).getY() == this.y;
    }
  }

  @Override
  public String toString() {
    return "(" + x + "," + y + ")";
  }

  @Override
  public int hashCode() {
    return Objects.hash(x, y);
  }
}
