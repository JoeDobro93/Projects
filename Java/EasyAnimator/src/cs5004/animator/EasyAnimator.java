package cs5004.animator;


import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.PrintStream;

import javax.swing.JOptionPane;

import cs5004.animator.controller.Controller;
import cs5004.animator.model.Model;
import cs5004.animator.model.ModelImpl;
import cs5004.animator.util.AnimationReader;
import cs5004.animator.util.MyAnimationBuilder;
import cs5004.animator.view.GraphicView;
import cs5004.animator.view.GraphicViewImpl;
import cs5004.animator.view.JFrameView;
import cs5004.animator.view.SVGViewImpl;
import cs5004.animator.view.TextViewImpl;

/**
 * Main for the animation project.  Accepts commandline arguments and parses them to run specific
 * views.  Junk input ignored as long as all needed parameters are present.  Contains methods to
 * help parse the input rather than handle it all in main in one big lump.  If the view fails to
 * initialize, a popup will appear with the error type and message.  When this is closed, the
 * program will exit.
 */
public final class EasyAnimator {

  /**
   * Takes in a command line and looks for -in.  If present, verifies the file and saves it into a
   * Readable, which is returned at the end of the function.
   *
   * @param line full command line string.
   * @return Readable object to be used by AnimationBuilder.
   * @throws IllegalArgumentException if file doesn't exist or something is wrong with the command.
   */
  public Readable readFile(String line) throws IllegalArgumentException {
    Readable inFile;
    String fileName;

    if (line.toLowerCase().contains("-in ")) {
      // try to parse argument.  If the index is out of bounds, then there is no argument
      try {
        fileName = line.substring(line.toLowerCase().indexOf("-in ") + 4);
        if (fileName.contains(" ")) {
          fileName = fileName.substring(0, fileName.indexOf(" "));
        }
      } catch (Exception e) {
        // tried to parse something out of bounds
        // no way to actually get here, just a precaution.
        throw new IllegalArgumentException("Improper formatting after -in");
      }
      // check that the argument isn't just a command
      if (fileName.length() == 0 || fileName.charAt(0) == '-') {
        throw new IllegalArgumentException("No argument after -in");
      }
      // try to open the file.
      try {
        inFile = new FileReader(fileName);
      } catch (Exception e) {
        throw new IllegalArgumentException(String.format("Could not find file \"%s\"", fileName));
      }
    } else {
      // -in not present in command
      throw new IllegalArgumentException("No file input set");
    }
    return inFile;
  }

  /**
   * Searches through a commandline argument for -view and parses the view type.  This method does
   * not check if it is a valid view, just that there is a valid string following -view.  If valid,
   * the argument is returned as a string.
   *
   * @param line full String of commandline input.
   * @return String of the argument following -view
   * @throws IllegalArgumentException If not formatted correctly or the argument is another
   *                                  command.
   */
  public String getView(String line) throws IllegalArgumentException {
    String viewType;

    if (line.toLowerCase().contains("-view ")) {
      // try to parse the argument.  Throws exception if out of bounds, meaning nothing after -view
      try {
        viewType = line.substring(line.toLowerCase().indexOf("-view ") + 6);
        if (viewType.contains(" ")) {
          viewType = viewType.substring(0, viewType.indexOf(" "));
        }
      } catch (Exception e) {
        throw new IllegalArgumentException("Improper formatting after -view");
      }
      // checks if the argument is just another command.
      if (viewType.length() == 0 || viewType.charAt(0) == '-') {
        throw new IllegalArgumentException("No argument after -view");
      }
    } else {
      throw new IllegalArgumentException("No view type set");
    }
    return viewType;
  }

  /**
   * Private helper function accessed by runView method.  This is called to only when the view type
   * calls for an output file.  This searches the commandline string for -out.  If not present, a
   * blank string is returned, otherwise the file name is returned.
   *
   * @param line full String commandline arguments.
   * @return String file name of destination output or blank String.
   * @throws IllegalArgumentException if something wrong with the formatting of the command.
   */
  private String getOutFileName(String line) throws IllegalArgumentException {
    String outFileName;

    if (line.toLowerCase().contains("-out ")) {
      try {
        outFileName = line.substring(line.toLowerCase().indexOf("-out ") + 5);
        if (outFileName.contains(" ")) {
          outFileName = outFileName.substring(0, outFileName.indexOf(" "));
        }
      } catch (Exception e) {
        throw new IllegalArgumentException("Improper formatting for -out");
      }
      if (outFileName.charAt(0) == '-') {
        throw new IllegalArgumentException("No argument after -out");
      }
    } else {
      // if no output was declared, set to empty.
      outFileName = "";
    }
    return outFileName;
  }

  /**
   * Private helper function accessed within runView.  Only called to by views where speed is
   * relevant.  This looks for -speed in the commandline argument and attempts to parse the argument
   * as an integer.  Will not work if there is a decimal or any other character.  Returns speed as
   * an integer.
   *
   * @param line full String commandline arguments.
   * @return int speed value.
   * @throws IllegalArgumentException if speed is invalid or formatting is incorrect.
   */
  private int getSpeed(String line) throws IllegalArgumentException {
    int speed;
    if (line.toLowerCase().contains("-speed ")) {
      try {
        line = line.substring(line.toLowerCase().indexOf("-speed ") + 7);
        if (line.contains(" ")) {
          line = line.substring(0, line.indexOf(" "));
        }
        speed = Integer.parseInt(line);
      } catch (Exception e) {
        throw new IllegalArgumentException("-speed argument not an integer");
      }
      if (speed <= 0) {
        throw new IllegalArgumentException("Speed must be greater than 0");
      }
    } else {
      // if no speed was declared, set to 1.
      speed = 1;
    }
    return speed;
  }

  /**
   * Determines which view to launch and launches it.
   *
   * @param inFile   input file as a Readable argument.
   * @param viewType String of view type.
   * @param line     full commandline String.
   * @throws IllegalArgumentException if view type doesn't exist or something goes wrong creating
   *                                  the outFile.
   */
  public void runView(Readable inFile, String viewType, String line)
          throws IllegalArgumentException {

    Model model;

    if (inFile == null) {
      throw new IllegalArgumentException("inFile cannot be null");
    }
    try {
      model = AnimationReader.parseFile(inFile, new MyAnimationBuilder<ModelImpl>());
    } catch (Exception e) {
      throw new IllegalArgumentException("Could not parse file");
    }
    String outFileName;
    PrintStream outFile;
    switch (viewType) {
      case "text":
        outFileName = getOutFileName(line);
        if (outFileName.equals("")) {
          outFile = System.out;
        } else {
          try {
            outFile = new PrintStream(new File(outFileName));
          } catch (Exception e) {
            throw new IllegalArgumentException("Problem creating file \"" + outFileName + "\"");
          }
        }
        new TextViewImpl(model, outFile);
        break;
      case "svg":
        outFileName = getOutFileName(line);
        if (outFileName.equals("")) {
          throw new IllegalArgumentException("SVG must have file output");
        } else {
          // if svg not included in filename, add it on.
          if (outFileName.length() < 4
                  || !outFileName.substring(outFileName.length() - 4).equals(".svg")) {
            outFileName += ".svg";
          }
          try {
            outFile = new PrintStream(new File(outFileName));
          } catch (Exception e) {
            throw new IllegalArgumentException("Problem creating file \"" + outFileName + "\"");
          }
        }
        new SVGViewImpl(model, outFile, getSpeed(line));
        break;
      case "visual":
        new JFrameView(model, getSpeed(line));
        break;
      case "playback":
        GraphicView view = new GraphicViewImpl(model.getShapes(), model.getBounds());
        new Controller(model, view, getSpeed(line));
        break;
      default:
        throw new IllegalArgumentException("Invalid view type \"" + viewType + "\"");
    }
  }

  /**
   * Main function to glue together all the parsing related methods and launch the appropriate view.
   * If an error occurs, it is caught and displayed in a JOptionPane before exiting the program.
   *
   * @param args input from commandline.
   */
  public static void main(String[] args) throws FileNotFoundException {
    String line = "";

    // I know I should have handled the args as an array, but I was dumb and tested with strings.
    // Because all my implementation is based around that mistake, this is my workaround.  I'll try
    // to make this better by part 3.
    for (String token : args) {
      line += token + " ";
    }
    line = line.substring(0, line.length() - 1);

    EasyAnimator easyAnimator = new EasyAnimator();

    String viewType;
    Readable inFile;

    try {
      inFile = easyAnimator.readFile(line);
      viewType = easyAnimator.getView(line);
      easyAnimator.runView(inFile, viewType, line);
    } catch (Exception e) {
      JOptionPane.showMessageDialog(null, e.getMessage(), e.getClass().toString(),
              JOptionPane.PLAIN_MESSAGE);
      System.exit(1);
    }
    /*
    Readable inFile = new FileReader("resources/smalldemo.txt");
    Model model = AnimationReader.parseFile(inFile, new MyAnimationBuilder<ModelImpl>());
    GraphicView view = new GraphicViewImpl(model.getShapes(), model.getBounds());
    new Controller(model, view);*/
  }
}
