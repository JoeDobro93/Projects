package cs5004.animator.view;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Graphics;
import java.util.LinkedList;
import java.util.List;

import javax.swing.JPanel;

import cs5004.animator.shape.Shape;

/**
 * The panel where all the action happens.  This is where we draw our shapes and is put into our
 * frame.
 */
public class AnimationPanel extends JPanel {
  private List<Shape> shapes;
  private int xOffset;
  private int yOffset;

  /**
   * Constructor which adds the shapes and offset parameters to the panel which will be used to
   * print our frames to the Frame.
   *
   * @param shapes  list of all the shapes to loop through each frame.
   * @param xOffset x offset to subtract from position.
   * @param yOffset y offset to subtract from position.
   * @throws IllegalArgumentException if the shape list is null
   */
  public AnimationPanel(List<Shape> shapes, int xOffset, int yOffset)
          throws IllegalArgumentException {
    super();
    if (shapes == null) {
      throw new IllegalArgumentException("Shape list null");
    }
    this.xOffset = xOffset;
    this.yOffset = yOffset;
    this.shapes = shapes;
    //shapes.add(new Oval(100,100, 10, 20, new Color(255,0,0)));
    //shapes.add(new Rectangle(30,100, 10, 20, new Color(0,0,255)));
    this.setBackground(Color.WHITE);
  }

  protected void setShapes(LinkedList<Shape> shapes) {
    this.shapes = shapes;
  }

  protected void setOffset(int xOffset, int yOffset) {
    this.xOffset = xOffset;
    this.yOffset = yOffset;
  }

  @Override
  protected void paintComponent(Graphics g) {
    //never forget to call super.paintComponent!
    super.paintComponent(g);

    Graphics2D g2d = (Graphics2D) g;

    for (Shape s : shapes) {
      g2d.setColor(s.getColor());
      if (s.isVisible()) {
        switch (s.getType()) {
          case OVAL:
            g2d.fillOval((int) (s.getReference().getX() - xOffset),
                    (int) (s.getReference().getY() - yOffset),
                    (int) (s.getSizeX()), (int) (s.getSizeY()));
            break;
          case RECTANGLE:
            g2d.fillRect((int) s.getReference().getX() - xOffset, (int)
                    s.getReference().getY() - yOffset, (int) s.getSizeX(), (int) s.getSizeY());
            break;
          default:
            throw new IllegalArgumentException("Incompatible Shape: " + s.getType());
        }
      }
    }
  }
}
