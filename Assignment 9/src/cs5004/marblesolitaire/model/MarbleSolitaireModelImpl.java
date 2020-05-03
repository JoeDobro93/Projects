package cs5004.marblesolitaire.model;

import cs5004.marblesolitaire.model.hw09.AbstractMarbleSolitaireModel;

/**
 * Implements MarbleSolitaireModel to initialize a board of Marble Solitaire as well as related
 * methods to play a game.  board is an array of boardSpaces which represent the positions of each
 * space on the board plus a score integer that keeps track of how many pieces are left on the
 * board.
 */
public class MarbleSolitaireModelImpl extends AbstractMarbleSolitaireModel {

  public MarbleSolitaireModelImpl() {
    super();
    initializeBoard(3, 3, 3);
  }

  /**
   * Marble Model constructor that takes two arguments, the column and row position for the starting
   * space.
   *
   * @throws IllegalArgumentException if the row and column coordinates are not in the playing
   *                                  area.
   */
  public MarbleSolitaireModelImpl(int row, int col) throws IllegalArgumentException {
    initializeBoard(3, row, col);
  }

  /**
   * Marble Model constructor that takes one argument, the thickness of each arm in the game.
   *
   * @throws IllegalArgumentException if size is less than 3 or is an even number.
   */
  public MarbleSolitaireModelImpl(int armThickness) throws IllegalArgumentException {
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
  public MarbleSolitaireModelImpl(int armThickness, int row, int col)
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

    int size = armThickness * 3 - 2;
    // starts at -1 to offset that one space will be initialized to EMPTY.
    this.score = -1;
    board = new boardSpace[size][size];

    // initialize all blank spaces.  Assigns all four corners simultaneously.
    for (int x = 0; x < armThickness - 1; x++) {
      for (int y = 0; y < armThickness - 1; y++) {
        board[y][x] = boardSpace.BLANK;
        board[size - 1 - y][x] = boardSpace.BLANK;
        board[y][size - 1 - x] = boardSpace.BLANK;
        board[size - 1 - y][size - 1 - x] = boardSpace.BLANK;
      }
    }
    // assigns the "wings"
    for (int x = 0; x < armThickness - 1; x++) {
      for (int y = armThickness - 1; y < armThickness * 2 - 1; y++) {
        board[y][x] = boardSpace.MARBLE;
        board[y][size - 1 - x] = boardSpace.MARBLE;
        score += 2;
      }
    }
    // assigns the remaining marbles.
    for (int x = armThickness - 1; x < armThickness * 2 - 1; x++) {
      for (int y = 0; y < size; y++) {
        board[y][x] = boardSpace.MARBLE;
        score++;
      }
    }

    // At end after board is geneterated.
    if (board.length <= row || board.length <= col || row < 0 || col < 0) {
      throw new IllegalArgumentException("INVALID COORDINATE");
    } else if (board[row][col] == boardSpace.MARBLE) {
      board[row][col] = boardSpace.EMPTY;
    } else {
      throw new IllegalArgumentException("SPACE NOT IN PLAYING AREA");
    }
  }
}
