package edu.neu.mad_sea.jdobrowolski;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import edu.neu.mad_sea.jdobrowolski.dictionary.Acknowledgments;
import edu.neu.mad_sea.jdobrowolski.dictionary.ErrorDialog;
import edu.neu.mad_sea.jdobrowolski.dictionary.TST;

public class Dictionary extends AppCompatActivity {
    TST wordList;
    Button find;
    Button clear;
    Button acknowledgements;
    TextView input;
    TextView resultDisplay;
    LinearLayout constraint;
    TextView constraintSize;
    TextView timeDisplay;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // https://stackoverflow.com/a/2591311
        try { // get rid of the top title bar to get more room for the activity
            this.getSupportActionBar().hide();
        } catch (NullPointerException e){} // just assuming it works I guess, if not it just stays.

        // Populate a TST based on the desired dictionary.
        wordList = new TST("wordlist.txt", this, true);

        setContentView(R.layout.activity_dictionary);

        // get all the buttons/textfields that we need to use and store them locally
        // also assign button functionality
        input = (TextView) findViewById(R.id.word_input);

        resultDisplay = (TextView) findViewById(R.id.result_display);
        resultDisplay.setMovementMethod(new ScrollingMovementMethod());

        timeDisplay = (TextView) findViewById(R.id.time_text);

        constraintSize = (TextView) findViewById(R.id.constraint_size);
        constraint = findViewById(R.id.constraints);

        find = (Button) findViewById(R.id.find_button);
        find.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    findWords();
                } catch (Exception e) {
                    // Makes error popup if something is not right.
                    openDialog(e.getMessage());
                }
            }
        });

        clear = (Button) findViewById(R.id.clear_button);
        clear.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                clearAll();
            }
        });

        acknowledgements = (Button) findViewById(R.id.acknowledgement_button);
        acknowledgements.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openAcknowledgement();
            }
        });
    }

    /**
     * Takes input from all text parameters and Tries (pun intended) to find all words within the
     * constraint that can be made from a given word.
     */
    public void findWords() {
        String[] resultArray;
        String constraintStr = "";

        // Exceptions if the input is bad with a message that will be sent to the popup ErrorDialog
        if (input.getText().length() > 10) { // this should be impossible because of maxLength
            throw new IllegalArgumentException("Must contain 10 or fewer characters");
        }
        if (input.getText().length() == 0) { // no reference text entered
            throw new IllegalArgumentException("Please enter some text");
        }
        if (Integer.parseInt(constraintSize.getText().toString()) > input.getText().length()) {
            // if the constraint is too long.  This won't break anything, however, either way
            throw new IllegalArgumentException("Constraint can't be longer than source text");
        }
        if (constraintSize.getText().toString().equals("") ||
                Integer.parseInt(constraintSize.getText().toString()) < 1) {
            // if no length was entered or the length is 0 or negative
            throw new IllegalArgumentException("Please enter a length > 0 for the constraint");
        }

        // Iterate over the constraint HorizontalView components in order.  This will get each
        // entered constraint character up to the specified length.  If there is a character, it
        // is added to the constraint string, if not a '_' is added instead.
        for (int i = 0; i < Integer.parseInt(constraintSize.getText().toString()); ++i) {
            TextView t = (TextView) constraint.getChildAt(i);
            if (t.getText().toString().toLowerCase().compareTo("a") >= 0
                    && t.getText().toString().toLowerCase().compareTo("z") <= 0) {
                constraintStr += t.getText().toString();
            } else {
                constraintStr += "_";
            }
        }

        double timer = System.currentTimeMillis();  //start timer
        // sends in the word and constraint to the TST with dictionary loaded in and gets a String
        // array of all matching words.
        resultArray = wordList.getAnagrams(input.getText().toString(), constraintStr);
        timer = System.currentTimeMillis() - timer; // end timer and find difference for duration
        timer = timer/1000;  // convert to seconds
        String timeText = timer + "s"; // convert to string first (didn't like this as param)
        timeDisplay.setText(timeText); // update the time display

        // update the result display with new text
        String printMe = "";
        for (String str : resultArray) {
            printMe += str + "\n";
        }
        resultDisplay.setText(printMe);

        // throws exception if no results found to give user feedback via ErrorDialog popup
        if (resultArray.length < 1) {
            throw new IllegalArgumentException("No results found");
        }
    }

    /**
     * Sets all text fields to blank
     */
    public void clearAll() {
        for (int i = 0; i < constraint.getChildCount(); ++i) {
            TextView t = (TextView) constraint.getChildAt(i);
            t.setText("");
        }
        input.setText("");

        constraintSize.setText("");
        timeDisplay.setText("");
        resultDisplay.setText("");
    }

    /**
     * @param message accepts an error message to use in an ErrorDailog popup to give the user a
     *                useful message to help identify a bad parameter.
     */
    public void openDialog(String message) {
        ErrorDialog error = new ErrorDialog();
        error.setMessage(message);
        error.show(getSupportFragmentManager(), "something"); // don't understand what the tag
        // is used for honestly
    }

    /**
     * My thrown together acknowledgement dialog popup.
     */
    public void openAcknowledgement() {
        Acknowledgments ack = new Acknowledgments();
        ack.show(getSupportFragmentManager(), "");
    }
}