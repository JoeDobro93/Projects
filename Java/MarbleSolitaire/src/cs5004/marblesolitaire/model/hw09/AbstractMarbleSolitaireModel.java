package cs5004.marblesolitaire.model.hw09;

import cs5004.marblesolitaire.model.MarbleSolitaireModel;
import cs5004.marblesolitaire.model.boardSpace;

/**
 * Contains elements of a Marble Solitaire board that are shared by many or all board types.
 * Methods that are a little different may be overridden in their concrete classes.
 */
public class AbstractMarbleSolitaireModel implements MarbleSolitaireModel {
  protected boardSpace[][] board;
  protected int score;

  /**
   * Initializes the gameboard.
   *
   * @param armThickness integer thickness of the arms
   */
  protected void initializeBoard(int armThickness, int row, int col)
          throws IllegalArgumentException {
    /* To be implemented individually for each type of game.  Must use the boardSpace array*/
  }

  protected void symmetricalAssign(int row, int col, int size, boardSpace type) {
    board[row][col] = type;
    board[size - 1 - row][col] = type;
    board[row][size - 1 - col] = type;
    board[size - 1 - row][size - 1 - col] = type;
  }

  @Override
  public void move(int fromRow, int fromCol, int toRow, int toCol) throws IllegalArgumentException {
    checkValidMove(fromRow, fromCol, toRow, toCol);
    board[fromRow][fromCol] = boardSpace.EMPTY;
    board[toRow][toCol] = boardSpace.MARBLE;
    board[(fromRow + toRow) / 2][(fromCol + toCol) / 2] = boardSpace.EMPTY;
    // every successful move removes a piece from the board.
    score--;
  }

  /**
   * Helper function to throw exceptions with unique messages for each invalid move.  No exceptions
   * are thrown if the move is valid.
   *
   * @param fromRow int starting row coordinate.
   * @param fromCol int starting column coordinate.
   * @param toRow   int destination row coordinate.
   * @param toCol   int destination column coordinate.
   */
  protected void checkValidMove(int fromRow, int fromCol, int toRow, int toCol)
          throws IllegalArgumentException {
    if (fromRow >= board.length || fromCol >= board.length
            || toRow >= board.length || toCol >= board.length
            || fromRow < 0 || fromCol < 0
            || toRow < 0 || toCol < 0) {
      // case where any coordinate is not in the board range
      throw new IllegalArgumentException("ONE OR MORE COORDINATE IS OFF THE BOARD");
    } else if (board[fromRow][fromCol] != boardSpace.MARBLE
            || board[toRow][toCol] != boardSpace.EMPTY) {
      // case where from is EMPTY or destination is MARBLE
      throw new IllegalArgumentException("INVALID TO AND/OR FROM SPACE");
    } else if (fromRow != toRow && fromCol != toCol) {
      // case where destination is diagonal from target
      throw new IllegalArgumentException("SOURCE AND DESTINATION CANNOT BE DIAGONAL");
    } else if (Math.sqrt(Math.pow(fromRow - toRow, 2) + Math.pow(fromCol - toCol, 2)) != 2) {
      // case where they are in line, but too close or too far.
      throw new IllegalArgumentException("SOURCE MUST BE 2 SPACES AWAY FROM DESTINATION");
    } else if (board[(fromRow + toRow) / 2][(fromCol + toCol) / 2] != boardSpace.MARBLE) {
      // case where midpoint is not a MARBLE
      throw new IllegalArgumentException("MARBLE MUST BE IN BETWEEN SOURCE AND DESTINATION");
    }
  }

  @Override
  public boolean isGameOver() {
    if (this.score == 1) {
      return true;
    } else if (!movesRemain()) {
      return true;
    }
    return false;
  }

  /**
   * Checks every position on board to see if it is a MARBLE and if so if it can move.  If you are
   * reading this, it means I ran out of time to reimplement this with a list of coordinates to keep
   * track of where all the MARBLEs are and only cycle through those.  For now, I understand that
   * this an inefficient solution, checking even spaces that are not in the playing area.
   *
   * @return true if there are moves remaining, false otherwise.
   */
  protected boolean movesRemain() {
    for (int x = 0; x < board.length; x++) {
      for (int y = 0; y < board[0].length; y++) {
        if (board[x][y] == boardSpace.MARBLE) {
          if (canMove(x, y)) {
            System.out.println("TRUE SO BREAK!!!");
            return true;
          }
        }
      }
    }
    return false;
  }

  /**
   * Private helper function to check for valid moves. From a given coordinate (assumed to be a
   * MARBLE but will work otherwise), it checks if the move is valid in each cardinal direction.  If
   * any work, true is returned.
   *
   * @param row Starting position row.
   * @param col Starting position column
   * @return true if valid move exists, false otherwise.
   */
  protected boolean canMove(int row, int col) {
    try {
      checkValidMove(row, col, row + 2, col);
      return true;
    } catch (Exception e) {
      try {
        checkValidMove(row, col, row - 2, col);
        return true;
      } catch (Exception e2) {
        try {
          checkValidMove(row, col, row, col + 2);
          return true;
        } catch (Exception e3) {
          try {
            checkValidMove(row, col, row, col - 2);
            return true;
          } catch (Exception e4) {
            return false;
          }
        }
      }
    }
  }

  @Override
  public String getGameState() {
    String boardRep = "";
    int size = board.length;
    int length;
    boolean trailSpaceCheck = false;
    for (int x = 0; x < size; x++) {
      for (int y = 0; y < size; y++) {
        boardRep += board[x][y];
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

  @Override
  public int getScore() {
    return this.score;
  }
}
