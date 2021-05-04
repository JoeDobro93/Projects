package cs5004.animator.controller;

import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.util.Map;

/**
 * Keyboard listener to be used in our view.  Any command can be added from the controller and is
 * stored in here to handle when the key is pressed.  Essentially copied from the examples from
 * class.
 */
public class KeyboardListener implements KeyListener {
  private Map<Character, Runnable> keyTypedMap;
  private Map<Integer, Runnable> keyPressedMap;
  private Map<Integer, Runnable> keyReleasedMap;

  /**
   * Empty default constructor.
   */
  public KeyboardListener() {
    //empty by default, as was in example.
  }

  /**
   * Stores commands that are executed upon something being typed (not just pressed and released but
   * results in character input somewhere).
   *
   * @param map map of the character and the command attached to it.
   */
  public void setKeyTypedMap(Map<Character, Runnable> map) {
    keyTypedMap = map;
  }

  /**
   * Stores commands that are executed upon a key press, even before release.
   *
   * @param map map of the key to press and the command attached to it.
   */
  public void setKeyPressedMap(Map<Integer, Runnable> map) {
    keyPressedMap = map;
  }

  /**
   * Stores commands that are executed upon key release after being pressed down.
   *
   * @param map map of the key to release and the command attached to it.
   */
  public void setKeyReleasedMap(Map<Integer, Runnable> map) {
    keyReleasedMap = map;
  }

  @Override
  public void keyTyped(KeyEvent e) {
    if (keyTypedMap.containsKey(e.getKeyChar())) {
      keyTypedMap.get(e.getKeyChar()).run();
    }
  }

  @Override
  public void keyPressed(KeyEvent e) {
    if (keyPressedMap.containsKey(e.getKeyCode())) {
      keyPressedMap.get(e.getKeyCode()).run();
    }
  }

  @Override
  public void keyReleased(KeyEvent e) {
    if (keyReleasedMap.containsKey(e.getKeyCode())) {
      keyReleasedMap.get(e.getKeyCode()).run();
    }
  }
}
