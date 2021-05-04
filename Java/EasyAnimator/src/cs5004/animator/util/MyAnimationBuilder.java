package cs5004.animator.util;

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.Map;

import cs5004.animator.model.Model;
import cs5004.animator.model.ModelImpl;
import cs5004.animator.model.ShapeTypes;

/**
 * Implementation of Animation builder which we use to decode the input file data and insert it into
 * the model.
 *
 * @param <Doc> Generic placeholder for the model class.
 */
public class MyAnimationBuilder<Doc> implements AnimationBuilder<Doc> {
  private Map<String, ShapeTypes> shapes = new LinkedHashMap<String, ShapeTypes>();
  private Map<String, LinkedList<Transform>> motion = new HashMap<String, LinkedList<Transform>>();
  private Map<String, Integer> minTime = new HashMap<String, Integer>();
  private Map<String, Integer> maxTime = new HashMap<String, Integer>();
  private int[] bounds;

  @Override
  public Doc build() {
    Model model = new ModelImpl(bounds);
    for (Map.Entry<String, ShapeTypes> entry : shapes.entrySet()) {
      // Initialize the shape based on the first values in the first "transform" entry.
      Transform start = motion.get(entry.getKey()).get(0);

      model.addShape(entry.getKey(), entry.getValue(), start.x1, start.y1, start.w1, start.h1,
              start.r1, start.g1, start.b1, minTime.get(entry.getKey()),
              maxTime.get(entry.getKey()));

      for (Transform command : motion.get(entry.getKey())) {
        if (command.x1 != command.x2 || command.y1 != command.y2) {
          //System.out.println(command.moveString());
          model.moveShape(entry.getKey(), command.x1, command.y1, command.x2, command.y2,
                  command.t1, command.t2);
        }
        if (command.w1 != command.w2 || command.h1 != command.h2) {
          //System.out.println(command.scaleString());
          model.scaleShape(entry.getKey(), command.w1, command.h1, command.w2, command.h2,
                  command.t1, command.t2);
        }
        if (command.r1 != command.r2 || command.g1 != command.g2 || command.b1 != command.b2) {
          //System.out.println(command.colorString());
          model.changeShapeColor(entry.getKey(), command.r1, command.g1, command.b1,
                  command.r2, command.g2, command.b2, command.t1, command.t2);
        }
      }
    }

    return (Doc) model;
  }

  @Override
  public AnimationBuilder<Doc> setBounds(int x, int y, int width, int height) {
    this.bounds = new int[]{x, y, width, height};
    return this;
  }

  @Override
  public AnimationBuilder<Doc> declareShape(String name, String type)
          throws IllegalArgumentException {
    if (shapes.containsKey(name)) {
      throw new IllegalArgumentException(String.format("Name \"%s\" already taken."));
    }
    switch (type.toLowerCase()) {
      case "ellipse":
        shapes.put(name, ShapeTypes.OVAL);
        break;
      case "rectangle":
        shapes.put(name, ShapeTypes.RECTANGLE);
        break;
      default:
        throw new IllegalArgumentException(String.format("\"%s\" is not a valid shape type", type));
    }
    return this;
  }

  @Override
  public AnimationBuilder<Doc> addMotion(String name, int t1, int x1, int y1, int w1, int h1,
                                         int r1, int g1, int b1, int t2, int x2, int y2,
                                         int w2, int h2, int r2, int g2, int b2) {

    if (motion.containsKey(name)) {
      motion.get(name).add(
              new Transform(t1, t2, x1, y1, x2, y2, w1, h1, w2, h2, r1, g1, b1, r2, g2, b2));
    } else {
      LinkedList<Transform> temp = new LinkedList<Transform>();
      temp.add(new Transform(t1, t2, x1, y1, x2, y2, w1, h1, w2, h2, r1, g1, b1, r2, g2, b2));
      motion.put(name, temp);
    }

    // checks if the current motion start/stop time extend the max/min times of shape.
    if (minTime.containsKey(name)) {
      if (minTime.get(name) > t1) {
        minTime.put(name, t1);
      }
      if (maxTime.get(name) < t2) {
        maxTime.put(name, t2);
      }
    } else {
      minTime.put(name, t1);
      maxTime.put(name, t2);
    }

    return this;
  }

  private class Transform {
    int t1;
    int t2;
    int x1;
    int y1;
    int x2;
    int y2;
    int w1;
    int h1;
    int w2;
    int h2;
    int r1;
    int g1;
    int b1;
    int r2;
    int g2;
    int b2;

    public Transform(int t1, int t2, int x1, int y1, int x2, int y2, int w1, int h1, int w2, int h2,
                     int r1, int g1, int b1, int r2, int g2, int b2) {

      this.t1 = t1;
      this.t2 = t2;

      this.x1 = x1;
      this.y1 = y1;
      this.x2 = x2;
      this.y2 = y2;

      this.w1 = w1;
      this.h1 = h1;
      this.w2 = w2;
      this.h2 = h2;

      this.r1 = r1;
      this.g1 = g1;
      this.b1 = b1;
      this.r2 = r2;
      this.g2 = g2;
      this.b2 = b2;
    }

    public String moveString() {
      return String.format("Move from (%d,%d) to (%d,%d) start t=%d end t=%d",
              x1, y1, x2, y2, t1, t2);
    }

    public String colorString() {
      return String.format("Change color from (r=%d,g=%d,b=%d) to (r=%d,g=%d,b=%d) start t=%d end"
              + " t=%d", r1, g1, b1, r2, g2, b2, t1, t2);
    }

    public String scaleString() {
      return String.format("Scale from w=%d h=%d to w=%d h=%d  start t=%d end t=%d",
              w1, h1, w2, h2, t1, t2);
    }

  }
}

