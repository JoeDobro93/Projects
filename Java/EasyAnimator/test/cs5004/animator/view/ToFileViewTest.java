package cs5004.animator.view;

import org.junit.Before;
import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;

import cs5004.animator.model.Model;
import cs5004.animator.model.ModelImpl;
import cs5004.animator.model.ShapeTypes;

import static org.junit.Assert.assertEquals;

/**
 * Tests the svg and text interfaces by redirecting system.out to a local variable and comparing the
 * result.  Each type of shape and motion is tested.
 */
public class ToFileViewTest {
  private Model model;

  @Before
  public void setUp() throws Exception {
    // set up the model with one of each shape and each motion.
    int[] canvas = new int[]{1, 2, 3, 4};
    model = new ModelImpl(canvas);
    model.addShape("C", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7,
            0, 10);
    model.addShape("R", ShapeTypes.RECTANGLE, 11, 12, 13, 14, 15, 16, 17,
            10, 20);
    model.moveShape("C", 1, 2, 3, 4, 5, 10);
    model.scaleShape("R", 11, 12, 13, 14, 15, 20);
    model.changeShapeColor("C", 5, 6, 7, 100, 200, 250, 0, 10);
  }

  @Test
  public void SVGTest() {
    String expectedHeader = "<svg width=\"3\" height=\"4\" version=\"1.1\" viewBox=\"1 2 3 4\"\n"
            + "\txmlns=\"http://www.w3.org/2000/svg\">";

    String expectedShape1 = "<ellipse id=\"C\" cx=\"2\" cy=\"4\" rx=\"1\" ry=\"2\" "
            + "fill=\"rgb(5,6,7)\" visibility=\"hidden\" >";

    String expectedShape2 = "<rect id=\"R\" x=\"11\" y=\"12\" width=\"13\" height=\"14\" "
            + "fill=\"rgb(15,16,17)\" visibility=\"hidden\" >";

    String expectedShape1Moves = "\t<set attributeName=\"visibility\" to=\"visible\" "
            + "begin=\"0ms\" dur=\"100ms\" fill=\"freeze\" />\n"
            + "\t<animate attributeType=\"xml\" begin=\"0.0ms\" dur=\"100.0ms\" "
            + "attributeName=\"fill\" from=\"rgb(5,6,7)\" to=\"rgb(100,200,250)\" "
            + "fill=\"freeze\" />\n"
            + "\t<animate attributeType=\"xml\" begin=\"50.0ms\" dur=\"50.0ms\" "
            + "attributeName=\"cx\" from=\"2\" to=\"4\" fill=\"freeze\" />\n"
            + "\t<animate attributeType=\"xml\" begin=\"50.0ms\" dur=\"50.0ms\" "
            + "attributeName=\"cy\" from=\"4\" to=\"6\" fill=\"freeze\" />";

    String expectedShape2Moves = "\t<set attributeName=\"visibility\" to=\"visible\" "
            + "begin=\"100ms\" dur=\"200ms\" fill=\"freeze\" />\n"
            + "\t<animate attributeType=\"xml\" begin=\"150.0ms\" dur=\"50.0ms\" "
            + "attributeName=\"width\" from=\"11\" to=\"13\" fill=\"freeze\" />\n"
            + "\t<animate attributeType=\"xml\" begin=\"150.0ms\" dur=\"50.0ms\" "
            + "attributeName=\"height\" from=\"12\" to=\"14\" fill=\"freeze\" />";

    String fullString = expectedHeader + "\n\n"
            + expectedShape1 + "\n" + expectedShape1Moves + "\n</ellipse>\n"
            + expectedShape2 + "\n" + expectedShape2Moves + "\n</rect>\n"
            + "</svg>\n";

    // set the output to our bytestream, helps test since we can't access private methods.
    ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
    PrintStream newOut = new PrintStream(byteStream);
    PrintStream defaultOut = System.out;
    new SVGViewImpl(model, newOut, 10);
    //set back to original when we're done.
    System.setOut(defaultOut);

    assertEquals(fullString, byteStream.toString());

  }

  @Test
  public void TextViewTest() {
    String expectedHeader = "Canvas size: 3 x 4\n"
            + "x/y offset: (1, 2)";

    String expectedShape1 = "---------------------------------\n"
            + "Shape Name: C\n"
            + "\tType: Ellipse\n"
            + "\tTime Range: t=0 to t=10\n"
            + "\tInitial parameters:\n"
            + "\t\tCenter: (1.00, 2.00)\n"
            + "\t\tSize: Diameter X - 3.00, Diameter Y - 4.00\n"
            + "\t\tColor: r=5, g=6, b=7";

    String expectedShape2 = "---------------------------------\n"
            + "Shape Name: R\n"
            + "\tType: Rectangle\n"
            + "\tTime Range: t=10 to t=20\n"
            + "\tInitial parameters:\n"
            + "\t\tTop left corner: (11.00, 12.00)\n"
            + "\t\tSize: Width - 13.00, Height - 14.00\n"
            + "\t\tColor: r=15, g=16, b=17";

    String expectedShape1Moves = "\tCommands:\n"
            + "\t\tRECOLOR from (r=5,g=6,b=7) to (r=100,g=200,b=250) from t=0 to t=10\n"
            + "\t\tMOVE from (1,2) to (3,4) from t=5 to t=10";

    String expectedShape2Moves = "\tCommands:\n"
            + "\t\tSCALE from x size 11, y size 12 to x size 13, y size 14 "
            + "from t=15 to t=20";

    String fullString = expectedHeader + "\n\n"
            + expectedShape1 + "\n\n" + expectedShape1Moves + "\n"
            + expectedShape2 + "\n\n" + expectedShape2Moves + "\n"
            + "---------------------------------\n"; //needs ending /n from console


    // set the output to our bytestream, helps test since we can't access private methods.
    ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
    PrintStream newOut = new PrintStream(byteStream);
    PrintStream defaultOut = System.out;
    new TextViewImpl(model, newOut);
    //set back to original when we're done.
    System.setOut(defaultOut);
    assertEquals(fullString, byteStream.toString());
  }
}