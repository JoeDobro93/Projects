package cs5004.animator.model;

import java.util.LinkedList;
import java.util.Map;

import cs5004.animator.shape.Shape;

/**
 * Interface outlining the methods an animation model should have.  A model should be able to accept
 * a shape as a parameter, create commands to move, scale, change color, or any other command that
 * may be added in the future.
 */
public interface Model {

  /**
   * Getter for the map of shell shapes, which will be used by the viewer.
   *
   * @return Map of ShapeShell objects with the Shape name as the key.
   */
  Map<String, ShapeShell> getShapesShells();

  LinkedList<Shape> getShapes();

  /**
   * Returns the final frame number in animation to assist in looping or stopping.
   *
   * @return int final frame in animation.
   */
  int getLastFrame();

  /**
   * Getter for the window bounds used to display an animation.  Int array assignment listed below.
   *
   * @return bound array [0] = x, [1] = y, [2] = width, [3] = height
   */
  int[] getBounds();

  /**
   * Takes in information about the shape.  This information is parsed together to create a new
   * Shape object, avoiding potentially having the same shape assigned to two different named keys.
   * Lastly, this parsed shape is sent into a wrapper that keeps
   *
   * @param name      name to assign to the Shape key in our container map.
   * @param type      a string representation of the type of shape.
   * @param x         int x reference coordinate.
   * @param y         int y reference coordinate.
   * @param w         int width of shape.
   * @param h         int height of shape.
   * @param r         int red value in range 0-255
   * @param g         int green value in range 0-255
   * @param b         int blue value in range 0-255
   * @param appear    int time where shape first appears.
   * @param disappear int time where shape disappears.
   */
  void addShape(String name, ShapeTypes type, int x, int y, int w, int h,
                int r, int g, int b, int appear, int disappear);

  /**
   * Generates a formatted command string which is then added to the corresponding ShapeShell's
   * commands list.  If the start position is different from where the shape was prior to this, the
   * shape will be expected to instantly snap to the start position of this command, mirroring
   * keyframe functionality like in Flash.
   *
   * @param name      String name of the shape we are targeting.
   * @param x1        Starting int x coordinate.
   * @param y1        Starting int y coordinate.
   * @param x2        Ending int x coordinate.
   * @param y2        Ending int y coordinate.
   * @param startTime int time action begins
   * @param endTime   int time of last "tic" of action.
   * @throws IllegalArgumentException if parameters coming in are invalid or conflicting.
   */
  void moveShape(String name, int x1, int y1, int x2, int y2, int startTime, int endTime)
          throws IllegalArgumentException;

  /**
   * Generates a formatted command string which is then added to the corresponding ShapeShell's
   * commands list.  If the start size is different from what the shape was prior to this, the shape
   * will be expected to instantly snap to the start sizes of this command, mirroring keyframe
   * functionality like in Flash.
   *
   * @param name      String name of target shape.
   * @param xStart    double containing the start x length.
   * @param yStart    double object containing the start y length.
   * @param x         double object containing the end x length.
   * @param y         double object containing the end y length.
   * @param startTime integer start time for the command.
   * @param endTime   integer end time for the command (inclusive)
   * @throws IllegalArgumentException for invalid or conflicting times or target shape not present
   *                                  or if the scale sizes are less than or equal to 0.
   */
  void scaleShape(String name, double xStart, double yStart, double x, double y,
                  int startTime, int endTime);

  /**
   * Generates a formatted command string which is then added to the corresponding ShapeShell's
   * commands list.  If the start color is different from where the shape was prior to this, the
   * shape will be expected to instantly snap to the start color of this command, mirroring keyframe
   * functionality like in Flash.
   *
   * @param name      String name of target shape.
   * @param r1        int red value in range 0-255.
   * @param g1        int green value in range 0-255.
   * @param b1        int blue value in range 0-255.
   * @param r2        int red value in range 0-255.
   * @param g2        int green value in range 0-255.
   * @param b2        int blue value in range 0-255.
   * @param startTime integer start time for the command.
   * @param endTime   integer end time for the command (inclusive)
   * @throws IllegalArgumentException for invalid or conflicting times or target shape not present
   *                                  or if Color objects are null.
   */
  void changeShapeColor(String name, int r1, int g1, int b1, int r2, int g2, int b2, int startTime,
                        int endTime);

  /**
   * Takes in a given frame and sets every shape to where they should be at that position.
   *
   * @param frame int frame number
   */
  void setToFrame(int frame);

  void removeCommand(String shapeName, MotionType commandType, int startTime)
          throws IllegalArgumentException;

  void removeShape(String shapeName);
}
