package cs5004.animator.view;

import java.awt.event.ActionListener;
import java.awt.event.KeyListener;
import java.util.LinkedList;

import cs5004.animator.shape.Shape;

/**
 * Interface for a GraphicView interface that can communicate with a controller.
 */
public interface GraphicView {

  /**
   * Assigns an ActionListener to all parameters that need it.  In this case, we want to handle
   * JComponents that have an actionCommand assigned.
   *
   * @param e the actionlistener (which is our controller).
   */
  void setListener(ActionListener e);

  /**
   * Changes the text that is displayed on what should be the play/pause button.  Used by the
   * controller to change to Resume when paused and Pause when playing.
   *
   * @param message the text we want to display on the button.
   */
  void playPause(String message);

  /**
   * Assigns a KeyListener to the JComponents that may need to react to keyboard input.
   *
   * @param k keyboard listener with actions from key presses.
   */
  void setKeyListener(KeyListener k);

  /**
   * Getter for the speed typed into the Speed parameter box.  Used by the controller to change the
   * speed.
   *
   * @return current speed from the view.
   */
  String getSpeed();

  /**
   * Toggles the check on the Loop check box.  It is up to the controller to determine the actual
   * loop functionality is synchronized with what this box displays.
   */
  void toggleLoop();

  /**
   * Used to repaint the frame, updating the AnimationPanel contents at the current time.
   */
  void repaintPanel();

  /**
   * Allows for the animation to be swapped from a new files which provides us with the shapes and
   * bounds for the new file.
   *
   * @param shapes new list of shapes from new animation model.
   * @param bounds new bounds to fix screen to new model.
   */
  void fileChange(LinkedList<Shape> shapes, int[] bounds);

  /**
   * Updates shapes displayed with new list of shapes.  Intended to be used after shapes are added
   * or removes from the current model, though similar to fileChange.
   *
   * @param shapes updated list of shapes.
   */
  void refreshShapes(LinkedList<Shape> shapes);


}
