package cs5004.animator.controller;

import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.io.File;
import java.io.FileReader;
import java.io.PrintStream;
import java.util.HashMap;
import java.util.Map;

import javax.swing.Timer;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JCheckBox;
import javax.swing.JTextField;
import javax.swing.JButton;
import javax.swing.JPanel;

import cs5004.animator.model.Model;
import cs5004.animator.model.ModelImpl;
import cs5004.animator.util.AnimationReader;
import cs5004.animator.util.MyAnimationBuilder;
import cs5004.animator.view.GraphicView;
import cs5004.animator.view.SVGViewImpl;
import cs5004.animator.view.TextViewImpl;

/**
 * Controller that takes handles action commands from buttons in the view and makes changes to the
 * model.
 */
public class Controller implements ActionListener {
  private Model model;
  private GraphicView view;
  private int frame;
  private Timer timer;
  private boolean loop;
  private AnimationEditor animationEditor;

  /**
   * Constructor for the controller.  Takes in an initialized model and GraphicView as parameters as
   * well as the initial playback speed.  Also initializes a new AnimationEditor, which I would
   * consider to be an extension of the controller. Configures the listeners as well and defaults
   * loop to off, timer not running and first frame to frame 0 (not to be confused with JFrame).
   *
   * @param model model for the animation
   * @param view  GraphicView viewtype to represent the model.
   */
  public Controller(Model model, GraphicView view, int speed) {
    this.model = model;
    this.view = view;
    this.animationEditor = new AnimationEditor(model, view);
    view.setListener(this);
    configureKeyBoardListener();
    // make sure we are at frame one when initializing
    model.setToFrame(0);
    loop = false;
    timer = new Timer(speed, this);
    timer.setActionCommand("Timer");

    frame = 0;
  }

  /**
   * Sets up the keyListener like in the example mvc models.  There's room to add more functionality
   * but right now is only handles the L key press toggling loop.
   */
  private void configureKeyBoardListener() {
    Map<Character, Runnable> keyTypes = new HashMap<Character, Runnable>();
    Map<Integer, Runnable> keyPresses = new HashMap<Integer, Runnable>();
    Map<Integer, Runnable> keyReleases = new HashMap<Integer, Runnable>();

    keyPresses.put(KeyEvent.VK_L, () -> {
      view.toggleLoop();
      loop = !loop;
    });

    KeyboardListener k = new KeyboardListener();
    k.setKeyTypedMap(keyTypes);
    k.setKeyPressedMap(keyPresses);
    k.setKeyReleasedMap(keyReleases);

    view.setKeyListener(k);

  }

  /**
   * Handles all actions from user interaction in the GraphicView.  If a function cannot be
   * performed, a JOptionPanel pops up letting the user know why without crashing the program.
   * Contains the following functions:
   * Timer: when a timer tic occurs, advances the frame by one. If loop is on, when we reach the
   *       final frame, it resets to frame 0.  Otherwise, stops timer.
   * Playback: Starts and stops the timer at current frame.  Changes label on playback button.
   * Loop: toggles the loop function.
   * Speed: Updates timer with speed user typed in
   * Load: Pops up JOptionPanel and prompts a file name.  Attempts to open and repopulates the view
   *      with this data.  Makes a new AnimationEditor for this animation.
   * Edit: Opens AnimationEditor in a
   *      separate window.  See AnimationEditor class for specific details.
   * Write: Opens up a JOption panel that allows the user to type in a file name and check which
   *      type of file they would like to write and saves current model in this file.
   *
   * @param e action event that contains the command name.
   */
  @Override
  public void actionPerformed(ActionEvent e) {
    switch (e.getActionCommand()) {
      case "Timer":
        model.setToFrame(frame);
        frame++;
        if (frame > model.getLastFrame()) {
          frame = 0;
          if (!loop) {
            timer.stop();
            view.playPause("Replay");
          }
        }
        view.repaintPanel();
        break;
      case "Playback":
        if (timer.isRunning()) {
          view.playPause("Resume");
          timer.stop();
        } else {
          view.playPause("Pause");
          timer.start();
        }
        break;
      case "Loop":
        loop = !loop;
        break;
      case "Speed":
        try {
          timer.setDelay(Integer.parseInt(view.getSpeed()));
        } catch (Exception ex) {
          JFrame errorBox = new JFrame();
          JOptionPane.showMessageDialog(errorBox, "Speed must be positive number",
                  e.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
        }
        break;
      case "Load":
        loadFile();
        break;
      case "Edit":
        animationEditor.setVisible(true);
        break;
      case "Write":
        writeFile();
        break;
      default:
    }
  }

  /**
   * Pops up JOptionPanel and prompts a file name.  Attempts to open and repopulates the view with
   * this data.  Makes a new AnimationEditor for this animation.
   */
  private void loadFile() {
    JFrame frame = new JFrame();
    String fileName = JOptionPane.showInputDialog(frame, "Enter a file:");
    try {
      this.frame = 0;
      loop = false;
      timer.stop();

      //TODO: remove resources before converting to jar
      Readable inFile = new FileReader(fileName);
      model = AnimationReader.parseFile(inFile, new MyAnimationBuilder<ModelImpl>());
      model.setToFrame(0);

      view.fileChange(model.getShapes(), model.getBounds());
      view.playPause("Play");

      //TODO: if time allows, update editor rather than replace
      animationEditor.dispose();
      animationEditor = new AnimationEditor(model, view);


    } catch (NullPointerException npe) {
      // clicked cancel do nothing
    } catch (Exception e) {
      JOptionPane.showMessageDialog(null, ("Error opening files:\n" + e.getMessage()),
              e.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
    }
  }

  /**
   * Opens up a JOption panel that allows the user to type in a file name and check which type of
   * file they would like to write and saves current model in this file.
   */
  private void writeFile() {
    JFrame fileWriter = new JFrame();
    fileWriter.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
    fileWriter.setTitle("Write File");
    fileWriter.setSize(600, 300);

    JCheckBox textFileCheck = new JCheckBox("Text File");
    JCheckBox svgFileCheck = new JCheckBox("SVG File");
    JTextField fileName = new JTextField();
    fileName.setColumns(20);
    GridBagConstraints c = new GridBagConstraints();
    JPanel writePanel = new JPanel(new GridBagLayout());

    JButton writeButton = new JButton("Write");
    writeButton.addActionListener(e -> {
      try {
        String name = fileName.getText();
        if (!name.equals("")) {
          PrintStream outFile;
          // prevent glitches from the model playing while writing.  not a perfect
          // solution.
          timer.stop();
          if (textFileCheck.isSelected()) {
            try {
              outFile = new PrintStream(new File(name + ".txt"));
              new TextViewImpl(model, outFile);
            } catch (Exception ex) {
              throw new IllegalArgumentException("Could not write file");
            }
          }

          if (svgFileCheck.isSelected()) {
            try {
              outFile = new PrintStream(new File(name + ".svg"));
              new SVGViewImpl(model, outFile, timer.getDelay());
            } catch (Exception ex) {
              throw new IllegalArgumentException("Could not write file");
            }
          }
          fileWriter.dispose();
        } else {
          throw new IllegalArgumentException("Please enter a file name");
        }
      } catch (Exception ex) {
        JOptionPane.showMessageDialog(null, ex.getMessage(),
                e.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
      }
    });
    c.gridx = 0;
    c.gridy = 0;
    writePanel.add(fileName, c);
    c.gridy = 1;
    writePanel.add(textFileCheck, c);
    c.gridy = 2;
    writePanel.add(svgFileCheck, c);
    c.gridy = 3;
    writePanel.add(writeButton, c);

    fileWriter.add(writePanel);
    fileWriter.pack();
    fileWriter.setVisible(true);
  }
}
