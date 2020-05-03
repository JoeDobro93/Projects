package cs5004.marblesolitaire.model.hw09;

import org.junit.Before;
import org.junit.Test;

import cs5004.marblesolitaire.model.MarbleSolitaireModel;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Test suite for all methods in TriangleSolitaireModelImpl, including a setup that sets up every
 * type of constructor to prove that no exceptions are thrown.  Also includes some private helper
 * functions to play through a winning game and a losing game..
 */
public class TriangleSolitaireModelImplTest {
  private MarbleSolitaireModel size5board;
  private MarbleSolitaireModel size5board2;
  private MarbleSolitaireModel size6board;
  private MarbleSolitaireModel size9board;
  private MarbleSolitaireModel winningGame;
  private MarbleSolitaireModel dudGame;

  @Before
  public void setUp() throws Exception {
    size5board = new TriangleSolitaireModelImpl(2, 1);
    size5board2 = new TriangleSolitaireModelImpl();
    size6board = new TriangleSolitaireModelImpl(6);
    size9board = new TriangleSolitaireModelImpl(9, 6, 3);

    winningGame = new TriangleSolitaireModelImpl(2, 1);
    dudGame = new TriangleSolitaireModelImpl(2, 1);

    setUpWinningGame();
    setUpDudGame();
  }

  private void setUpWinningGame() {
    winningGame.move(4, 1, 2, 1);
    winningGame.move(4, 3, 4, 1);
    winningGame.move(3, 3, 3, 1);
    winningGame.move(1, 1, 3, 3);
    winningGame.move(1, 0, 3, 2);
    winningGame.move(3, 0, 1, 0);

    winningGame.move(4, 4, 2, 2);
    winningGame.move(2, 2, 4, 2);
    winningGame.move(4, 1, 4, 3);
    winningGame.move(0, 0, 2, 0);
    winningGame.move(2, 0, 4, 2);
    winningGame.move(4, 3, 4, 1);
    winningGame.move(4, 0, 4, 2);
  }

  private void setUpDudGame() {
    dudGame.move(4, 1, 2, 1);
    dudGame.move(4, 3, 4, 1);
    dudGame.move(3, 3, 3, 1);
    dudGame.move(1, 1, 3, 3);
    dudGame.move(1, 0, 3, 2);
    dudGame.move(3, 0, 1, 0);

    dudGame.move(4, 4, 2, 2);
    dudGame.move(2, 2, 4, 2);
    dudGame.move(4, 1, 4, 3);
    dudGame.move(0, 0, 2, 0);
    dudGame.move(2, 0, 4, 2);

    dudGame.move(4, 2, 4, 4);
  }

  @Test
  public void badConstructorTest() {
    MarbleSolitaireModel failMe;
    // test out of bounds coordinates
    try {
      failMe = new TriangleSolitaireModelImpl(-1, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid row", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(5, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid row", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(3, -1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid column", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(3, 4);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid column", e.getMessage());
    }

    // test for invalid size boards
    try {
      failMe = new TriangleSolitaireModelImpl(1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Height must be 2 or greater", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(-6);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Height must be 2 or greater", e.getMessage());
    }

    // test all errors again with 3 argument constructor
    // test out of bounds coordinates
    try {
      failMe = new TriangleSolitaireModelImpl(6, -1, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid row", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(6, 6, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid row", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(6, 3, -1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid column", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(6, 3, 4);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid column", e.getMessage());
    }

    // test for invalid size boards
    try {
      failMe = new TriangleSolitaireModelImpl(1, 2, 2);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Height must be 2 or greater", e.getMessage());
    }

    try {
      failMe = new TriangleSolitaireModelImpl(-6, 2, 2);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Height must be 2 or greater", e.getMessage());
    }
  }

  /**
   * Testing for valid moves.  Uses that getGameState method to help check since there is no other
   * way to directly access the board and see that the moves happened as planned.
   */
  @Test
  public void moveTest() {

    size5board.move(4, 1, 2, 1); //up right
    size5board.move(1, 1, 3, 1); // down left
    size5board.move(3, 3, 1, 1); // up left
    size5board.move(0, 0, 2, 2); // down right
    size5board.move(3, 1, 3, 3); // right
    size5board.move(4, 3, 4, 1); // left
    assertEquals(
            "    _\n" +
                    "   O _\n" +
                    "  O _ O\n" +
                    " O _ _ O\n" +
                    "O O _ _ O", size5board.getGameState());
  }

  /**
   * Tests cases where coordinates are off the board.
   */
  @Test
  public void moveFailTest1() {
    String message = "ONE OR MORE COORDINATE IS OFF THE BOARD";

    try {
      size5board.move(3, 3, -1, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(3, 3, 1, -3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(3, 3, 6, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(3, 3, 1, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(-3, 3, 1, 1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(3, -3, 1, 1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(5, 3, 1, 1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(4, 5, 1, 1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }
  }

  /**
   * Tests cases where either the target it not a space or the source is not a marble.
   */
  @Test
  public void moveFailTest2() {
    String message = "INVALID TO AND/OR FROM SPACE";

    try {
      size5board.move(3, 3, 2, 2);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(2, 1, 2, 1);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(2, 1, 2, 2);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }
  }

  /**
   * Tests cases where the target is not in range of the source.
   */
  @Test
  public void moveFailTest3() {
    String message = "ILLEGAL MOVE";

    try {
      size5board.move(0, 0, 2, 1);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(2, 0, 2, 1);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }

    try {
      size5board.move(4, 4, 2, 1);
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }
  }

  /**
   * Tests where all else is valid, but no marble exists between the source and dest.
   */
  @Test
  public void moveFailTest4() {
    String message = "No marble in between";
    System.out.println(size5board.getGameState());

    try {
      size5board.move(4, 3, 2, 1);
      size5board.move(2, 1, 4, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals(message, e.getMessage());
    }
  }

  @Test
  public void getGameState() {
    assertEquals("    _\n" +
                    "   _ _\n" +
                    "  _ _ _\n" +
                    " _ _ _ _\n" +
                    "_ _ O _ _", winningGame.getGameState());

    assertEquals("    _\n" +
                    "   _ _\n" +
                    "  _ _ _\n" +
                    " _ _ _ _\n" +
                    "O _ _ _ O", dudGame.getGameState());

    assertEquals("    O\n" +
                    "   O O\n" +
                    "  O _ O\n" +
                    " O O O O\n" +
                    "O O O O O", size5board.getGameState());

    assertEquals("    _\n" +
                    "   O O\n" +
                    "  O O O\n" +
                    " O O O O\n" +
                    "O O O O O", size5board2.getGameState());

    assertEquals("     _\n" +
                    "    O O\n" +
                    "   O O O\n" +
                    "  O O O O\n" +
                    " O O O O O\n" +
                    "O O O O O O", size6board.getGameState());

    assertEquals("        O\n" +
                    "       O O\n" +
                    "      O O O\n" +
                    "     O O O O\n" +
                    "    O O O O O\n" +
                    "   O O O O O O\n" +
                    "  O O O _ O O O\n" +
                    " O O O O O O O O\n" +
                    "O O O O O O O O O", size9board.getGameState());
  }

  @Test
  public void isGameOver() {
    assertTrue(winningGame.isGameOver());
    assertFalse(size5board.isGameOver());
    assertFalse(size5board2.isGameOver());
    assertTrue(dudGame.isGameOver());
  }

  @Test
  public void getScoreTest() {
    assertEquals(14, size5board.getScore());
    assertEquals(14, size5board2.getScore());
    assertEquals(20, size6board.getScore());
    assertEquals(44, size9board.getScore());
    assertEquals(1, winningGame.getScore());
    assertEquals(2, dudGame.getScore());
  }
}