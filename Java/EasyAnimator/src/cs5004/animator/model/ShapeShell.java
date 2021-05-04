package cs5004.animator.model;

import java.awt.Color;
import java.util.LinkedList;
import java.util.List;
import java.util.stream.Collectors;

import cs5004.animator.shape.Shape;

/**
 * A shell class to contain a shape and all relevant commands it will take.  Also tracks the changes
 * against the time parameters to make sure that movements are possible.
 */
public class ShapeShell {
  private Shape shape;
  private LinkedList<MotionBlock> commandList;
  private int appear;
  private int disappear;
  private final InitialParams initialParams;

  /**
   * Constructor that assigns a Shape object to shape and the integer appear and disappear times.
   *
   * @param shape     Shape object to be stored in this wrapper.
   * @param appear    integer time for this shape to appear.
   * @param disappear integer time for this shape to disappear.
   * @throws IllegalArgumentException if shape is null or the appear and disappear times are
   *                                  conflicting.
   */
  protected ShapeShell(Shape shape, int appear, int disappear) throws IllegalArgumentException {
    // Exception handling
    if (shape == null) {
      throw new IllegalArgumentException("Shape cannot be null");
    } else if (appear < 0 || disappear < 0) {
      throw new IllegalArgumentException("Times cannot be negative");
    } else if (appear > disappear) {
      throw new IllegalArgumentException("Cannot disappear before appearing");
    }

    this.shape = shape;
    this.appear = appear;
    this.disappear = disappear;
    this.initialParams = new InitialParams((int) (shape.getReference().getX()),
            (int) (shape.getReference().getY()),
            (int) shape.getSizeX(), (int) shape.getSizeY(),
            shape.getColor());
    commandList = new LinkedList<MotionBlock>();
  }

  /**
   * Getter for shape.
   *
   * @return this object's current Shape.
   */
  public Shape getShape() {
    return this.shape;
  }

  /**
   * Getter for the list of command in current ShapeShell.
   *
   * @return LinkedList of commands stored as strings.
   */
  public LinkedList<MotionBlock> getCommandList() {
    return this.commandList;
  }

  /**
   * Getter for the appear and disappear times.
   *
   * @return an integer array with appear time in position 0 and disappear at position 1.
   */
  public int[] getAppearDisappearTimes() {
    int[] times = new int[]{appear, disappear};
    return times;
  }

  /**
   * Sets current shape back to its original state from first appearance.
   */
  protected void setInitialParams() {
    this.shape.scale(initialParams.w, initialParams.h);
    this.shape.move(initialParams.x, initialParams.y);
    this.shape.changeColor(initialParams.color);
  }

  /**
   * Accepts a command sent in from the model.  This method should not be accessed directly, but
   * should only be sent commands that have been properly formatted from the model.
   *
   * @param command String command to add to commands list.
   * @throws IllegalArgumentException if something is wrong with the command format or if the times
   *                                  conflict with another command.
   */
  protected void addCommand(MotionBlock command) throws IllegalArgumentException {
    if (command == null) {
      throw new IllegalArgumentException("command was null");
    }

    int thisTime = command.getTimes()[0];
    int otherTime;
    timeConflicts(command);
    // if we passed out time conflict check, this must be a valid command, so we will change our
    // shape appear/disappear times in case that changes due to the new command.
    if (command.getTimes()[0] < this.appear) {
      appear = command.getTimes()[0];
      /*switch (command.getType()) {
        case MOVE:
          initialParams.x = command.getStartParams()[0];
          initialParams.y = command.getStartParams()[1];
          break;
        case SCALE:
          initialParams.w = command.getStartParams()[0];
          initialParams.h = command.getStartParams()[1];
          break;
        case RECOLOR:
          initialParams.color = new Color(command.getStartParams()[0], command.getStartParams()[0],
                  command.getStartParams()[0]);
      }*/
    }
    if (command.getTimes()[1] > this.disappear) {
      disappear = command.getTimes()[1];
    }

    // add the command based on start time order.  Scale always must come before motion if
    // simultaneous.
    for (int i = 0; i < commandList.size(); ++i) {
      otherTime = commandList.get(i).getTimes()[0];
      if (otherTime > thisTime) {
        // puts it one before the first command with a greater start time that it.
        commandList.add(i, command);
        return;
      } else if (otherTime == thisTime) {
        // this forces scale to come before move since calculating the center is dependant on the
        // size.
        if (commandList.get(i).getType() == MotionType.MOVE) {
          if (commandList.size() - 1 == i) {
            commandList.addLast(command);
          } else {
            commandList.add(i + 1, command);
          }
        } else {
          commandList.add(i, command);
        }
        break;
      }
    }
    //if we made it through all of the previous commands and the time
    commandList.addLast(command);
  }

  protected void removeCommand(MotionType commandType, int startTime)
          throws IllegalArgumentException {
    MotionBlock command;
    for (int i = 0; i < commandList.size(); ++i) {
      command = commandList.get(i);
      if (command.getType() == commandType) {
        if (startTime == command.getTimes()[0]) {
          commandList.remove(i);
          return;
        } else if (command.getTimes()[0] > startTime) {
          break;
        }
      }
    }
    throw new IllegalArgumentException("No matching command " + commandType.name() + " "
            + startTime);
  }


  /**
   * Helper function that checks internally for time conflicts.  negative time and end time before
   * start time is handled in the MotionBlock class when taking in arguments, so it is not repeated
   * here.
   *
   * @param command MotionBlock to check against existing commands.
   * @throws IllegalArgumentException if conflicting command exists
   */
  private void timeConflicts(MotionBlock command) throws IllegalArgumentException {
    List<MotionBlock> subList = commandList.stream().filter(c -> c.getType() == command.getType())
            .collect(Collectors.toList());

    for (MotionBlock each : subList) {
      // if start of command is less than end of each AND end of command is greater than each
      if ((command.getTimes()[0] < each.getTimes()[1]
              && command.getTimes()[1] > each.getTimes()[0])) {
        throw new IllegalArgumentException(String.format("\"%s\" conflicts with \"%s\"",
                command.toString(), each.toString()));
      }
    }
  }

  @Override
  public String toString() {
    String commandString = String.format("Appears: %d; Disappears: %d\n",
            this.appear, this.disappear);
    for (MotionBlock each : commandList) {
      //TODO remove final \n
      commandString += each.toString() + "\n";
    }

    return shape.toString() + "\n\n" + commandString;
  }

  /**
   * helper to hold initial parameters.
   */
  private class InitialParams {
    int x;
    int y;
    int w;
    int h;
    Color color;

    public InitialParams(int x, int y, int w, int h, Color color) {
      this.x = x;
      this.y = y;
      this.w = w;
      this.h = h;
      this.color = color;
    }

  }
}
