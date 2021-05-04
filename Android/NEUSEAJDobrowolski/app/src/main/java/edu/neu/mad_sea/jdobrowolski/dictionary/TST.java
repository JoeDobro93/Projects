package edu.neu.mad_sea.jdobrowolski.dictionary;

import android.content.Context;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Writer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Stack;

/**
 * Creates a TST with a head containing an array of nodes for each alphabetical character.
 * This simplified datastructure does not need a delete, as it is only for reading in a dictionary.
 * It also does not need to "get" any values, since only bools are contained in each node so
 * "contains" performs the same function.
 */
public class TST {
    private Node[] head;
    Context context;
    private static final int radix = 26; // change this if you intend on modifying the radix

    private class Node {
        private char character;
        private Node left;
        private Node middle;
        private Node right;
        private boolean wordEnd;

        public Node (char character) {
            this.character = character;
            left = null;
            right = null;
            middle = null;
            wordEnd = false;
        }
    }

    /**
     * Default constructor, creates a new array of Nodes of size 'radix'
     */
    public TST () {
        head = new Node[radix];
    }

    /**
     * @param fileName
     * @param isDictionary if true, read in as dictionary.  if false, real in as tst text file
     */
    public TST (String fileName, Context context, boolean isDictionary) {
        head = new Node[26];
        this.context = context;
        if (isDictionary) {
            try {
                // Reads in from a file and creates new TST entries for every line
                InputStreamReader isr = new InputStreamReader(context.getAssets().open(fileName));
                BufferedReader br = new BufferedReader(isr);
                String line;
                while ((line = br.readLine()) != null) {
                    insert(line);
                }
            } catch (Exception e) {
                System.out.println(e.getMessage());
            }
        }
        else { //IGNORE THIS CODE, THIS IS FOR AN UNFINISHED FEATURE I DECIDED WASN'T WORTH DOING
                // FOR THIS ASSIGNMENT.  I may revist this later which is why I left this.
            try (BufferedReader br = new BufferedReader(new FileReader(fileName))) {
                String line;
                Stack<Node> nodeStack = new Stack<>();

                //TODO: this section
            } catch (Exception e) {
                System.out.println(e.getMessage());
            }
        }

    }

    /**
     * Allows a new word to be inserted into current TST.  Not used by Dictionary, but left public
     * in case a future Activity needs this and needs it.
     * @param word
     */
    public void insert(String word) {
        word = word.toLowerCase(); // set to all lowercase, don't care about capitalization.

        head[word.charAt(0) - 'a'] = insert(head[word.charAt(0) - 'a'], word, 0);
    }

    /**
     * Insert helper function that is really where all the work is handled.  Recursively traverses
     * or adds to the TST until the end of the new word is reached.  If a blank node is reached, a
     * new one is created and added to the left if a mismatch and lesser, right if a mismatch and
     * greater, or to the middle if the previous was a match.
     */
    private Node insert(Node node, String word, int index) {
        if (node == null) {
            node = new Node(word.charAt(index));
        }
        if (word.charAt(index) < node.character) {
            node.left = insert(node.left, word, index);
        } else if (word.charAt(index) > node.character) {
            node.right = insert(node.right, word, index);
        } else if (index < word.length() - 1) {
            node.middle = insert(node.middle, word, index + 1);
        } else {
            node.wordEnd = true;
        }

        return node;
    }

    /**
     * Checks if a word is contained in the TST.  Not needed by Dictionary but left in just in case
     * needed in future Activities.
     * @param word
     * @return
     */
    public boolean contains(String word) {
        return contains(head[word.charAt(0) - 'a'], word, 0);
    }

    /**
     * Contains helper function that is really where all the work is handled.  Recursively traverses
     * or adds to the TST until the end of the new word is reached.  If a blank node is reached,
     * false is returned. If not, traverses the TST much like insert does, until the length of the
     * input word is reached and we can check that node for a match, or we reach a null.
     */
    private boolean contains(Node node, String word, int index) {
        if (node == null) {
            return false;
        }

        if (word.charAt(index) < node.character) {
            return contains(node.left, word, index);
        } else if (word.charAt(index) > node.character) {
            return contains(node.right, word, index);
        } else if (index < word.length() - 1) {
            return contains(node.middle, word, index + 1);
        } else {
            return node.wordEnd;
        }
    }

    /**
     * UNUSED/UNFINISHED IMPLEMENTATION.  MAY REVISIT LATER BUT PLEASE IGNORE FOR NOW.
     * @throws IOException
     */
    public void writeFile() throws IOException {
        Writer outFile = new FileWriter("src/tst.txt", false);
        outFile.write(toString());
        outFile.close();
    }

    /**
     * @param word the original word used as a "letter bank"
     * @param constraint uses "_" for wild cards and letters for predetermined characters.  Length
     *                   is the length constraint.
     * @return
     */
    public String[] getAnagrams (String word, String constraint) {
        HashSet<String> wordSet = new HashSet<>();
        HashMap<Character, Integer> letterMap = new HashMap<>(); //keeps track of valid letters and quantity

        //TODO: throw exceptions for bad constraints (mostly handled in Dictionary.java already)

        for (Character c : word.toCharArray()) {
            if (!letterMap.containsKey(c)) {
                letterMap.put(c, 1);
            } else {
                letterMap.put(c, letterMap.get(c) + 1);
            }
        }

        // For slight efficiency boost, the "first letters" are handled as a 26 character array
        // containing subtsts that start with these letters.
        // This iterates over each of these first letters and checks if they match a letter in the
        // "word".  This also prevents the same first letter from being checked more than once
        for (Node node : head) {
            // each time a word is found, it is added to the wordSet within the private helper
            // method.
            if (letterMap.containsKey(node.character) && head[node.character - 'a'] != null) {
                getAnagrams(node, letterMap, constraint, wordSet);
            }
        }

        String[] result = new String[wordSet.size()];
        int i = 0;
        for (String s : wordSet) {
            result[i] = s;
            ++i;
        }
        Arrays.sort(result);

        return result;
    }

    /**
     * Traverses the TST looking for words that exist in the TST and are anagrams from some subset
     * of characters contained in the base word.
     * We start with a blank char[] to hold onto the word being built.  This is added onto as we go
     * down a path where all the characters in that path are in the original word.  These valid
     * letters are stored in a HashMap containing the letter as well as how many occurrences of that
     * letter were in the original word for linear lookup time.  Whenever a match is found, that
     * number is decremented.
     * Whenever we hit a deadend (null or a character not available in the letterMap) or reach the
     * length of the constraint, we backtrack to the last branching node.  When do this by pushing
     * new nodes and their current index level to two stacks and popping to get back.  We use the
     * index to remove characters from the wordBuilder and add them back into the letterMap.
     *
     * Could be further improved by not adding index when middle is pushed, but this changed a few
     * parts to an extend where time constraints made it not worth it.  May also be possible to
     * reduce redundancy of calling backtrack in several if statements separately, but again I
     * decided the time restrictions made this not a priority.
     *
     * @param node
     * @param letterMap
     * @param constraint
     * @param wordSet
     */
    private void getAnagrams (Node node, HashMap<Character, Integer> letterMap, String constraint,
                              HashSet<String> wordSet) {

        Stack<Node> next = new Stack<>();
        Stack<Integer> indexStack = new Stack<>();
        next.push(node);
        indexStack.push(-1);

        Character[] wordBuild = new Character[constraint.length()];
        Arrays.fill(wordBuild, null);

        int index; // keeps track of how many indices are *currently* filled.

        while (!next.isEmpty()) {
            node = next.pop();
            index = indexStack.pop();


            // add even if current node doesn't match
            if (node.left != null) {
                next.push(node.left);
                indexStack.push(index);
            }
            // add even if current node doesn't match
            if (node.right != null) {
                next.push(node.right);
                indexStack.push(index);
            }

            //increment index if the current node is valid
            if (// check that current node character is a candidate given the input word
                    letterMap.containsKey(node.character)
                            // check that the candidate character hasn't already been used up
                            && letterMap.get(node.character) > 0
                            // check that this character is compatible with the constraint
                            && (node.character == constraint.charAt(index + 1)
                            || constraint.charAt(index + 1) == '_'))
            {

                index++;
                // add current character to the wordBuilder
                wordBuild[index] = node.character;

                // decrement the number of that character available
                letterMap.put(node.character, letterMap.get(node.character) - 1);

                if (index == constraint.length() - 1) {
                    if (node.wordEnd) {
                        String word = "";
                        for (char c : wordBuild) {
                            word += c;
                        }
                        wordSet.add(word);
                    }
                    // backtrack the wordBuild to the previous branch
                    backtrack(wordBuild, indexStack.isEmpty() ? -1 : indexStack.peek(), letterMap);
                }
                else if (node.middle != null) {
                    next.push(node.middle);
                    indexStack.push(index);
                } else {
                    backtrack(wordBuild, indexStack.isEmpty() ? -1 : indexStack.peek(), letterMap);
                }
            } else {
                backtrack(wordBuild, indexStack.isEmpty() ? -1 : indexStack.peek(), letterMap);
            }
        }
    }

    /**
     * Helper function which backtracks the wordBuild to an earlier index and repopulates the
     * letterMap with the characters being removed from the wordBuilder
     * @param word from our wordBuild.  stores chars in word being built we want to undo.
     * @param index index we want to return to
     * @param letterMap map of valid characters to add back into.
     */
    private void backtrack(Character[] word, int index, HashMap<Character, Integer> letterMap) {
        for (int i = word.length - 1; i > index; --i) {
            if (word[i] != null) {
                letterMap.put(word[i], letterMap.get(word[i]) + 1);
                word[i] = null;
            }
        }
    }

    @Override
    public String toString() {
        //TODO: this is incomplete, do it if there's time to implement a quick load for a tst
        String str = "";

        Stack<Node> stack = new Stack<>();
        for (Node node : head) {
            if (node == null) continue;
            stack.push(node);
            str += toString(node);
        }

        return str;
    }

    private String toString(Node node) {
        String str = "";
        str += String.format("%s,%b,%b,%b,%b\n",
                node.character, node.wordEnd,
                node.middle != null, node.left != null, node.right != null);

        if (node.middle != null) {
            str += toString(node.middle);

            // left/right can only exist if the middle isn't null
            if (node.left != null) {
                str += toString(node.left);
            }
            if (node.right != null) {
                str += toString(node.right);
            }
        }
        return str;
    }
}
