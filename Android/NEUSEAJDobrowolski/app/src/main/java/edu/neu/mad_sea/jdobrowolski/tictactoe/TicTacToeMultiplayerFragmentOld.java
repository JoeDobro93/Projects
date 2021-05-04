package edu.neu.mad_sea.jdobrowolski.tictactoe;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import edu.neu.mad_sea.jdobrowolski.R;

public class TicTacToeMultiplayerFragmentOld extends Fragment implements View.OnClickListener {

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
    private final FirebaseDatabase database = FirebaseDatabase.getInstance();
    private DatabaseReference dbRef;
    private String gameNumber;
    private GameState gs;
    private Boolean player;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View view = inflater.inflate(R.layout.fragment_tic_tac_toe, container, false);

        model = new TicTacToeModel();

        // set and find IDs for TextViews and reset button
        playerOneScore = (TextView) view.findViewById(R.id.playerOneScore);
        playerTwoScore = (TextView) view.findViewById(R.id.playerTwoScore);
        playerStatus = (TextView) view.findViewById(R.id.playerStatus);
        resetGame = (Button) view.findViewById(R.id.resetGame);

        // sets and finds all of the IDs for the game piece buttons.
        for (int i=0; i < buttons.length; i++) {
            String buttonID = "btn_" + i;
            int resourceID = view.getResources().getIdentifier(buttonID, "id",
                    getActivity().getPackageName());
            buttons[i] = (Button) view.findViewById(resourceID);
            buttons[i].setOnClickListener(this);
            buttons[i].setText(R.string.unused_space);
        }

        dbRef = database.getReference("game").child(gameNumber);

        dbRef.addListenerForSingleValueEvent(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                player = (Boolean) dataSnapshot.child("player").getValue();
                if (dataSnapshot.child("activePlayers").getValue().toString().equals("2")) {
                    player = !player;
                }

                for (int i = 0; i < buttons.length; ++i) {
                    String playerOnSpace = (String) dataSnapshot
                            .child("board")
                            .child(i + "")
                            .getValue();

                    if (playerOnSpace.equals("X") || playerOnSpace.equals("O")) {
                        setPlayerColor(buttons[i], playerOnSpace.equals("X"));
                    }
                }

                //if (player != model.getActivePlayer()) model.changeActivePlayer();
                //gs = new GameState();
                //gs.setActivePlayers((Integer) dataSnapshot.child("activePlayers").getValue());
            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {

            }
        });


        dbRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                Boolean currentTurn = (Boolean) dataSnapshot.child("player").getValue();

                if (currentTurn != model.getActivePlayer()) {
                    model.changeActivePlayer();
                    model.nextTurn();
                }


                if (currentTurn == player) {
                    for (int i = 0; i < buttons.length; ++i) {
                        String playerOnSpace = (String) dataSnapshot
                                .child("board")
                                .child(i + "")
                                .getValue();

                        if (playerOnSpace.equals("X") || playerOnSpace.equals("O")) {
                            setPlayerColor(buttons[i], playerOnSpace.equals("X"));
                        }
                    }
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {

            }
        });

        // Inflate the layout for this fragment
        return view;
    }

    @Override
    public void onClick(View v) {
        // Checks to see if a button has been previously been pressed.
        // If button has been pressed, players cannot reselect a button, but if not, the button
        // is still selectable in game play.
        if(!((Button) v).getText().toString().equals(getString(R.string.unused_space))
                || model.getActivePlayer() != this.player){
            return;
        }

        // Below finds the button ID that was clicked and updates the game state.
        String buttonID = v.getResources().getResourceEntryName((v.getId()));
        int gameStatePointer = Integer.parseInt(buttonID.substring(buttonID.length()-1, buttonID.length()));

        setPlayerColor((Button) v); // set the colors
        model.setGameTile(gameStatePointer); // update the model state
        model.nextTurn(); // update model to switch to the next player

        if (endOfGame()) playAgain(); // resets the game if a winner is found or the 9th turn
        else {
            model.changeActivePlayer(); // if the game is not over, change players
            dbRef.child("player").setValue(model.getActivePlayer());
        }

        updatePlayerStatus();

        // Reset game TODO: Can I get this out of here?
        resetGame.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                model.reset();
                playerStatus.setText(R.string.no_one_in_lead);
                updatePlayerScore();
                for (int i = 0; i < buttons.length; i++) {
                    buttons[i].setText(R.string.unused_space);
                }
            }
        });

    }


    /**
     * Sets the button color based on the current player
     *
     * @param b the button being updated
     */
    private void setPlayerColor(Button b) {
        // Player markings depending on turn.
        if(model.getActivePlayer()) {
            b.setText(R.string.player1_piece);
            b.setTextColor(ContextCompat.getColor(this.getContext(), R.color.player1_color));
        }
        else{
            b.setText(R.string.player2_piece);
            b.setTextColor(ContextCompat.getColor(this.getContext(), R.color.player2_color));
        }

        for (int i = 0; i < buttons.length; ++i) {
            String piece = buttons[i].getText().toString();

            dbRef.child("board").child("" + i).setValue(
                    piece.equals("X") ? "X" :
                            piece.equals("O") ? "O" : "");
        }
    }

    /**
     * Sets the button color based on the current player
     *
     * @param b the button being updated
     */
    private void setPlayerColor(Button b, boolean player) {
        // Player markings depending on turn.
        if(player) {
            b.setText(R.string.player1_piece);
            b.setTextColor(ContextCompat.getColor(this.getContext(), R.color.player1_color));
        }
        else{
            b.setText(R.string.player2_piece);
            b.setTextColor(ContextCompat.getColor(this.getContext(), R.color.player2_color));
        }
    }

    /**
     * Returns a bool if the game is over or not.  If a winner is found, the model is also updated
     * to reflect a new win.
     */
    private boolean endOfGame() {
        if(model.checkWinner()) {
            if (model.getActivePlayer()) {
                model.pointForPlayer1();
                updatePlayerScore();
                Toast.makeText(getActivity(), R.string.player1_wins, Toast.LENGTH_SHORT).show();
            }
            else {
                model.pointForPlayer2();
                updatePlayerScore();
                Toast.makeText(getActivity(), R.string.player2_wins, Toast.LENGTH_SHORT).show();
            }
        } else if (model.getTurn() == 9) {
            // If nine rounds have been reached, no winner is declared.
            Toast.makeText(getActivity(), R.string.tie_game, Toast.LENGTH_SHORT).show();
        } else{
            // the game is not over
            return false;
        }

        return true;
    }

    // Updates player scores.
    private void updatePlayerScore() {
        playerOneScore.setText(String.format("%d", model.getPlayer1Score()));
        playerTwoScore.setText((String.format("%d", model.getPlayer2Score())));
    }

    // Starts a new round with a clean game board.
    private void playAgain() {
        model.nextGame();

        for (int i = 0; i < buttons.length; i++) {
            buttons[i].setText(R.string.unused_space);
        }
    }

    private void updatePlayerStatus() {
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
    }

    protected void setGameNumber(String gameNumber) {
        this.gameNumber = gameNumber;
    }

}
