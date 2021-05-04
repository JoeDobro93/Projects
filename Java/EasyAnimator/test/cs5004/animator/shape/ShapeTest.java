package cs5004.animator.shape;

import org.junit.Before;
import org.junit.Test;

import java.awt.Color;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

/**
 * Test to make sure shapes can be created and modified with expected results or exceptions.
 */
public class ShapeTest {
  private Shape circle1;
  private Shape circle2;
  private Shape circle3;
  private Shape rect1;
  private Shape rect2;
  private Shape rect3;
  private Shape rect4;

  @Before
  public void setUp() throws Exception {
    circle1 = new Oval(30, 40, 10, 20, new Color(0, 100, 200));
    circle2 = new Oval(-30, 40, 2, 10, new Color(100, 200, 0));
    circle3 = new Oval(30, -40, 100, 2, new Color(200, 0, 100));
    rect1 = new Rectangle(20, 50, 10, 20, new Color(0, 100, 200));
    rect2 = new Rectangle(-20, 50, 2, 10, new Color(100, 200, 0));
    rect3 = new Rectangle(20, -50, 100, 2, new Color(200, 0, 100));
    rect4 = new Rectangle(-20, -50, 20, 72, new Color(50, 150, 255));
  }

  @Test
  public void testGetters() {
    // test get reference
    assertEquals(30, circle1.getReference().getX(), .0001);
    assertEquals(-30, circle2.getReference().getX(), .0001);
    assertEquals(20, rect1.getReference().getX(), .0001);
    assertEquals(-20, rect2.getReference().getX(), .0001);
    assertEquals(40, circle1.getReference().getY(), .0001);
    assertEquals(-40, circle3.getReference().getY(), .0001);
    assertEquals(50, rect1.getReference().getY(), .0001);
    assertEquals(-50, rect3.getReference().getY(), .0001);
    // test get size
    assertEquals(10, circle1.getSizeX(), .0001);
    assertEquals(2, rect2.getSizeX(), .0001);
    assertEquals(2, circle3.getSizeY(), .0001);
    assertEquals(72, rect4.getSizeY(), .0001);
    // test getcolor
    Color color1 = new Color(0, 100, 200);
    Color color2 = new Color(100, 200, 0);
    Color color3 = new Color(200, 0, 100);
    Color color4 = new Color(50, 150, 255);
    assertEquals(color1.toString(), circle1.getColor().toString());
    assertEquals(color2.toString(), rect2.getColor().toString());
    assertEquals(color3.toString(), circle3.getColor().toString());
    assertEquals(color4.toString(), rect4.getColor().toString());
  }

  @Test
  public void testMove() {
    circle1.move(3, 5);
    rect1.move(5, 8);
    assertEquals(3, circle1.getReference().getX(), .0001);
    assertEquals(5, circle1.getReference().getY(), .0001);
    assertEquals(5, rect1.getReference().getX(), .0001);
    assertEquals(8, rect1.getReference().getY(), .0001);

    circle1.move(-3, 5);
    rect1.move(5, -8);
    assertEquals(-3, circle1.getReference().getX(), .0001);
    assertEquals(5, circle1.getReference().getY(), .0001);
    assertEquals(5, rect1.getReference().getX(), .0001);
    assertEquals(-8, rect1.getReference().getY(), .0001);
  }

  @Test
  public void testScale() {
    circle1.scale(4, 7);
    rect1.scale(1, 45);
    assertEquals(4, circle1.getSizeX(), .0001);
    assertEquals(7, circle1.getSizeY(), .0001);
    assertEquals(1, rect1.getSizeX(), .0001);
    assertEquals(45, rect1.getSizeY(), .0001);

    try {
      circle1.scale(0, 1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Diameter must be 1 or more", e.getMessage());
    }
    try {
      rect1.scale(0, 1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Size must be non-zero and positive", e.getMessage());
    }

    try {
      circle1.scale(4, -1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Diameter must be 1 or more", e.getMessage());
    }
    try {
      rect1.scale(-3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Size must be non-zero and positive", e.getMessage());
    }
  }

  @Test
  public void changeColorTest() {
    Color color1 = new Color(100, 30, 67);
    Color color2 = new Color(43, 45, 34);

    circle1.changeColor(color1);
    rect1.changeColor(color2);
    assertEquals(color1, circle1.getColor());
    assertEquals(color2, rect1.getColor());
  }
}