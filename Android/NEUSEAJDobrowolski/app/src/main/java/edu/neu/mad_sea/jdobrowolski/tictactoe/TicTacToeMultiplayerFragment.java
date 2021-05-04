package edu.neu.mad_sea.jdobrowolski.tictactoe;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;


import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Scanner;

import edu.neu.mad_sea.jdobrowolski.R;

public class TicTacToeMultiplayerFragment extends Fragment {

    private static final String SERVER_KEY = "key=AAAAwxMW8Ks:APA91bFGUo-fxH-tG__bKmLIZ6qaITb9OphI29KMj4aXq1Sm6Y3KtQPpM1d3Kl-mxAw6iCGr_dRehyaIx6MlykKGUEcDx_HbEtj0XG9HqjEV4zc5mUutyWHMiDY5YgzG-22jhT2hvLna";

    private TicTacToeModel model;
    private final FirebaseDatabase database = FirebaseDatabase.getInstance();
    private DatabaseReference dbRef;
    private String gameNumber;
    private GameState gs;
    private Boolean player;

    private Button[] buttons = new Button[9];
    private Button resetButton;
    private TextView playerOneScore, playerTwoScore, playerStatus;
    private TextView playerOneName, playerTwoName;

    private Context context;

    public static TicTacToeMultiplayerFragment newInstance(String gameNumber, Boolean player) {
        TicTacToeMultiplayerFragment tttBoardFragment = new TicTacToeMultiplayerFragment();

        Bundle args = new Bundle();
        args.putString("gameNumber", gameNumber);
        args.putBoolean("player", player);
        tttBoardFragment.setArguments(args);

        return tttBoardFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    //TODO: understand how to use newInstance to do this instead of this.
    public void setNumberAndPlayer(String gameNumber, Boolean player, Context context) {
        this.gameNumber = gameNumber;
        this.player = player;
        this.context = context;
    }

    // Handles what happens where a boardspace is clicked
    private View.OnClickListener boardSpaceClick = new View.OnClickListener() {
        @Override
        public void onClick(View v) {

        }
    };



    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        // instantiate the view
        View view = inflater.inflate(R.layout.fragment_tic_tac_toe, container, false);

        // initialize default gamestate
        gs = new GameState();
        // instantiate database reference to the current game number
        dbRef = database.getReference("game").child(gameNumber);

        // initialize model
        model = new TicTacToeModel();
        inidializeGameState();
        // if current player out of sync, update model
        if (model.getActivePlayer() != gs.getPlayer()) model.changeActivePlayer();

        initializeViews(view);
        initializeButtons(view);

        dbRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                List<String> currentBoard = new ArrayList<>();
                // initialize the board
                int[] gameState = {2,2,2,2,2,2,2,2,2};
                for (int i = 0; i < gs.getBoard().size(); ++i) {
                    String playerOnSpace = (String) dataSnapshot
                            .child("board")
                            .child(i + "")
                            .getValue();

                    // MAKING ASSUMPTION THAT ONLY X AND O ARE USED, I KNOW THIS IS FLAWED
                    //TODO: sanitize data on input
                    currentBoard.add(i, playerOnSpace);
                    if (playerOnSpace.equals("X") || playerOnSpace.equals("O")) {
                        setPlayerColor(buttons[i], playerOnSpace.equals("X"));
                        gameState[i] = playerOnSpace.equals("X") ? 0 : 1;
                    } else {
                        buttons[i].setText("");
                    }
                }
                model.setGameState(gameState);
                playerOneName.setText(dataSnapshot.child("p1Name").getValue().toString());
                playerTwoName.setText(dataSnapshot.child("p2Name").getValue().toString());
                gs.setBoard(currentBoard);
                gs.setPlayer((Boolean) dataSnapshot.child("player").getValue());

                gs.setTurnNumber(dataSnapshot.child("turnNumber").getValue(Integer.class));
                gs.setP1Score(dataSnapshot.child("p1Score").getValue(Integer.class));
                gs.setP2Score(dataSnapshot.child("p2Score").getValue(Integer.class));
                // initialize player names
                gs.setP1Name((String) dataSnapshot.child("p1Name").getValue());
                gs.setP2Name((String) dataSnapshot.child("p2Name").getValue());

                if (gs.getTurnNumber() == 0) {
                    model.nextGame();
                } else {
                    model.setTurn(gs.getTurnNumber());
                }

                model.setTurn(gs.getTurnNumber());
                model.setP1Score(gs.getP1Score());
                model.setP2Score(gs.getP2Score());

                updatePlayerStatus();
                updatePlayerScore();

                // TODO: do I need this if?
                if (model.getActivePlayer() != gs.getPlayer()) {
                    model.changeActivePlayer();
                    //model.nextTurn();
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {

            }
        });

        return view;
    }

    private void initializeViews(View view) {
        playerOneScore = (TextView) view.findViewById(R.id.playerOneScore);
        playerTwoScore = (TextView) view.findViewById(R.id.playerTwoScore);
        playerStatus = (TextView) view.findViewById(R.id.playerStatus);
        resetButton = (Button) view.findViewById(R.id.resetGame);
        playerOneName = (TextView) view.findViewById(R.id.playerOne);
        playerTwoName = (TextView) view.findViewById(R.id.playerTwo);
    }

    private void inidializeGameState() {
        final List<String> currentBoard = new ArrayList<>();

        dbRef.addListenerForSingleValueEvent(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                // initialize the board
                int[] gameState = {2,2,2,2,2,2,2,2,2};
                for (int i = 0; i < gs.getBoard().size(); ++i) {
                    String playerOnSpace = (String) dataSnapshot
                            .child("board")
                            .child(i + "")
                            .getValue();

                    // MAKING ASSUMPTION THAT ONLY X AND O ARE USED, I KNOW THIS IS FLAWED
                    //TODO: sanitize data on input
                    currentBoard.add(i, playerOnSpace);
                    if (playerOnSpace.equals("X") || playerOnSpace.equals("O")) {
                        setPlayerColor(buttons[i], playerOnSpace.equals("X"));
                        gameState[i] = playerOnSpace.equals("X") ? 0 : 1;
                    }
                }
                model.setGameState(gameState);

                playerOneName.setText(dataSnapshot.child("p1Name").getValue().toString());
                playerTwoName.setText(dataSnapshot.child("p2Name").getValue().toString());
                gs.setBoard(currentBoard);

                // initialize the active player
                // TODO: catch the null exception
                gs.setPlayer((Boolean) dataSnapshot.child("player").getValue());

                gs.setActivePlayers(Integer.parseInt(dataSnapshot.child("activePlayers").getValue().toString()));
                // initialize player names
                gs.setP1Name((String) dataSnapshot.child("p1Name").getValue());
                gs.setP2Name((String) dataSnapshot.child("p2Name").getValue());

                gs.setTurnNumber(dataSnapshot.child("turnNumber").getValue(Integer.class));
                gs.setP1Score(dataSnapshot.child("p1Score").getValue(Integer.class));
                gs.setP2Score(dataSnapshot.child("p2Score").getValue(Integer.class));

                if (gs.getTurnNumber() == 0) {
                    model.nextGame();
                } else {
                    model.setTurn(gs.getTurnNumber());
                }

                model.setP1Score(gs.getP1Score());
                model.setP2Score(gs.getP2Score());

                updatePlayerStatus();
                updatePlayerScore();

                // TODO: do I need this if?
                if (model.getActivePlayer() != gs.getPlayer()) {
                    model.changeActivePlayer();
                    //model.nextTurn();
                }

            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {

            }
        });
    }

    private void initializeButtons(View view) {
        resetButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                model.reset();
                playerStatus.setText(R.string.no_one_in_lead);
                updatePlayerScore();
                for (int i = 0; i < buttons.length; i++) {
                    buttons[i].setText(R.string.unused_space);
                }
                gs.setTurnNumber(0);
                String[] temp = new String[9];
                Arrays.fill(temp, "");
                gs.setBoard(new ArrayList<String>(Arrays.asList(temp)));
                dbRef.setValue(gs);
            }
        });

        // handles what happens when a button is clicked
        View.OnClickListener boardButtonListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(!((Button) v).getText().toString().equals(getString(R.string.unused_space))
                        || model.getActivePlayer() != player){
                    return;
                }

                // Below finds the button ID that was clicked and updates the game state.
                String buttonID = v.getResources().getResourceEntryName((v.getId()));
                int gameStatePointer = Integer
                        .parseInt(buttonID.substring(buttonID.length()-1, buttonID.length()));

                setPlayerColor((Button) v); // set the colors
                model.setGameTile(gameStatePointer); // update the model state
                model.nextTurn(); // update model to switch to the next player
                gs.setTurnNumber(model.getTurn());

                if (endOfGame()) playAgain(); // resets the game if a winner is found or the 9th turn
                else {
                    model.changeActivePlayer(); // if the game is not over, change players
                    gs.setPlayer(model.getActivePlayer());
                }
                updatePlayerStatus();
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        sendNotificationToAll(player ? gs.getP1Name() : gs.getP2Name());
                    }
                }).start();
                dbRef.setValue(gs);
            }
        };

        // sets and finds all of the IDs for the game piece buttons.
        for (int i=0; i < buttons.length; i++) {
            String buttonID = "btn_" + i;
            int resourceID = view.getResources().getIdentifier(buttonID, "id",
                    getActivity().getPackageName());
            buttons[i] = (Button) view.findViewById(resourceID);
            buttons[i].setOnClickListener(boardButtonListener);
            buttons[i].setText(R.string.unused_space);
        }
    }

    // Updates player scores.
    private void updatePlayerScore() {
        // TODO: Figure out what this warning means and how to fix it
        playerOneScore.setText(String.format("%d", model.getPlayer1Score()));
        playerTwoScore.setText((String.format("%d", model.getPlayer2Score())));
    }

    /**
     * Sets the button color based on the current player
     *
     * @param b the button being updated
     */
    private void setPlayerColor(Button b) {
        // Player markings depending on turn.
        if(player) {
            b.setText(R.string.player1_piece);
            // TODO: This is sometimes null.  figure out why
            b.setTextColor(ContextCompat.getColor(context, R.color.player1_color));
        }
        else{
            b.setText(R.string.player2_piece);
            b.setTextColor(ContextCompat.getColor(context, R.color.player2_color));
        }

        for (int i = 0; i < buttons.length; ++i) {
            String piece = buttons[i].getText().toString();

            gs.getBoard().set(i, piece);
        }
    }

    private void setPlayerColor(Button b, boolean player) {
        // Player markings depending on turn.
        if(player) {
            b.setText(R.string.player1_piece);
            b.setTextColor(ContextCompat.getColor(context, R.color.player1_color));
        }
        else{
            b.setText(R.string.player2_piece);
            b.setTextColor(ContextCompat.getColor(context, R.color.player2_color));
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
                gs.setP1Score(model.getPlayer1Score());
                Toast.makeText(getActivity(), R.string.player1_wins, Toast.LENGTH_SHORT).show();
            }
            else {
                model.pointForPlayer2();
                updatePlayerScore();
                Toast.makeText(getActivity(), R.string.player2_wins, Toast.LENGTH_SHORT).show();
                gs.setP2Score(model.getPlayer2Score());
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

    // Starts a new round with a clean game board.
    private void playAgain() {
        model.nextGame();

        for (int i = 0; i < buttons.length; i++) {
            buttons[i].setText(R.string.unused_space);
        }
        String[] emptyBoard = new String[9];
        Arrays.fill(emptyBoard, "");

        gs.setBoard(new ArrayList<String>(Arrays.asList(emptyBoard)));
        gs.setTurnNumber(0);
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

    private void sendNotificationToAll(String playerName) {
        JSONObject jNotification = new JSONObject();
        JSONObject jPayload = new JSONObject();

        try {
            jNotification.put("title", "Move Made!");
            jNotification.put("body", playerName + " just made a move in game " + gameNumber);
            jNotification.put("sound", "default");
            jNotification.put("badge", "1");

            jPayload.put("priority", "high");
            jPayload.put("notification", jNotification);
            jPayload.put("to", R.string.channel_name);

            URL url = new URL("https://fcm.googleapis.com/fcm/send");
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Authorization", SERVER_KEY);
            conn.setRequestProperty("Content-Type", "application/json");
            conn.setDoOutput(true);

            // Send FCM message content.
            OutputStream outputStream = conn.getOutputStream();
            outputStream.write(jPayload.toString().getBytes());
            outputStream.close();

            // Read FCM response.
            InputStream inputStream = conn.getInputStream();
            final String resp = convertStreamToString(inputStream);

            Handler h = new Handler(Looper.getMainLooper());
            h.post(new Runnable() {
                @Override
                public void run() {
                    Log.e("TicTacToe", "run: " + resp);
                    Toast.makeText(context,resp,Toast.LENGTH_LONG).show();
                }
            });
        } catch (JSONException | IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Helper function
     * @param is
     * @return
     */
    private String convertStreamToString(InputStream is) {
        Scanner s = new Scanner(is).useDelimiter("\\A");
        return s.hasNext() ? s.next().replace(",", ",\n") : "";
    }

    @Nullable
    @Override
    public Context getContext() {
        return context;
    }
}
