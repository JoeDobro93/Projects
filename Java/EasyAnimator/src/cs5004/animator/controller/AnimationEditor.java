package cs5004.animator.controller;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.stream.Stream;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.JComboBox;
import javax.swing.JTextField;
import javax.swing.JLabel;

import cs5004.animator.model.Model;
import cs5004.animator.model.MotionType;
import cs5004.animator.model.ShapeTypes;
import cs5004.animator.view.GraphicView;

/**
 * Editor for our model.  While this is visual and does represent some data from the model, it is
 * not a view and is purely for changing aspects in our model.
 */
public class AnimationEditor extends JFrame {
  private GridBagConstraints c;
  //drop down selector JComboBoxes;
  String[] motionList;
  String[] shapeList;
  String[] shapeTypeList;

  // Add Command relevant parameters
  private JComboBox<String> motionSelAddCom;
  private JComboBox<String> shapeSelAddCom;
  private JPanel addCommandPanel;
  private JPanel paramRow;
  private JTextField startTime;
  private JTextField endTime;
  private JPanel moveParamPanel;
  private JTextField startX;
  private JTextField startY;
  private JTextField endX;
  private JTextField endY;
  private JPanel scaleParamPanel;
  private JTextField startW;
  private JTextField startH;
  private JTextField endW;
  private JTextField endH;
  private JPanel colorParamPanel;
  private JTextField startR;
  private JTextField startG;
  private JTextField startB;
  private JTextField endR;
  private JTextField endG;
  private JTextField endB;

  // Remove Command relevant parameters
  private JComboBox<String> motionSelRemoveCom;
  private JComboBox<String> shapeSelRemoveCom;
  private JComboBox<String> motionTimeSel;
  private JPanel removeCommandPanel;

  // Add Shape relevant parameters
  private JPanel addShapePanel;
  private JTextField newShapeName;
  private JComboBox<String> shapeType;
  private JTextField initX;
  private JTextField initY;
  private JTextField initW;
  private JTextField initH;
  private JTextField initR;
  private JTextField initG;
  private JTextField initB;
  private JTextField initStart;
  private JTextField initEnd;

  // Remove Shape relevant parameters
  private JPanel removeShapePanel;
  private JComboBox<String> shapeSelRemoveShape;

  // Our view and model
  private Model model;
  private GraphicView view;

  /**
   * Takes in the view and model to interact with.  Sets up all editor fields.  Protected because
   * only the controller should be make one of these.
   * @param model model, which we commit our changes to.
   * @param view mostly not needed, but for updating the view
   */
  public AnimationEditor(Model model, GraphicView view) {
    super();
    this.model = model;
    this.view = view;

    this.setTitle("Animation Editor Pro");
    this.setSize(400, 300);
    this.setDefaultCloseOperation(JFrame.HIDE_ON_CLOSE);

    //===== Initialize ComboBox Fields =====
    c = new GridBagConstraints();
    motionList = Stream.of(MotionType.values()).map(MotionType::toString)
            .toArray(String[]::new);
    shapeList = model.getShapesShells().keySet().toArray(String[]::new);
    shapeTypeList = Stream.of(ShapeTypes.values()).map(ShapeTypes::name)
            .toArray(String[]::new);

    //END== Initialize Time Input Fields =====

    initializeAddCommand();
    initializeRemoveCommand();
    initializeAddShape();
    initializeRemoveShape();
    JPanel editorGui = new JPanel(new GridBagLayout());
    c.gridx = 0;
    c.gridy = 0;
    c.insets = new Insets(10, 10, 20, 10);
    editorGui.add(addCommandPanel, c);
    c.gridy = 1;
    editorGui.add(removeCommandPanel, c);
    c.gridy = 2;
    editorGui.add(addShapePanel, c);
    c.gridy = 3;
    editorGui.add(removeShapePanel, c);

    this.add(editorGui, BorderLayout.CENTER);

    this.pack();
  }

  /**
   * Sets up the Add Command section.  Editable parameters change when selecting the type of move.
   */
  private void initializeAddCommand() {
    addCommandPanel = new JPanel(new GridBagLayout());
    JLabel addCommandLabel = new JLabel("----Add Command----");
    // activate the Add Command Button
    JButton addCommand = new JButton("Add Command");
    addCommand.addActionListener(taskPerformer);
    addCommand.setActionCommand("Add");
    // populate that shape and command lists for add.  They must be independent from the remove
    // command and remove shape combo boxes.  Despite the same content, if they use the same
    // combobox objects, when one changes they all change, which I do not want.
    shapeSelAddCom = new JComboBox<>(shapeList);
    motionSelAddCom = new JComboBox<>(motionList);
    motionSelAddCom.setActionCommand("Motion");
    motionSelAddCom.addActionListener(taskPerformer);
    // create all parameter input text fields.
    initializeParamBoxes();

    //===== Initialize Time Input Fields =====
    JPanel timePanelAdd = new JPanel(new GridBagLayout());
    JLabel startTimeLabel = new JLabel("Start:");
    JLabel endTimeLabel = new JLabel("End:");
    startTime = new JTextField();
    startTime.setColumns(4);
    endTime = new JTextField();
    endTime.setColumns(4);
    c.gridx = 0;
    c.gridy = 0;
    timePanelAdd.add(startTimeLabel, c);
    c.gridx = 1;
    timePanelAdd.add(startTime, c);
    c.gridx = 2;
    timePanelAdd.add(endTimeLabel, c);
    c.gridx = 3;
    timePanelAdd.add(endTime, c);
    //END== Initialize Time Input Fields =====

    // position cop row components: Shape Selector | Motion Selector | Time inputs
    JPanel topRow = new JPanel(new GridBagLayout());
    c.gridx = 0;
    c.gridy = 0;
    topRow.add(shapeSelAddCom, c);
    c.gridx = 1;
    topRow.add(motionSelAddCom, c);
    c.gridx = 2;
    topRow.add(timePanelAdd, c);

    // position the top row above the parameter row.
    c.gridx = 0;
    c.gridy = 0;
    addCommandPanel.add(addCommandLabel, c);
    c.gridy = 1;
    addCommandPanel.add(topRow, c);
    c.gridy = 2;
    paramRow.setPreferredSize(new Dimension(500, 58));
    addCommandPanel.add(paramRow, c);
    c.gridy = 3;
    addCommandPanel.add(addCommand, c);

    motionSelAddCom.setSelectedIndex(0);
  }

  // helper function for initializeAddcommand to keep this repetitious setup code separate.
  private void initializeParamBoxes() {
    //this will dynamically change based on selected motion.
    paramRow = new JPanel(new GridBagLayout());
    moveParamPanel = new JPanel(new GridBagLayout());
    scaleParamPanel = new JPanel(new GridBagLayout());
    colorParamPanel = new JPanel(new GridBagLayout());
    Dimension textSize = new Dimension(58, 29);
    //===== Move Panel =====
    startX = new JTextField();
    startX.setPreferredSize(textSize);
    startY = new JTextField();
    startY.setPreferredSize(textSize);
    endX = new JTextField();
    endX.setPreferredSize(textSize);
    endY = new JTextField();
    endY.setPreferredSize(textSize);
    c.gridx = 0;
    c.gridy = 0;
    moveParamPanel.add(new JLabel("Start x:"), c);
    c.gridx = 1;
    moveParamPanel.add(startX, c);
    c.gridx = 2;
    moveParamPanel.add(new JLabel("Start y:"), c);
    c.gridx = 3;
    moveParamPanel.add(startY, c);
    c.gridx = 0;
    c.gridy = 1;
    moveParamPanel.add(new JLabel("End x:"), c);
    c.gridx = 1;
    moveParamPanel.add(endX, c);
    c.gridx = 2;
    moveParamPanel.add(new JLabel("End y:"), c);
    c.gridx = 3;
    moveParamPanel.add(endY, c);
    moveParamPanel.setVisible(false);
    //===== End Move Panel ====
    //===== Scale Panel =====
    startW = new JTextField();
    startW.setPreferredSize(textSize);
    startH = new JTextField();
    startH.setPreferredSize(textSize);
    endW = new JTextField();
    endW.setPreferredSize(textSize);
    endH = new JTextField();
    endH.setPreferredSize(textSize);
    c.gridx = 0;
    c.gridy = 0;
    scaleParamPanel.add(new JLabel("Start width:"), c);
    c.gridx = 1;
    scaleParamPanel.add(startW, c);
    c.gridx = 2;
    scaleParamPanel.add(new JLabel("Start height:"), c);
    c.gridx = 3;
    scaleParamPanel.add(startH, c);
    c.gridx = 0;
    c.gridy = 1;
    scaleParamPanel.add(new JLabel("End width:"), c);
    c.gridx = 1;
    scaleParamPanel.add(endW, c);
    c.gridx = 2;
    scaleParamPanel.add(new JLabel("End height:"), c);
    c.gridx = 3;
    scaleParamPanel.add(endH, c);
    scaleParamPanel.setVisible(false);
    //===== END Scale Panel =====
    //===== Color Panel =====
    startR = new JTextField();
    startR.setPreferredSize(textSize);
    startG = new JTextField();
    startG.setPreferredSize(textSize);
    startB = new JTextField();
    startB.setPreferredSize(textSize);
    endR = new JTextField();
    endR.setPreferredSize(textSize);
    endG = new JTextField();
    endG.setPreferredSize(textSize);
    endB = new JTextField();
    endB.setPreferredSize(textSize);
    c.gridx = 0;
    c.gridy = 0;
    colorParamPanel.add(new JLabel("Start red:"), c);
    c.gridx = 1;
    colorParamPanel.add(startR, c);
    c.gridx = 2;
    colorParamPanel.add(new JLabel("Start green:"), c);
    c.gridx = 3;
    colorParamPanel.add(startG, c);
    c.gridx = 4;
    colorParamPanel.add(new JLabel("Start blue:"), c);
    c.gridx = 5;
    colorParamPanel.add(startB, c);
    c.gridy = 1;
    c.gridx = 0;
    colorParamPanel.add(new JLabel("End red:"), c);
    c.gridx = 1;
    colorParamPanel.add(endR, c);
    c.gridx = 2;
    colorParamPanel.add(new JLabel("End green:"), c);
    c.gridx = 3;
    colorParamPanel.add(endG, c);
    c.gridx = 4;
    colorParamPanel.add(new JLabel("End blue:"), c);
    c.gridx = 5;
    colorParamPanel.add(endB, c);
    colorParamPanel.setVisible(false);
    //===== END Color Panel=====
    //only one should show at a time, but added the gridconstraints just in case
    c.gridx = 0;
    c.gridy = 0;
    paramRow.add(moveParamPanel, c);
    c.gridy = 1;
    paramRow.add(scaleParamPanel, c);
    c.gridy = 2;
    paramRow.add(colorParamPanel, c);
  }

  // clears the param text boxes.
  private void clearParamInput() {
    startX.setText("");
    startY.setText("");
    endX.setText("");
    endY.setText("");
    startW.setText("");
    startH.setText("");
    endW.setText("");
    endH.setText("");
    startR.setText("");
    startG.setText("");
    startB.setText("");
    endR.setText("");
    endG.setText("");
    endB.setText("");
  }

  /**
   * Sets up the Remove Command section.  Uses JComboBoxes to select a command based on the start
   * time to remove.
   */
  private void initializeRemoveCommand() {
    removeCommandPanel = new JPanel(new GridBagLayout());
    JLabel removeCommandLabel = new JLabel("----Remove Command----");
    JPanel removeRow = new JPanel(new GridBagLayout());
    JButton removeCommandButton = new JButton("Remove");
    removeCommandButton.addActionListener(taskPerformer);
    removeCommandButton.setActionCommand("Remove");

    shapeSelRemoveCom = new JComboBox<>(shapeList);
    shapeSelRemoveCom.addActionListener(taskPerformer);
    shapeSelRemoveCom.setActionCommand("Update Remove");
    motionSelRemoveCom = new JComboBox<>(motionList);
    motionSelRemoveCom.addActionListener(taskPerformer);
    motionSelRemoveCom.setActionCommand("Update Remove");
    motionTimeSel = new JComboBox<>(getTimeList());

    c.gridx = 0;
    c.gridy = 0;
    removeRow.add(shapeSelRemoveCom, c);
    c.gridx = 1;
    removeRow.add(motionSelRemoveCom, c);
    c.gridx = 2;
    removeRow.add(motionTimeSel, c);
    c.gridx = 3;
    removeRow.add(removeCommandButton, c);

    c.gridx = 0;
    removeCommandPanel.add(removeCommandLabel, c);
    c.gridy = 1;
    removeCommandPanel.add(removeRow, c);
  }

  /**
   * Helper function for remove command section.  When this is called, the ComboBox of frame numbers
   * is updated with the current information for the highlighted command type on the highlighted
   * shape.
   *
   * @return array of relevant times for currently selected command.
   */
  private String[] getTimeList() {
    try {
      return model.getShapesShells().get(shapeSelRemoveCom.getSelectedItem().toString())
              .getCommandList().stream().filter(s -> s.getType()
                      == MotionType.valueOf(motionSelRemoveCom.getSelectedItem().toString()))
              .map(s -> "Frame " + s.getTimes()[0])
              .toArray(String[]::new);
    } catch (Exception e) {
      return null;
    }
  }

  /**
   * Sets up the Add Shape Command section.
   */
  private void initializeAddShape() {
    addShapePanel = new JPanel(new GridBagLayout());
    JButton addShapeButton = new JButton("Add Shape");
    addShapeButton.addActionListener(taskPerformer);
    addShapeButton.setActionCommand("Add Shape");

    JLabel addShapeLabel = new JLabel("----Add Shape----");
    JPanel topRow = new JPanel(new GridBagLayout());
    JLabel shapeNameLabel = new JLabel("Name:");
    newShapeName = new JTextField();
    newShapeName.setColumns(10);
    JLabel shapeTypeLabel = new JLabel("Type:");
    shapeType = new JComboBox<String>(shapeTypeList);
    topRow.add(shapeNameLabel);
    topRow.add(newShapeName);
    topRow.add(shapeTypeLabel);
    topRow.add(shapeType);

    JPanel paramRow1 = new JPanel(new GridBagLayout());
    JLabel initXLabel = new JLabel("X:");
    initX = new JTextField();
    initX.setColumns(4);
    JLabel initYLabel = new JLabel("Y:");
    initY = new JTextField();
    initY.setColumns(4);
    JLabel initWLabel = new JLabel("Width:");
    initW = new JTextField();
    initW.setColumns(4);
    JLabel initHLabel = new JLabel("Height:");
    initH = new JTextField();
    initH.setColumns(4);
    c.gridx = 0;
    c.gridy = 0;
    paramRow1.add(initXLabel, c);
    c.gridx = 1;
    paramRow1.add(initX, c);
    c.gridx = 2;
    paramRow1.add(initYLabel, c);
    c.gridx = 3;
    paramRow1.add(initY, c);
    c.gridx = 4;
    paramRow1.add(initWLabel, c);
    c.gridx = 5;
    paramRow1.add(initW, c);
    c.gridx = 6;
    paramRow1.add(initHLabel, c);
    c.gridx = 7;
    paramRow1.add(initH, c);

    JPanel paramRow2 = new JPanel(new GridBagLayout());
    JLabel initRLabel = new JLabel("Red:");
    initR = new JTextField();
    initR.setColumns(4);
    JLabel initGLabel = new JLabel("Green:");
    initG = new JTextField();
    initG.setColumns(4);
    JLabel initBLabel = new JLabel("Blue:");
    initB = new JTextField();
    initB.setColumns(4);
    c.gridx = 0;
    paramRow2.add(initRLabel, c);
    c.gridx = 1;
    paramRow2.add(initR, c);
    c.gridx = 2;
    paramRow2.add(initGLabel, c);
    c.gridx = 3;
    paramRow2.add(initG, c);
    c.gridx = 4;
    paramRow2.add(initBLabel, c);
    c.gridx = 5;
    paramRow2.add(initB, c);

    JPanel paramRow3 = new JPanel(new GridBagLayout());
    JLabel initStartLabel = new JLabel("Start:");
    initStart = new JTextField();
    initStart.setColumns(4);
    JLabel initEndLabel = new JLabel("End:");
    initEnd = new JTextField();
    initEnd.setColumns(4);
    c.gridx = 0;
    paramRow3.add(initStartLabel);
    c.gridx = 1;
    paramRow3.add(initStart);
    c.gridx = 2;
    paramRow3.add(initEndLabel);
    c.gridx = 3;
    paramRow3.add(initEnd);

    c.gridx = 0;
    c.gridy = 0;
    addShapePanel.add(addShapeLabel, c);
    c.gridy = 1;
    addShapePanel.add(topRow, c);
    c.gridy = 2;
    addShapePanel.add(paramRow1, c);
    c.gridy = 3;
    addShapePanel.add(paramRow2, c);
    c.gridy = 4;
    addShapePanel.add(paramRow3, c);
    c.gridy = 5;
    addShapePanel.add(addShapeButton, c);

  }

  /**
   * Sets up the Remove Shape section.  Editable parameters change when selecting the type of move.
   */
  private void initializeRemoveShape() {
    JLabel removeShapeLabel = new JLabel("----Remove Shape----");
    removeShapePanel = new JPanel(new GridBagLayout());
    JButton removeShapeButton = new JButton("Remove Shape");
    removeShapeButton.setActionCommand("Remove Shape");
    removeShapeButton.addActionListener(taskPerformer);
    shapeSelRemoveShape = new JComboBox<>(shapeList);
    JLabel shapeSelectLabel = new JLabel("Choose Shape");

    JPanel removeShapeRow = new JPanel(new GridBagLayout());
    removeShapeRow.add(shapeSelectLabel);
    removeShapeRow.add(shapeSelRemoveShape);
    removeShapeRow.add(removeShapeButton);

    c.gridx = 0;
    c.gridy = 0;
    removeShapePanel.add(removeShapeLabel, c);
    c.gridy = 1;
    removeShapePanel.add(removeShapeRow, c);
  }

  /**
   * Adding a self contained listener here separate from the controller, since this is really just
   * another controller add-on and the ActionListener functions in Controller are supposed to be
   * focused on the View.
   * Commands handled:
   * Motion: For the Add Command section, updated the visible editable parameters with the relevant
   *      parameters for the current type of motion user wants to add.
   * Add: Takes parameters in Add Command section and tries to add them to the model.  Popup error
   *      message if fails with the reason displayed.
   * Update Remove: Updates list of moition times in the Remove command list.  Also updated after
   *      a command is added.
   * Remove: Removes selected command from a shape based on start frame.
   * Add Shape: Add a new shape with initial parameters to the model based on fields in the Add
   *      Shape sections.
   * Remove Shape: Remove selected shape from the model based on selection from Remove Shape section
   */
  ActionListener taskPerformer = new ActionListener() {
    public void actionPerformed(ActionEvent e) {
      switch (e.getActionCommand()) {
        case "Motion":
          try {
            switch (motionSelAddCom.getSelectedItem().toString()) {
              case "MOVE":
                scaleParamPanel.setVisible(false);
                colorParamPanel.setVisible(false);
                moveParamPanel.setVisible(true);
                break;
              case "SCALE":
                colorParamPanel.setVisible(false);
                moveParamPanel.setVisible(false);
                scaleParamPanel.setVisible(true);
                break;
              case "RECOLOR":
                moveParamPanel.setVisible(false);
                scaleParamPanel.setVisible(false);
                colorParamPanel.setVisible(true);
                break;
              default:
                moveParamPanel.setVisible(false);
                scaleParamPanel.setVisible(false);
                colorParamPanel.setVisible(false);
            }
          } catch (NullPointerException npe) {
            JOptionPane.showMessageDialog(null, "field was null",
                    "NullPointerException", JOptionPane.PLAIN_MESSAGE);
          }
          break;

        case "Add":
          try {
            switch (motionSelAddCom.getSelectedItem().toString()) {
              case "MOVE":
                model.moveShape(shapeSelAddCom.getSelectedItem().toString(),
                        Integer.parseInt(startX.getText()),
                        Integer.parseInt(startY.getText()),
                        Integer.parseInt(endX.getText()),
                        Integer.parseInt(endY.getText()),
                        Integer.parseInt(startTime.getText()),
                        Integer.parseInt(endTime.getText()));
                break;
              case "SCALE":
                model.scaleShape(shapeSelAddCom.getSelectedItem().toString(),
                        Integer.parseInt(startW.getText()),
                        Integer.parseInt(startH.getText()),
                        Integer.parseInt(endW.getText()),
                        Integer.parseInt(endH.getText()),
                        Integer.parseInt(startTime.getText()),
                        Integer.parseInt(endTime.getText()));
                break;
              case "RECOLOR":
                model.changeShapeColor(shapeSelAddCom.getSelectedItem().toString(),
                        Integer.parseInt(startR.getText()),
                        Integer.parseInt(startG.getText()),
                        Integer.parseInt(startB.getText()),
                        Integer.parseInt(endR.getText()),
                        Integer.parseInt(endG.getText()),
                        Integer.parseInt(endB.getText()),
                        Integer.parseInt(startTime.getText()),
                        Integer.parseInt(endTime.getText()));
                break;
              default:
                JOptionPane.showMessageDialog(null, "Unhandled motion type");
            }
            view.refreshShapes(model.getShapes());
            clearParamInput();
          } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(null, "Make sure all inputs are integers",
                    ex.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
          } catch (Exception ex) {
            JOptionPane.showMessageDialog(null, ex.getMessage(),
                    ex.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
          }
          //intentionally no break.  I want the motionTimeSel ComboBox to update after a command is
          // added.
          // update the frames with given command in frame list
        case "Update Remove":
          motionTimeSel.removeAllItems();
          if (getTimeList() != null) {
            for (String frameNum : getTimeList()) {
              motionTimeSel.addItem(frameNum);
            }
          }
          break;
        case "Remove":
          try {
            model.removeCommand(shapeSelRemoveCom.getSelectedItem().toString(),
                    MotionType.valueOf(motionSelRemoveCom.getSelectedItem().toString()),
                    Integer.parseInt(motionTimeSel.getSelectedItem().toString()
                            .substring(6)));
            motionTimeSel.removeItemAt(motionTimeSel.getSelectedIndex());
          } catch (Exception ex) {
            JOptionPane.showMessageDialog(null, ex.getMessage(),
                    ex.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
          }
          break;
        case "Add Shape":
          try {
            int x = Integer.parseInt(initX.getText());
            int y = Integer.parseInt(initY.getText());
            int w = Integer.parseInt(initW.getText());
            int h = Integer.parseInt(initH.getText());
            int r = Integer.parseInt(initR.getText());
            int g = Integer.parseInt(initG.getText());
            int b = Integer.parseInt(initB.getText());

            model.addShape(newShapeName.getText(),
                    ShapeTypes.valueOf(shapeType.getSelectedItem().toString()),
                    x, y, w, h, r, g, b, Integer.parseInt(initStart.getText()),
                    Integer.parseInt(initEnd.getText()));
            view.refreshShapes(model.getShapes());
            shapeSelRemoveShape.addItem(newShapeName.getText());
            shapeSelAddCom.addItem(newShapeName.getText());
            shapeSelRemoveCom.addItem(newShapeName.getText());

          } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(null, "Make sure all inputs are integers",
                    ex.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
          } catch (Exception ex) {
            JOptionPane.showMessageDialog(null, ex.getMessage(),
                    ex.getClass().toString(), JOptionPane.PLAIN_MESSAGE);
          }
          break;
        case "Remove Shape":
          try {
            model.removeShape(shapeSelRemoveShape.getSelectedItem().toString());
            view.refreshShapes(model.getShapes());
            Object removeMe = shapeSelRemoveShape.getSelectedItem();
            shapeSelRemoveShape.removeItem(removeMe);
            shapeSelAddCom.removeItem(removeMe);
            shapeSelRemoveCom.removeItem(removeMe);

          } catch (Exception ex) {
            JOptionPane.showMessageDialog(null, ex.getMessage(),
                    "No shape to remove", JOptionPane.PLAIN_MESSAGE);
          }
          break;
        default:
          JOptionPane.showMessageDialog(null, "Unhandled action");
      }
    }
  };

}
