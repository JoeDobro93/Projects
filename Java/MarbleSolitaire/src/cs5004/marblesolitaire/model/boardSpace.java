package cs5004.marblesolitaire.model;

/**
 * Enum to represent spaces on the board.  MARBLE('X') represents a mable, EMPTY('_') represent a
 * valid move space that is not occupied, BLANK(' ') represents spaces outside of the playing area.
 */
public enum boardSpace {
  MARBLE("O "),
  EMPTY("_ "),
  BLANK("  ");

  private final String txt;

  boardSpace(String txt) {
    this.txt = txt;
  }

  /**
   * Allows MARBLE and EMPTY spaces to change states.  If written properly, the final else should
   * not be reached, but just in case it will simply return itself.
   *
   * @return The new state.
   */
  public boardSpace changeState() {
    if (this.txt == MARBLE.txt) {
      return boardSpace.EMPTY;
    } else if (this.txt == EMPTY.txt) {
      return boardSpace.MARBLE;
    } else {
      throw new IllegalArgumentException("Invalid space");
    }
  }

  @Override
  public String toString() {
    return this.txt;
  }
}
