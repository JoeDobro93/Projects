package cs5004.animator.model;

/**
 * Enum for shape types.  Each type can return a string representation of itself.  Add more if more
 * shapes are added.
 */
public enum ShapeTypes {
  OVAL("oval"),
  RECTANGLE("rectangle");

  private String txt;

  ShapeTypes(String txt) {
    this.txt = txt;
  }

  @Override
  public String toString() {
    return this.txt;
  }
}
