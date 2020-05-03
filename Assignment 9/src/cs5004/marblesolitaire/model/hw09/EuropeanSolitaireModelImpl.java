package cs5004.marblesolitaire.model.hw09;

import cs5004.marblesolitaire.model.boardSpace;

/**
 * Implements methods that are specific to the European Marble Solitaire board.
 */
public class EuropeanSolitaireModelImpl extends AbstractMarbleSolitaireModel {

  public EuropeanSolitaireModelImpl() {
    initializeBoard(3, 3, 3);
  }

  /**
   * Marble Model constructor that takes two arguments, the column and row position for the starting
   * space.
   *
   * @throws IllegalArgumentException if the row and column coordinates are not in the playing
   *                                  area.
   */
  public EuropeanSolitaireModelImpl(int row, int col) throws IllegalArgumentException {
    initializeBoard(3, row, col);
  }

  /**
   * Marble Model constructor that takes one argument, the thickness of each arm in the game.
   *
   * @throws IllegalArgumentException if size is less than 3 or is an even number.
   */
  public EuropeanSolitaireModelImpl(int armThickness) throws IllegalArgumentException {
    // I came to this by seeing the center is half of the (arm width - 1) plus the length of the
    // blank space plus one.  The blank space is thickness - 1, so this could be rewritten as
    // 2((thickness - 1)/2).  Thus, the distance to center is 3((thickness - 1)/2).  3/2 = 1.5.
    int center = (int) ((armThickness - 1) * 1.5);
    initializeBoard(armThickness, center, center);
  }

  /**
   * Marble Model constructor that takes three arguments, the column and row position for starting
   * space and the arm thickness.
   *
   * @throws IllegalArgumentException if the row and column coordinates are not in the playing area
   *                                  or if size is less than 3 or is an even number.
   */
  public EuropeanSolitaireModelImpl(int armThickness, int row, int col)
          throws IllegalArgumentException {
    initializeBoard(armThickness, row, col);
  }

  /**
   * Initializes the gameboard based on the arm thickness as well as the coordinates for the empty
   * space.  I opted to initialize this using three sets of for loops, rather than a single for loop
   * with conditionals.  This way, each space on the board is looked at exactly once and does not
   * need extra code to check conditionals, optimizing bigO complexity by working with smaller
   * chunks.  Additionally, I set up these chunks so that they could assign board spaces based on
   * symmetry, further reducing the number of loops.
   *
   * @param armThickness integer thickness of the arms
   */
  @Override
  protected void initializeBoard(int armThickness, int row, int col)
          throws IllegalArgumentException {
    if (armThickness < 3) {
      throw new IllegalArgumentException("ARM THICKNESS MUST BE AT LEAST 3");
    } else if (armThickness % 2 == 0) {
      throw new IllegalArgumentException("ARM THICKNESS MUST BE ODD");
    }
    int loopHelper;
    int size = armThickness * 3 - 2;
    int midpoint = (size - 1) / 2;

    // starts at -1 to offset that one space will be initialized to EMPTY.
    this.score = 0;
    board = new boardSpace[size][size];

    // initialize all blank spaces.  Assigns all four corners simultaneously.
    loopHelper = armThickness - 1;
    for (int y = 0; y < armThickness - 1; y++) {
      for (int x = 0; x < loopHelper; x++) {
        symmetricalAssign(y, x, size, boardSpace.BLANK);
      }
      loopHelper--;
    }

    // fill in the center row/column
    for (int i = 0; i < midpoint; i++) {
      board[i][midpoint] = boardSpace.MARBLE;
      board[size - 1 - i][midpoint] = boardSpace.MARBLE;
      board[midpoint][i] = boardSpace.MARBLE;
      board[midpoint][size - 1 - i] = boardSpace.MARBLE;
      score += 4;
    }

    // fill in the quadrants.  May come back with more efficient algorithm that doesn't need to
    // check if a space is blank.
    for (int y = 0; y < midpoint; y++) {
      for (int x = 0; x < midpoint; x++) {
        if (board[y][x] == null) {
          symmetricalAssign(y, x, size, boardSpace.MARBLE);
          score += 4;
        }
      }
    }

    //lastly fill the center
    board[midpoint][midpoint] = boardSpace.MARBLE;

    // and now we can add the starting place.  If it can't change state, it is invalid and will
    // throw an exception.
    try {
      board[row][col] = board[row][col].changeState();
    } catch (IllegalArgumentException e) {
      throw new IllegalArgumentException(e.getMessage() + String
              .format(" row %s, col %s", row, col));
    } catch (ArrayIndexOutOfBoundsException e) {
      throw new IllegalArgumentException(String.format("row %s, col %s is out of bounds.",
              row, col));
    }
  }
}
