import cs5004.marblesolitaire.model.MarbleSolitaireModel;
import cs5004.marblesolitaire.model.MarbleSolitaireModelImpl;
import cs5004.marblesolitaire.model.hw09.EuropeanSolitaireModelImpl;
import cs5004.marblesolitaire.model.hw09.TriangleSolitaireModelImpl;


/**
 * Main class that can accept command line instructions.
 */
public final class MarbleSolitaire {

  /**
   * Name of types: english, european, triangular
   * -size # (the number is the size of the board)
   * -hole # # (the coordinate for where you want the hole.
   */
  public static void main(String[] args) {
    String type = "";
    MarbleSolitaireModel model = null;
    int size = 0; //default to zero.  if the user actually enters zero, then the default size is
    // used
    int[] hole = null;

    for (int i = 0; i < args.length; i++) {
      if (args[i].equals("english") || args[i].equals("european") || args[i].equals("triangular")) {
        type = args[i];
      } else if (args[i].equals("-size")) {
        i++;
        size = Integer.parseInt(args[i]);
      } else if (args[i].equals("-hole")) {
        hole = new int[]{Integer.parseInt(args[++i]), Integer.parseInt(args[++i])};
      }
    }

    switch (type) {
      case "english":
        if (size == 0 && hole == null) {
          model = new MarbleSolitaireModelImpl();
        } else if (size != 0 && hole == null) {
          model = new MarbleSolitaireModelImpl(size);
        } else if (size == 0 && hole != null) {
          model = new MarbleSolitaireModelImpl(hole[0], hole[1]);
        } else {
          model = new MarbleSolitaireModelImpl(size, hole[0], hole[1]);
        }
        break;
      case "european":
        if (size == 0 && hole == null) {
          model = new EuropeanSolitaireModelImpl();
        } else if (size != 0 && hole == null) {
          model = new EuropeanSolitaireModelImpl(size);
        } else if (size == 0 && hole != null) {
          model = new EuropeanSolitaireModelImpl(hole[0], hole[1]);
        } else {
          model = new EuropeanSolitaireModelImpl(size, hole[0], hole[1]);
        }
        break;
      case "triangular":
        if (size == 0 && hole == null) {
          model = new TriangleSolitaireModelImpl();
        } else if (size != 0 && hole == null) {
          model = new TriangleSolitaireModelImpl(size);
        } else if (size == 0 && hole != null) {
          model = new TriangleSolitaireModelImpl(hole[0], hole[1]);
        } else {
          model = new TriangleSolitaireModelImpl(size, hole[0], hole[1]);
        }
        break;
      default:
        System.out.println("Nothing happened");
    }
    if (model != null) {
      System.out.println(model.getGameState());
    }
  }
}