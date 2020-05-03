package cs5004.animator;

import org.junit.Before;
import org.junit.Test;

import java.io.StringReader;

import static org.junit.Assert.assertEquals;

/**
 * Test my EasyAnimator public methods for exception handling as expected.
 */
public class EasyAnimatorTest {
  EasyAnimator testEasyAnimator;

  @Before
  public void setUp() throws Exception {
    testEasyAnimator = new EasyAnimator();
  }

  /**
   * Does not test for passing tests because they cannot initialize properly outside of the jar.
   * However, I am able to make sure it throws expected exceptions with their messages.
   */
  @Test
  public void readFile() {
    try {
      testEasyAnimator.readFile("NoProper-inHere");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("No file input set", e.getMessage());
    }

    try {
      testEasyAnimator.readFile("stuff junk -in ");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("No argument after -in", e.getMessage());
    }

    try {
      testEasyAnimator.readFile("stuff junk -in fileyo");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Could not find file \"fileyo\"", e.getMessage());
    }

    try {
      testEasyAnimator.readFile("-IN -another command");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("No argument after -in", e.getMessage());
    }
  }

  @Test
  public void getView() {
    assertEquals("type", testEasyAnimator.getView("-view type oijsvf"));
    assertEquals("type", testEasyAnimator.getView("oijoij -view type"));
    assertEquals("type", testEasyAnimator.getView("-view type"));

    try {
      testEasyAnimator.getView("NoProper-viewHere");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("No view type set", e.getMessage());
    }

    try {
      testEasyAnimator.getView("stuff junk -view ");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("No argument after -view", e.getMessage());
    }

    try {
      testEasyAnimator.getView("-vIeW -another command");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("No argument after -view", e.getMessage());
    }
  }

  @Test
  public void runView() {
    try {
      testEasyAnimator.runView(null, "", "");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("inFile cannot be null", e.getMessage());
    }

    Readable readable = new StringReader("Hey guys");
    try {
      testEasyAnimator.runView(readable, "bad type", "");
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Could not parse file", e.getMessage());
    }
  }
}