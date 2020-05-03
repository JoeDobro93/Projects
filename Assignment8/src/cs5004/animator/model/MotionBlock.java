package cs5004.animator.model;

/**
 * Helper class to store move parameters.  Data in "params" is specific to the type of motion being
 * stored.
 */
public class MotionBlock {
  private MotionType type;
  private int[] times;
  private int[] startParams;
  private int[] endParams;

  protected MotionBlock(MotionType type, int[] times, int[] startParams, int[] endParams)
          throws IllegalArgumentException {
    // verify valid times
    if (times[0] < 0) {
      throw new IllegalArgumentException("Start time must be 0 or greater");
    } else if (times[1] <= times[0]) {
      // this also ensures end time also is not negative.
      throw new IllegalArgumentException("Duration must be 1 or higher");
    }
    // verify equal number of start and end params
    if (startParams.length != endParams.length) {
      throw new IllegalArgumentException("Invalid number of params");
    }

    // verify number of commands is the expected for that type.
    switch (type) {
      case MOVE:
      case SCALE:
        if (startParams.length != 2) {
          throw new IllegalArgumentException("Invalid number of params");
        }
        break;
      case RECOLOR:
        if (startParams.length != 3) {
          throw new IllegalArgumentException("Invalid number of params");
        }
        break;
      default:
        throw new IllegalArgumentException("Unhandled motion type");
    }


    this.type = type;
    this.times = times;
    this.startParams = startParams;
    this.endParams = endParams;
  }

  public MotionType getType() {
    return type;
  }

  public int[] getTimes() {
    return times;
  }

  public int[] getStartParams() {
    return startParams;
  }

  public int[] getEndParams() {
    return endParams;
  }

  @Override
  public String toString() throws IllegalArgumentException {
    String s = "";
    switch (type) {
      case MOVE:
        s = String.format("%s from (%d,%d) to (%d,%d) from t=%d to t=%d",
                type.toString(), startParams[0], startParams[1],
                endParams[0], endParams[1], times[0], times[1]);
        break;
      case SCALE:
        s = String.format("%s from x size %d, y size %d to x size %d, y size %d from t=%d to t=%d",
                type.toString(), startParams[0], startParams[1],
                endParams[0], endParams[1], times[0], times[1]);
        break;
      case RECOLOR:
        s = String.format("%s from (r=%d,g=%d,b=%d) to (r=%d,g=%d,b=%d) from t=%d to t=%d",
                type.toString(), startParams[0], startParams[1], startParams[2],
                endParams[0], endParams[1], endParams[2], times[0], times[1]);
        break;
      default:
        throw new IllegalArgumentException(String.format("Type \"%s\" not implemented",
                type.toString()));
    }
    return s;
  }
}
