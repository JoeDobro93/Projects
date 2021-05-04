package edu.neu.mad_sea.jdobrowolski.tictactoe;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

import edu.neu.mad_sea.jdobrowolski.R;

import android.graphics.Color;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

/**
 * This Android in Java application is meant to represent
 * the game tic-tac-toe. Game play details can be found at
 * https://en.wikipedia.org/wiki/Tic-tac-toe
 */

public class TicTacToe extends AppCompatActivity implements View.OnClickListener{

    TicTacToeModel model;

    /**
     * UI Components:
     *    - playerOneScore: number of games won by player one
     *    - playerTwoScore: number of games won by player two
     *    - playerStatus: displays the current overall game leader
     *    - buttons: nine game piece buttons
     *    - resetGame: button used to reset the entire game, player scores, and playerStatus
     */
    private TextView playerOneScore, playerTwoScore, playerStatus;
    private Button[] buttons = new Button[9];
    private Button resetGame;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_tic_tac_toe);

        // initialize the model
        model = new TicTacToeModel();

        // set and find IDs for TextViews and reset button
        playerOneScore = (TextView) findViewById(R.id.playerOneScore);
        playerTwoScore = (TextView) findViewById(R.id.playerTwoScore);
        playerStatus = (TextView) findViewById(R.id.playerStatus);
        resetGame = (Button) findViewById(R.id.resetGame);

        // sets and finds all of the IDs for the game piece buttons.
        for (int i=0; i < buttons.length; i++) {
            String buttonID = "btn_" + i;
            int resourceID = getResources().getIdentifier(buttonID, "id", getPackageName());
            buttons[i] = (Button) findViewById(resourceID);
            buttons[i].setOnClickListener(this);
        }
    }

    @Override
    public void onClick(View v) {
        // Checks to see if a button has been previously been pressed.
        // If button has been pressed, players cannot reselect a button, but if not, the button
        // is still selectable in game play.
        if(!((Button) v).getText().toString().equals("")){
            return;
        }

        // Below finds the button ID that was clicked and updates the game state.
        String buttonID = v.getResources().getResourceEntryName((v.getId()));
        int gameStatePointer = Integer.parseInt(buttonID.substring(buttonID.length()-1, buttonID.length()));

        // Player markings depending on turn.
        if(model.getActivePlayer()) {
            ((Button)v).setText(R.string.player1_piece);
            ((Button)v).setTextColor(Color.parseColor("#FFC34A"));
        }
        else{
            ((Button)v).setText(R.string.player2_piece);
            ((Button)v).setTextColor(Color.parseColor("#70FFEA"));
        }
        model.setGameTile(gameStatePointer);

        model.nextTurn();

        // Below. checks for a winner
        if(model.checkWinner()) {
            if (model.getActivePlayer()) {
                model.pointForPlayer1();
                updatePlayerScore();
                Toast.makeText(this, R.string.player1_wins, Toast.LENGTH_SHORT).show();
                playAgain();
            }
            else {
                model.pointForPlayer2();
                updatePlayerScore();
                Toast.makeText(this, R.string.player2_wins, Toast.LENGTH_SHORT).show();
                playAgain();
            }
        } else if (model.getTurn() == 9) {
            // If nine rounds have been reached, no winner is declared.
            playAgain();
            Toast.makeText(this, R.string.tie_game, Toast.LENGTH_SHORT).show();
        } else{
            // If there is no winner, switch  player.
            model.changeActivePlayer();
        }

        // Set the player status if one player has more wins than the other.
        if( model.getPlayer1Score() > model.getPlayer2Score()) {
            playerStatus.setText(R.string.player1_in_lead);
        }
        else if( model.getPlayer1Score() < model.getPlayer2Score()) {
            playerStatus.setText(R.string.player2_in_lead);
        }
        else {
            playerStatus.setText(R.string.no_one_in_lead);
        }

        // Reset game
        resetGame.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                model.reset();
                playerStatus.setText(R.string.no_one_in_lead);
                updatePlayerScore();
                for (int i = 0; i < buttons.length; i++) {
                    buttons[i].setText("");
                }
            }
        });

    }



    // Updates player scores.
    public void updatePlayerScore() {
        playerOneScore.setText(String.format("%d", model.getPlayer1Score()));
        playerTwoScore.setText((String.format("%d", model.getPlayer2Score())));
    }


    // Starts a new round with a clean game board.
    public void playAgain() {
        model.nextGame();

        for (int i = 0; i < buttons.length; i++) {
            buttons[i].setText("");
        }
    }

}