package cs5004.animator.view;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.LinkedList;

import javax.swing.JFrame;
import javax.swing.JScrollPane;
import javax.swing.Timer;

import cs5004.animator.model.Model;
import cs5004.animator.shape.Shape;

/**
 * Implementaion of the AnimationView.  This stores out model, keeps track of the current frame, and
 * calls on the Panel to repaint each time a frame changes and we update the shape positions.
 */
public class JFrameView extends JFrame implements AnimationView {
  private Model model;
  private int frame;

  /**
   * JFrame constructor.  Takes in the model and delay in milliseconds between frames. Initializes
   * the frame with a JPanel and JScrollPane.
   *
   * @param model our model object.
   * @param speed speed in milliseconds per frame.
   */
  public JFrameView(Model model, int speed) {
    super();
    this.model = model;
    this.frame = 0;

    this.setTitle("Animayshun");
    this.setSize(10, 10);
    this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

    this.setLayout(new BorderLayout());
    LinkedList<Shape> shapes = model.getShapes();
    AnimationPanel animationPanel = new AnimationPanel(shapes, model.getBounds()[0],
            model.getBounds()[1]);
    animationPanel.setPreferredSize(new Dimension(model.getBounds()[2], model.getBounds()[3]));

    Timer timer = new Timer(speed, taskPerformer);
    JScrollPane scrollPane = new JScrollPane(animationPanel);
    this.add(scrollPane, BorderLayout.CENTER);
    this.pack();
    this.setVisible(true);
    timer.start();
  }

  /**
   * Action listener that is controlled by the timer telling it what to do each tic.
   */
  ActionListener taskPerformer = new ActionListener() {
    public void actionPerformed(ActionEvent evt) {
      model.setToFrame(frame);
      //TODO loop over shapes to render.

      frame++;
      // loops animation TODO: add loop toggle for part 3, stop animation if not on
      if (model.getLastFrame() + 1 == frame) {
        //timer.stop();
        frame = 0;
      }
      refresh();
    }
  };

  public void refresh() {
    this.repaint();
  }
}
