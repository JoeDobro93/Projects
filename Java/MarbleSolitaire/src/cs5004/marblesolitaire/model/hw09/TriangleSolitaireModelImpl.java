package cs5004.marblesolitaire.model.hw09;

import cs5004.marblesolitaire.model.boardSpace;

/**
 * Implements methods that are specific to the Triangular Marble Solitaire board.
 */
public class TriangleSolitaireModelImpl extends AbstractMarbleSolitaireModel {
  /**
   * Marble Model constructor that takes no arguments.  Defaults size of 5
   */
  public TriangleSolitaireModelImpl() {
    initializeBoard(5, 0, 0);
  }

  /**
   * Marble Model constructor that takes two arguments, the column and row position for the starting
   * space.
   *
   * @throws IllegalArgumentException if the row and column coordinates are not in the playing
   *                                  area.
   */
  public TriangleSolitaireModelImpl(int row, int col) throws IllegalArgumentException {
    initializeBoard(5, row, col);
  }

  /**
   * Marble Model constructor that takes one argument, the thickness of each arm in the game.
   *
   * @throws IllegalArgumentException if size is less than 3 or is an even number.
   */
  public TriangleSolitaireModelImpl(int height) throws IllegalArgumentException {
    this.initializeBoard(height, 0, 0);
  }

  /**
   * Marble Model constructor that takes three arguments, the column and row position for starting
   * space and the arm thickness.
   *
   * @throws IllegalArgumentException if the row and column coordinates are not in the playing area
   *                                  or if size is less than 3 or is an even number.
   */
  public TriangleSolitaireModelImpl(int height, int row, int col)
          throws IllegalArgumentException {
    initializeBoard(height, row, col);
  }

  /**
   * Initializes the gameboard.
   *
   * @param height integer number of rows
   */
  @Override
  protected void initializeBoard(int height, int row, int col)
          throws IllegalArgumentException {
    score = -1;
    if (height < 2) {
      throw new IllegalArgumentException("Height must be 2 or greater");
    }

    if (row >= height || row < 0) {
      throw new IllegalArgumentException("Invalid row");
    } else if (col > row || col < 0) {
      throw new IllegalArgumentException("Invalid column");
    }

    board = new boardSpace[height][height * 2 - 1];
    int x; // row on board for populating board
    int y; // column on board for populating board


    for (y = 0; y < height; y++) {
      // this takes care of the spaces inside
      x = 0;
      for (int z = height - 1 - y; z > 0; z--) {
        symmetricalAssign(y, x, board[0].length - 1 - x, boardSpace.BLANK);
        x++;
      }

      // this fills in the rest of the marbles
      x = height - 2;
      for (int z = 0; z < y; z++) {
        // kind of proud of finding this, used what I learned in Discreet with truth tables
        if ((y % 2 == x % 2 && height % 2 == 1) || (y % 2 != x % 2 && height % 2 == 0)) {
          symmetricalAssign(y, x, board[0].length - 1 - x, boardSpace.MARBLE);
          score += 2;
        } else {
          symmetricalAssign(y, x, board[0].length - 1 - x, boardSpace.BLANK);
        }
        x--;
      } // end fill in rest of marble loop


      // this takes care of the center line
      if (y % 2 == 0) {
        board[y][height - 1] = boardSpace.MARBLE;
        score++;
      } else {
        board[y][height - 1] = boardSpace.BLANK;
      }
    } // end populating grid

    board[row][columnConverter(col, row)] = boardSpace.EMPTY;

  }

  @Override
  public void move(int fromRow, int fromCol, int toRow, int toCol) throws IllegalArgumentException {
    // almost the same as before, but first must convert the coordinate on the triangle to the array
    // coordinate.
    super.move(fromRow, columnConverter(fromCol, fromRow), toRow, columnConverter(toCol, toRow));
  }

  @Override
  protected void checkValidMove(int fromRow, int fromCol, int toRow, int toCol)
          throws IllegalArgumentException {
    // converting back to triangle coordinate because most of these checkers rely on that, other
    // than the test that checks if it's a marble or not.  Opted to deconvert here, despite being
    // immidiately after it's sent to the super.move method because otherwise I would need to
    // convert within the move method, causing lots of code duplication.
    int fromColTri = columnDeconverter(fromCol, fromRow);
    int toColTri = columnDeconverter(toCol, toRow);

    if (fromRow >= board.length || toRow >= board.length
            || fromColTri > fromRow || toColTri > toRow
            || fromColTri < 0 || toColTri < 0) {
      // case where any coordinate is not in the board range
      throw new IllegalArgumentException("ONE OR MORE COORDINATE IS OFF THE BOARD");
    } else if (board[fromRow][fromCol] != boardSpace.MARBLE
            || board[toRow][toCol] != boardSpace.EMPTY) {
      // case where from is EMPTY or destination is MARBLE
      throw new IllegalArgumentException("INVALID TO AND/OR FROM SPACE");
    } else if (!((fromRow == toRow && (toColTri == fromColTri + 2 || toColTri == fromColTri - 2))
            || (toRow == (fromRow + 2) && (toColTri == fromColTri || toColTri == fromColTri + 2))
            || (toRow == (fromRow - 2) && (toColTri == fromColTri || toColTri == fromColTri - 2)))
    ) {
      // ensures that the move is legal
      throw new IllegalArgumentException("ILLEGAL MOVE");
    } else if (board[(fromRow + toRow) / 2][(fromCol + toCol) / 2] != boardSpace.MARBLE) {
      throw new IllegalArgumentException("No marble in between");
    }
  }

  @Override
  protected void symmetricalAssign(int row, int col, int flip, boardSpace type) {
    board[row][col] = type;
    board[row][flip] = type;
  }

  /**
   * Converts the coordinates to the actual position in the array.
   *
   * @param x int x column number in triangle
   * @param y int y row number
   * @return x int column number on grid
   */
  private int columnConverter(int x, int y) {
    // number of gaps in a row plus 2 (since we skip each other space) times the initial x.
    x = (board.length - 1 - y) + 2 * x;
    return x;
  }

  /**
   * Converts the array column position into the triangle array position.
   *
   * @param x int x column number in array
   * @param y int y row number
   * @return x int column number on triangle
   */
  private int columnDeconverter(int x, int y) {
    x = (x - board.length + 1 + y) / 2;
    return x;
  }

  @Override
  protected boolean canMove(int row, int col) {
    try {
      checkValidMove(row, col, row, col + 4);
      return true;
    } catch (Exception e) {
      try {
        checkValidMove(row, col, row, col - 4);
        return true;
      } catch (Exception e2) {
        try {
          checkValidMove(row, col, row + 2, col + 2);
          return true;
        } catch (Exception e3) {
          try {
            checkValidMove(row, col, row + 2, col - 2);
            return true;
          } catch (Exception e4) {
            try {
              checkValidMove(row, col, row - 2, col + 2);
              return true;
            } catch (Exception e5) {
              try {
                checkValidMove(row, col, row - 2, col - 2);
                return true;
              } catch (Exception e6) {
                return false;
              }
            }
          }
        }
      }
    }
  }

  @Override
  public String getGameState() {
    String boardRep = "";
    int length;
    for (int x = 0; x < board.length; x++) {
      for (int y = 0; y < board[0].length; y++) {
        boardRep += board[x][y].toString().charAt(0);
      }
      // after each line, goes backwards until something not blank occurs.
      // This new, reduced length is used to make a substring that doesn't have the spaces.
      for (length = boardRep.length(); length > 0; length--) {
        if (!Character.isWhitespace(boardRep.charAt(length - 1))) {
          break;
        }
      }
      boardRep = boardRep.substring(0, length) + "\n";
    }
    // I opted to do this after, rather than check each loop if we were on the last row.
    // The reason is it seems inefficient if it has to check every loop when I know it will either
    // end with /n or be blank.
    if (!boardRep.equals("")) {
      boardRep = boardRep.substring(0, boardRep.length() - 1);
    }
    return boardRep;
  }
}
