package cs5004.animator.model;

import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Test for model to make sure it can handle shapes and commands and throw exceptions when expected.
 */
public class ModelTest {
  Model testModel;

  @Before
  public void setUp() throws Exception {
    testModel = new ModelImpl(new int[]{1, 2, 3, 4});
  }

  /**
   * Uses ShapesShell getter to check that the appropriate results are returned when adding/removing
   * any shape.
   */
  @Test
  public void shapesShellsTest() {
    // check that size is 0 at start
    assertEquals(0, testModel.getShapesShells().size());
    // Add shape, test that size is one and contains the name key
    testModel.addShape("Steve", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    assertEquals(1, testModel.getShapesShells().size());
    assertTrue(testModel.getShapesShells().containsKey("Steve"));
    // Add new shape, test that size is two and contains the new name key
    testModel.addShape("Phil", ShapeTypes.RECTANGLE, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    assertEquals(2, testModel.getShapesShells().size());
    assertTrue(testModel.getShapesShells().containsKey("Phil"));
    // Try to add shape with existing key.  Expect exception.
    try {
      testModel.addShape("Steve", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape name already taken", e.getMessage());
    }
    // Remove shape, check that size is back to 1 and the key is no longer present
    testModel.removeShape("Steve");
    assertEquals(1, testModel.getShapesShells().size());
    assertFalse(testModel.getShapesShells().containsKey("Steve"));
    // Remove shape, check that size is back to 0 and the key is no longer present
    testModel.removeShape("Phil");
    assertEquals(0, testModel.getShapesShells().size());
    assertFalse(testModel.getShapesShells().containsKey("Phil"));
    // Try to remove shape not present, then check that size is still 0
    testModel.removeShape("Phil");
    assertEquals(0, testModel.getShapesShells().size());
    assertFalse(testModel.getShapesShells().containsKey("Phil"));
  }

  /**
   * Make sure last frame is returned as expected. This also ends up testing add and remove
   * methods.
   */
  @Test
  public void getLastFrameTest() {
    assertEquals(0, testModel.getLastFrame());

    testModel.addShape("Steve", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    assertEquals(9, testModel.getLastFrame());

    testModel.addShape("Bill", ShapeTypes.RECTANGLE, 1, 2, 3, 4, 5, 6, 7, 1, 5);
    assertEquals(9, testModel.getLastFrame());

    testModel.removeShape("Steve");
    assertEquals(5, testModel.getLastFrame());

    testModel.addShape("Jim", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 8, 30);
    assertEquals(30, testModel.getLastFrame());

    assertEquals(ShapeTypes.OVAL, testModel.getShapesShells().get("Jim").getShape().getType());
    assertEquals(ShapeTypes.RECTANGLE, testModel.getShapesShells().get("Bill").getShape()
            .getType());

    testModel.moveShape("Jim", 1, 2, 3, 4, 0, 32);
    assertEquals(32, testModel.getLastFrame());
    testModel.changeShapeColor("Jim", 1, 2, 3, 4, 5, 6, 32, 34);
    assertEquals(34, testModel.getLastFrame());

    testModel.removeShape("Bill");
    testModel.removeShape("Jim");
    assertEquals(0, testModel.getLastFrame());
  }

  @Test
  public void throwExceptionsTest() {
    testModel.addShape("Steve", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    try {
      testModel.addShape("Steve", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape name already taken", e.getMessage());
    }

    try {
      testModel.addShape("Bill", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, -8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape appearance and/or disappearance times negative", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 8, -9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape appearance and/or disappearance times negative", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 7, 7, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Cannot disappear before appearing", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, -5, 6, 7, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Color values must be in range 0 - 255", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, 5, -6, 7, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Color values must be in range 0 - 255", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, -7, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Color values must be in range 0 - 255", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, 256, 6, 7, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Color values must be in range 0 - 255", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 6, 256, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Color values must be in range 0 - 255", e.getMessage());
    }

    try {
      testModel.addShape("Frank", ShapeTypes.OVAL, 1, 2, 3, 4, 5, 300, 7, 8, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Color values must be in range 0 - 255", e.getMessage());
    }

    try {
      testModel.moveShape("Bill", 5, 6, 7, 8, 9, 10);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape with name \"Bill\" not present", e.getMessage());
    }

    try {
      testModel.scaleShape("Bill", 5, 6, 7, 8, 9, 10);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape with name \"Bill\" not present", e.getMessage());
    }

    try {
      testModel.changeShapeColor("Bill", 5, 6, 7, 8, 9, 10, 11, 12);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Shape with name \"Bill\" not present", e.getMessage());
    }

    try {
      testModel.removeCommand("tim", MotionType.SCALE, 4);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("No such shape", e.getMessage());
    }
  }

}