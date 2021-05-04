package cs5004.marblesolitaire.model.hw09;

import org.junit.Before;
import org.junit.Test;

import cs5004.marblesolitaire.model.MarbleSolitaireModel;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Test suite for all methods in EuropeanSolitaireModelImpl, including a setup that sets up every
 * type of constructor to prove that no exceptions are thrown.  Also includes some private helper
 * functions to play through a winning game and a losing game..
 */
public class EuropeanSolitaireModelImplTest {
  public MarbleSolitaireModel size3board;
  public MarbleSolitaireModel size3board2;
  public MarbleSolitaireModel size5board;
  public MarbleSolitaireModel size7board;
  public MarbleSolitaireModel winningGame;
  public MarbleSolitaireModel dudGame;

  /**
   * This setup also acts as a test for the constructors, as everything would fail if any of these
   * did not work.  Other tests help verify that they have been initialized as expected, this just
   * helps know that they were able to initialize to something with the given parameters.
   */
  @Before
  public void setUp() {
    size3board = new EuropeanSolitaireModelImpl();
    size3board2 = new EuropeanSolitaireModelImpl(5, 4);
    size5board = new EuropeanSolitaireModelImpl(5);
    size7board = new EuropeanSolitaireModelImpl(7, 3, 9);

    winningGame = new EuropeanSolitaireModelImpl(0, 2);
    dudGame = new EuropeanSolitaireModelImpl(0, 2);
    setUpWinningGame();
    setUpDudGame();
  }

  /**
   * helper function to play all thr moves in a winning game.
   */
  private void setUpWinningGame() {
    winningGame.move(0, 4, 0, 2);
    winningGame.move(2, 3, 0, 3);
    winningGame.move(1, 5, 1, 3);
    winningGame.move(0, 3, 2, 3);
    winningGame.move(3, 4, 1, 4);
    winningGame.move(2, 6, 2, 4);

    winningGame.move(2, 3, 2, 5);
    winningGame.move(4, 6, 2, 6);
    winningGame.move(2, 6, 2, 4);
    winningGame.move(1, 4, 3, 4);
    winningGame.move(3, 4, 3, 6);
    winningGame.move(5, 5, 3, 5);
    winningGame.move(3, 6, 3, 4);
    winningGame.move(2, 1, 2, 3);

    winningGame.move(0, 2, 2, 2);
    winningGame.move(2, 3, 2, 1);
    winningGame.move(4, 2, 2, 2);
    winningGame.move(3, 4, 3, 2);
    winningGame.move(2, 2, 4, 2);
    winningGame.move(3, 0, 3, 2);

    winningGame.move(1, 1, 3, 1);
    winningGame.move(3, 2, 3, 0);
    winningGame.move(4, 3, 4, 5);
    winningGame.move(6, 4, 4, 4);
    winningGame.move(4, 5, 4, 3);
    winningGame.move(5, 2, 3, 2);

    winningGame.move(4, 0, 4, 2);
    winningGame.move(4, 3, 4, 1);
    winningGame.move(2, 0, 4, 0);
    winningGame.move(4, 0, 4, 2);
    winningGame.move(3, 2, 5, 2);
    winningGame.move(6, 3, 4, 3);
    winningGame.move(5, 1, 5, 3);

    winningGame.move(4, 3, 6, 3);
    winningGame.move(6, 2, 6, 4);
  }

  /**
   * Helper function to play moves until there are 5 marbles left and none can move.
   */
  private void setUpDudGame() {
    dudGame.move(0, 4, 0, 2);
    dudGame.move(2, 3, 0, 3);
    dudGame.move(1, 5, 1, 3);
    dudGame.move(0, 3, 2, 3);
    dudGame.move(3, 4, 1, 4);
    dudGame.move(2, 6, 2, 4);

    dudGame.move(2, 3, 2, 5);
    dudGame.move(4, 6, 2, 6);
    dudGame.move(2, 6, 2, 4);
    dudGame.move(1, 4, 3, 4);
    dudGame.move(3, 4, 3, 6);
    dudGame.move(5, 5, 3, 5);
    dudGame.move(3, 6, 3, 4);
    dudGame.move(2, 1, 2, 3);

    dudGame.move(0, 2, 2, 2);
    dudGame.move(2, 3, 2, 1);
    dudGame.move(4, 2, 2, 2);
    dudGame.move(3, 4, 3, 2);
    dudGame.move(2, 2, 4, 2);
    dudGame.move(3, 0, 3, 2);

    dudGame.move(1, 1, 3, 1);
    dudGame.move(3, 2, 3, 0);
    dudGame.move(4, 3, 4, 5);
    dudGame.move(6, 4, 4, 4);
    dudGame.move(4, 5, 4, 3);
    dudGame.move(5, 2, 3, 2);

    dudGame.move(4, 0, 4, 2);
    dudGame.move(4, 3, 4, 1);
    dudGame.move(2, 0, 4, 0);
    dudGame.move(4, 0, 4, 2);
    dudGame.move(4, 2, 2, 2);
    dudGame.move(6, 3, 4, 3);
  }

  @Test
  public void badConstructorTest() {
    MarbleSolitaireModel failMe;
    // test for out of bounds coordinates
    try {
      failMe = new EuropeanSolitaireModelImpl(7, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("row 7, col 3 is out of bounds.", e.getMessage());
    }

    try {
      failMe = new EuropeanSolitaireModelImpl(-5, 3);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("row -5, col 3 is out of bounds.", e.getMessage());
    }

    try {
      failMe = new EuropeanSolitaireModelImpl(3, 9);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("row 3, col 9 is out of bounds.", e.getMessage());
    }

    try {
      failMe = new EuropeanSolitaireModelImpl(5, -1);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("row 5, col -1 is out of bounds.", e.getMessage());
    }

    // test for in bounds coordinates that are not in playing area.
    try {
      failMe = new EuropeanSolitaireModelImpl(3, 0, 0);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid space row 0, col 0", e.getMessage());
    }

    try {
      failMe = new EuropeanSolitaireModelImpl(3, 1, 0);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("Invalid space row 1, col 0", e.getMessage());
    }

    // test for invalid size boards
    try {
      failMe = new EuropeanSolitaireModelImpl(-5);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("ARM THICKNESS MUST BE AT LEAST 3", e.getMessage());
    }

    try {
      failMe = new EuropeanSolitaireModelImpl(-2);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("ARM THICKNESS MUST BE AT LEAST 3", e.getMessage());
    }

    try {
      failMe = new EuropeanSolitaireModelImpl(2);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("ARM THICKNESS MUST BE AT LEAST 3", e.getMessage());
    }

    try {
      failMe = new EuropeanSolitaireModelImpl(6);
      fail();
    } catch (Exception e) {
      assertEquals(IllegalArgumentException.class, e.getClass());
      assertEquals("ARM THICKNESS MUST BE ODD", e.getMessage());
    }

  }

  /**
   * Testing for valid moves.  Uses that getGameState method to help check since there is no other
   * way to directly access the board and see that the moves happened as planned.
   */
  @Test
  public void moveTest() {

    size3board.move(3, 1, 3, 3);
    size3board.move(1, 2, 3, 2);
    size3board.move(4, 2, 2, 2);
    assertEquals("    O O O\n" +
            "  O _ O O O\n" +
            "O O O O O O O\n" +
            "O _ _ O O O O\n" +
            "O O _ O O O O\n" +
            "  O O O O O\n" +
            "    O O O", size3board.getGameState());

    size3board2.move(5, 2, 5, 4);
    assertEquals("    O O O\n" +
            "  O O O O O\n" +
            "O O O O O O O\n" +
            "O O O O O O O\n" +
            "O O O O O O O\n" +
            "  O _ _ O O\n" +
            "    O O O", size3board2.getGameState());


  }

  /**
   * Tests cases where coordinates are off the board.
   */
  @Test
  public void moveFailTest1() {
    String message = "ONE OR MORE COORDINATE IS OFF THE BOARD";
    // Coordinate is not on the board
    try {
      size3board.move(-1, 1, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, -4, 3, 2);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 1, -3, 2);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }
    try {
      size3board.move(3, 1, 3, -2);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }
    try {
      size3board.move(7, 1, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 8, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 1, 9, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 1, 3, 100);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }
  }

  /**
   * Tests cases where the coordinates are both on the board, but the source is not a MARBLE and/or
   * the destination is not EMPTY.
   */
  @Test
  public void moveFailTest2() {
    String message = "INVALID TO AND/OR FROM SPACE";
    try {
      size3board.move(3, 3, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 3, 2, 2);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }
  }

  /**
   * Test for cases where the source and destination are both on the board, the source is a MARBLE
   * and the destination is EMPTY, but they are in line with each other.
   */
  @Test
  public void moveFailTest3() {
    String message = "SOURCE AND DESTINATION CANNOT BE DIAGONAL";

    try {
      size3board.move(4, 2, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(4, 4, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(2, 2, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(2, 4, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

  }

  /**
   * Test for cases where the source and destination are both on the board, the source is a MARBLE
   * and the destination is EMPTY, they are in line, but are not exactly 2 spaces away from each
   * other.
   */
  @Test
  public void moveFailTest4() {
    String message = "SOURCE MUST BE 2 SPACES AWAY FROM DESTINATION";

    try {
      size3board.move(3, 0, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 2, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 4, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(3, 6, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(0, 3, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(2, 3, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(4, 3, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

    try {
      size3board.move(6, 3, 3, 3);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }

  }

  /**
   * Test for cases where the source and destination are both on the board, the source is a MARBLE
   * and the destination is EMPTY, they are in line, they are exactly 2 spaces apart, but there is
   * not another marble in between them.
   */
  @Test
  public void moveFailTest5() {
    size3board.move(3, 1, 3, 3);
    String message = "MARBLE MUST BE IN BETWEEN SOURCE AND DESTINATION";

    try {
      size3board.move(3, 3, 3, 1);
      fail();
    } catch (Exception e) {
      assertEquals(message, e.getMessage());
      assertEquals(IllegalArgumentException.class, e.getClass());
    }
  }

  @Test
  public void isGameOver() {
    assertTrue(winningGame.isGameOver());
    assertFalse(size3board.isGameOver());
    assertTrue(dudGame.isGameOver());
  }

  @Test
  public void getGameState() {
    assertEquals("    _ _ _\n" +
            "  _ _ _ _ _\n" +
            "_ _ _ _ _ _ _\n" +
            "_ _ _ _ _ _ _\n" +
            "_ _ _ _ _ _ _\n" +
            "  _ _ _ _ _\n" +
            "    _ _ O", winningGame.getGameState());

    assertEquals("    _ _ _\n" +
            "  _ _ _ _ _\n" +
            "_ _ O _ _ _ _\n" +
            "_ _ _ _ _ _ _\n" +
            "_ _ _ O _ _ _\n" +
            "  O _ _ _ _\n" +
            "    O _ _", dudGame.getGameState());

    assertEquals("    O O O\n" +
            "  O O O O O\n" +
            "O O O O O O O\n" +
            "O O O O O O O\n" +
            "O O O O O O O\n" +
            "  O O O _ O\n" +
            "    O O O", size3board2.getGameState());

    // test after a move
    size3board2.move(5, 2, 5, 4);
    assertEquals("    O O O\n" +
            "  O O O O O\n" +
            "O O O O O O O\n" +
            "O O O O O O O\n" +
            "O O O O O O O\n" +
            "  O _ _ O O\n" +
            "    O O O", size3board2.getGameState());

    assertEquals("        O O O O O\n" +
            "      O O O O O O O\n" +
            "    O O O O O O O O O\n" +
            "  O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O\n" +
            "O O O O O O _ O O O O O O\n" +
            "O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O\n" +
            "  O O O O O O O O O O O\n" +
            "    O O O O O O O O O\n" +
            "      O O O O O O O\n" +
            "        O O O O O", size5board.getGameState());

    assertEquals("            O O O O O O O\n" +
            "          O O O O O O O O O\n" +
            "        O O O O O O O O O O O\n" +
            "      O O O O O O _ O O O O O O\n" +
            "    O O O O O O O O O O O O O O O\n" +
            "  O O O O O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O O O O O O O\n" +
            "O O O O O O O O O O O O O O O O O O O\n" +
            "  O O O O O O O O O O O O O O O O O\n" +
            "    O O O O O O O O O O O O O O O\n" +
            "      O O O O O O O O O O O O O\n" +
            "        O O O O O O O O O O O\n" +
            "          O O O O O O O O O\n" +
            "            O O O O O O O", size7board.getGameState());
  }

  @Test
  public void getScore() {
    assertEquals(36, size3board.getScore());
    assertEquals(size3board.getScore(), size3board2.getScore());
    assertEquals(128, size5board.getScore());
    assertEquals(276, size7board.getScore());
    assertEquals(1, winningGame.getScore());
    assertEquals(4, dudGame.getScore());
  }
}