package cs5004.animator.view;

import java.awt.Dimension;
import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionListener;
import java.awt.event.KeyListener;
import java.util.LinkedList;

import javax.swing.JFrame;
import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.JCheckBox;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JScrollPane;

import cs5004.animator.shape.Shape;

/**
 * Our graphic view with GUI added on.  Top part is the animationPanel while the bottom section
 * contains controls.  All controls are mapped to an action command, which is interpreted by an
 * ActionListener based Controller.
 */
public class GraphicViewImpl extends JFrame implements GraphicView {
  private AnimationPanel animationPanel;
  JButton playButton;
  JCheckBox loopToggle;
  JTextField speedField;
  JButton loadButton;
  JButton animationEditor;
  JButton writeFile;

  /**
   * Initializes all JComponents and places them in their desired locations, mostly within JPanels
   * using GridBagLayout.  May have been made more concise if I broke this into functions like I
   * did in AnimationEditor.
   *
   * @param shapes list of shapes sent by the controller from the model.
   * @param bounds Bounds for the AnimationPanel to set its size.
   */
  public GraphicViewImpl(LinkedList<Shape> shapes, int[] bounds) {
    super();

    this.setTitle("Animayshun");
    this.setSize(10, 10);
    this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    JScrollPane scrollPane;

    this.setLayout(new BorderLayout());
    animationPanel = new AnimationPanel(shapes, bounds[0], bounds[1]);
    animationPanel.setPreferredSize(new Dimension(bounds[2], bounds[3]));
    scrollPane = new JScrollPane(animationPanel);
    //=====-Playback Controls-=====

    JPanel controlPane = new JPanel(new GridBagLayout());
    playButton = new JButton("Play");
    playButton.setActionCommand("Playback");
    playButton.setPreferredSize(new Dimension(75, 29));

    JLabel loopLabel = new JLabel("Loop");
    JPanel loopPanel = new JPanel(new GridBagLayout());
    loopToggle = new JCheckBox();
    loopToggle.setActionCommand("Loop");

    JLabel speedLabel = new JLabel("Speed:");
    JPanel speedPanel = new JPanel();
    speedField = new JTextField();
    speedField.setActionCommand("Speed");
    speedField.setColumns(5);

    loadButton = new JButton("Load File");
    loadButton.setActionCommand("Load");

    animationEditor = new JButton("Animation Editor Pro");
    animationEditor.setActionCommand("Edit");

    writeFile = new JButton("Save file");
    writeFile.setActionCommand("Write");

    //=====-Additional Stuff-=====

    GridBagConstraints c = new GridBagConstraints();
    Insets noInset = new Insets(0, 0, 0, 0);
    Insets inset = new Insets(0, 5, 0, 5);

    c.gridx = 0;
    c.gridy = 0;
    speedPanel.add(speedLabel, c);
    loopPanel.add(loopLabel, c);
    c.insets = inset;
    controlPane.add(playButton, c);
    c.insets = noInset;
    c.gridx = 1;
    speedPanel.add(speedField, c);
    loopPanel.add(loopToggle, c);
    c.insets = inset;
    controlPane.add(loopPanel, c);
    c.gridx = 2;
    controlPane.add(speedPanel, c);
    c.gridx = 0;
    c.gridy = 1;
    controlPane.add(loadButton, c);
    c.gridx = 1;
    controlPane.add(animationEditor, c);
    c.gridx = 2;
    controlPane.add(writeFile, c);

    this.add(controlPane, BorderLayout.SOUTH);
    this.add(scrollPane, BorderLayout.NORTH);

    this.pack();
    this.setVisible(true);
  }

  @Override
  public void setListener(ActionListener e) {
    playButton.addActionListener(e);
    loopToggle.addActionListener(e);
    speedField.addActionListener(e);
    loadButton.addActionListener(e);
    animationEditor.addActionListener(e);
    writeFile.addActionListener(e);
  }

  @Override
  public void playPause(String message) {
    this.playButton.setText(message);
  }

  @Override
  public void setKeyListener(KeyListener k) {
    loopToggle.addKeyListener(k);
    playButton.addKeyListener(k);
    loadButton.addKeyListener(k);
    animationEditor.addKeyListener(k);
  }

  @Override
  public String getSpeed() {
    String speed = speedField.getText();
    speedField.setText("");
    return speed;
  }

  @Override
  public void toggleLoop() {
    loopToggle.setSelected(!loopToggle.isSelected());
  }

  @Override
  public void repaintPanel() {
    this.repaint();
  }

  @Override
  public void fileChange(LinkedList<Shape> shapes, int[] bounds) {
    Dimension size = new Dimension(bounds[2], bounds[3]);
    animationPanel.setShapes(shapes);
    animationPanel.setOffset(bounds[0], bounds[1]);
    //can't resize without changing the preferred size first, since we set that earlier on.
    animationPanel.setPreferredSize(size);
    animationPanel.setSize(size);
    this.pack();
  }

  @Override
  public void refreshShapes(LinkedList<Shape> shapes) {
    animationPanel.setShapes(shapes);
  }
}
