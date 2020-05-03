package cs5004.animator.model;

import org.junit.Before;
import org.junit.Test;

import cs5004.animator.model.ShapeShell;
import cs5004.animator.shape.Shape;
import cs5004.animator.shape.Oval;

import static org.junit.Assert.assertEquals;

/**
 * Tests for inputs coming into a shape shell and constructors.
 */
public class ShapeShellTest {
  private ShapeShell shell1;
  private Shape shape1;

  @Before
  public void setUp() {
    shape1 = new Oval(4, 6.3);
    shell1 = new ShapeShell(shape1, 0, 8);
  }

  @Test
  public void badConstructorInputTest() {
    ShapeShell failMe;
    try {
      failMe = new ShapeShell(null, 0, 10);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape cannot be null", e.getMessage());
    }

    try {
      failMe = new ShapeShell(shape1, -3, 10);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Times cannot be negative", e.getMessage());
    }

    try {
      failMe = new ShapeShell(shape1, 3, -10);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Times cannot be negative", e.getMessage());
    }

    try {
      failMe = new ShapeShell(shape1, 10, 9);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Cannot disappear before appearing", e.getMessage());
    }

  }

  /**
   * Testing the getters on a newly initialized ShapeShell object.
   */
  @Test
  public void gettersTest() {
    assertEquals(shape1, shell1.getShape());
    assertEquals(0, shell1.getCommandList().size());
    assertEquals(2, shell1.getAppearDisappearTimes().length);
    assertEquals(0, shell1.getAppearDisappearTimes()[0]);
    assertEquals(8, shell1.getAppearDisappearTimes()[1]);
  }
}