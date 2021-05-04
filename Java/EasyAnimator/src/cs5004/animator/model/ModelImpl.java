package cs5004.animator.model;

import java.awt.Color;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.stream.Collectors;

import cs5004.animator.shape.Oval;
import cs5004.animator.shape.Rectangle;
import cs5004.animator.shape.Shape;

/**
 * Implementation of the model interface.  Stores shape and commands in a HashMap containing
 * ShapeShell objects, which stores links this data.  It also stores the inital data of the shape to
 * properly render animations at frames where the shape is present, but some types of commands have
 * not yet been used.
 */
public class ModelImpl implements Model {
  private int[] bounds;
  private int lastFrame;
  private LinkedHashMap<String, ShapeShell> shapeMap;

  /**
   * Model constructor, initializes with boundary data as well setting the last frame to 0 and
   * initializing a LinkedHashMap to keep everything in the order it came in.
   *
   * @param bounds bound array [0] = x, [1] = y, [2] = width, [3] = height
   */
  public ModelImpl(int[] bounds) {
    this.bounds = bounds;
    //default to 0;
    lastFrame = 0;
    shapeMap = new LinkedHashMap<String, ShapeShell>();
  }

  @Override
  public Map<String, ShapeShell> getShapesShells() {
    return shapeMap;
  }

  @Override
  public LinkedList<Shape> getShapes() {
    return shapeMap.values().stream().map(s -> s.getShape())
            .collect(Collectors.toCollection(LinkedList::new));
  }

  @Override
  public int getLastFrame() {
    return lastFrame;
  }

  @Override
  public int[] getBounds() {
    return this.bounds;
  }

  @Override
  public void addShape(String name, ShapeTypes type, int x, int y, int w, int h, int r, int g,
                       int b, int appear, int disappear) throws IllegalArgumentException {
    if (shapeMap.containsKey(name)) {
      throw new IllegalArgumentException("Shape name already taken");
    } else if (appear < 0 || disappear < 0) {
      throw new IllegalArgumentException("Shape appearance and/or disappearance times negative");
    } else if (appear > disappear) {
      throw new IllegalArgumentException("Cannot disappear before appearing");
    } else if (w <= 0 || h <= 0) {
      throw new IllegalArgumentException("Width and height must be non-zero and positive");
    } else if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
      throw new IllegalArgumentException("Color values must be in range 0 - 255");
    }
    // sets the last frame of the animation to the last frame of all shapes.
    if (disappear > lastFrame) {
      lastFrame = disappear;
    }
    Shape shape;
    Color color = new Color(r, g, b);

    /* If I have time, I want to explore a more object oriented approach to this.  I want to make a
     * way to have Shape know what shapes exist and send this argument to return a shape of the
     * appropriate type.  As is now, my model needs to know what types of Shapes exist which is not
     * in the spirit of OOP.
     */
    switch (type) {
      case OVAL:
        shape = new Oval(x, y, w, h, color);
        break;
      case RECTANGLE:
        shape = new Rectangle(x, y, w, h, color);
        break;
      default:
        throw new IllegalArgumentException("Unrecognized shape type");
    }
    shapeMap.put(name, new ShapeShell(shape, appear, disappear));
  }

  @Override
  public void moveShape(String name, int x1, int y1, int x2, int y2, int startTime, int endTime)
          throws IllegalArgumentException {
    if (shapeMap.containsKey(name)) {
      shapeMap.get(name).addCommand(new MotionBlock(MotionType.MOVE, new int[]{startTime, endTime},
              new int[]{x1, y1}, new int[]{x2, y2}));
    } else {
      throw new IllegalArgumentException(String.format("Shape with name \"%s\" not present", name));
    }

    if (endTime > lastFrame) {
      lastFrame = endTime;
    }
  }

  @Override
  public void scaleShape(String name, double x1, double y1, double x2, double y2,
                         int startTime, int endTime) throws IllegalArgumentException {
    if (shapeMap.containsKey(name)) {
      shapeMap.get(name).addCommand(new MotionBlock(MotionType.SCALE, new int[]{startTime, endTime},
              new int[]{(int) x1, (int) y1}, new int[]{(int) x2, (int) y2}));
    } else {
      throw new IllegalArgumentException(String.format("Shape with name \"%s\" not present", name));
    }
    checkLastFrame();
  }

  @Override
  public void changeShapeColor(String name, int r1, int g1, int b1, int r2, int g2, int b2,
                               int startTime, int endTime) throws IllegalArgumentException {
    if (shapeMap.containsKey(name)) {
      if (r1 < 0 || r1 > 255 || g1 < 0 || g1 > 255 || b1 < 0 || b1 > 255
              || r2 < 0 || r2 > 255 || g2 < 0 || g2 > 255 || b2 < 0 || b2 > 255) {
        throw new IllegalArgumentException("Color values must be in range 0 - 255");
      }
      shapeMap.get(name).addCommand(new MotionBlock(MotionType.RECOLOR,
              new int[]{startTime, endTime},
              new int[]{r1, g1, b1}, new int[]{r2, g2, b2}));
    } else {
      throw new IllegalArgumentException(String.format("Shape with name \"%s\" not present", name));
    }
    checkLastFrame();
  }

  /**
   * This method is very messy, inefficient, and not how I would handle this if I had time to do it
   * again.  I would much rather store all my ShapeShell's command data in structs and call to
   * methods in there to work directly with the data, rather than going back and forth between
   * strings, wasting processing time.  I may redo all of this again before part 3 is due but don't
   * have enough time right now before this assignment is due.
   *
   * @param frame int frame number
   */
  @Override
  public void setToFrame(int frame) {
    int[] tempTimes; //hold on to times while processing.
    MotionType tempType; // hold on to type name while processing.

    for (ShapeShell each : shapeMap.values()) {
      // commands that affect state at current frame.
      Map<MotionType, MotionBlock> relevantCommands = new HashMap<MotionType, MotionBlock>();
      // case for the frame being out of range, make shape invisible.
      if (each.getAppearDisappearTimes()[0] > frame || each.getAppearDisappearTimes()[1] < frame) {
        each.getShape().setVisibility(false);
      } else {
        // start by setting the shape to the default params.  This allows frames to be called out of
        // order without worrying about later commmands changing earlier states, always starting
        //from 0.
        each.getShape().setVisibility(true);
        each.setInitialParams();
        for (MotionBlock command : each.getCommandList()) {
          tempTimes = command.getTimes();
          tempType = command.getType();
          // add a command to the Map if it happens before or on the goal frame.  If the same type
          // of command occurs more than once before the frame, the key matches so it is replaced.
          if (tempTimes[0] <= frame) {
            relevantCommands.put(tempType, command);
          } else {
            // any command that starts after the frame is irrelevant, so exit after we get there.
            // The commands have been sorted, so we can trust this.  However, since this is relying
            // on the ShapeShell's internal behvoir, when I redo this section, I will implement this
            // type of logic in ShapeShell instead.
            break;
          }
        }
        // Now that we have these commands, we can run them up until the desired frame.
        // Start with this to ensure scale happens first, otherwise motion gets messed up.
        if (relevantCommands.containsKey(MotionType.SCALE)) {
          runCommand(each.getShape(), relevantCommands.get(MotionType.SCALE), frame);
          relevantCommands.remove(MotionType.SCALE);
        }
        for (MotionBlock command: relevantCommands.values()) {
          runCommand(each.getShape(), command, frame);
        }
      }
    }
  }

  @Override
  public void removeCommand(String shapeName, MotionType type, int startTime)
          throws IllegalArgumentException {
    if (this.shapeMap.keySet().contains(shapeName)) {
      shapeMap.get(shapeName).removeCommand(type, startTime);
    } else {
      throw new IllegalArgumentException("No such shape");
    }
    // if final command in current shape was the final action, last frame is moved.
    checkLastFrame();
  }

  @Override
  public void removeShape(String shapeName) {
    shapeMap.remove(shapeName);
    checkLastFrame();
  }

  /**
   * Private helper function to take a command and run it on a desired shape to the desired frame.
   */
  private void runCommand(Shape shape, MotionBlock command, int frame) {
    int[] times = command.getTimes();
    int[] param1 = command.getStartParams();
    int[] param2 = command.getEndParams();
    //System.out.print("\n" + shape.getType().toString()+ " ");
    switch (command.getType()) {
      case MOVE:
        //System.out.println(command);
        shape.move(stateAtFrame(param1[0], param2[0], times, frame),
                stateAtFrame(param1[1], param2[1], times, frame));
        break;
      case SCALE:
        //System.out.println(command);

        shape.scale(stateAtFrame(param1[0], param2[0], times, frame),
                stateAtFrame(param1[1], param2[1], times, frame));
        break;
      case RECOLOR:
        //System.out.println(command);
        Color color = new Color(stateAtFrame(param1[0], param2[0], times, frame),
                stateAtFrame(param1[1], param2[1], times, frame),
                stateAtFrame(param1[2], param2[2], times, frame));
        shape.changeColor(color);
        break;
      default:
        throw new IllegalArgumentException(command.getType().toString()
                + "is not a valid type of command");
    }
  }

  private int stateAtFrame(int start, int end, int[] times, int frame) {
    // if the action has been completed before the frame
    if (frame >= times[1]) {
      return end;
    } else if (frame == times[0]) {
      // avoid doing any calculations if it's just the first frame of the command.
      return start;
    } else {
      double result;

      double comRange = times[1] - times[0]; //
      double changePerFrame = (end - start) / comRange; //amount of change per frame;
      double numFrames = frame - times[0]; // number of frames to apply change to;

      result = (changePerFrame * numFrames) + start;
      return (int) result;
    }
  }

  private void checkLastFrame() {
    lastFrame = 0;
    int checkMe;
    for (Map.Entry<String, ShapeShell> entry: shapeMap.entrySet()) {
      checkMe = entry.getValue().getAppearDisappearTimes()[1];
      if (checkMe > lastFrame) {
        lastFrame = checkMe;
      }
    }
  }
}