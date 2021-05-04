package edu.neu.mad_sea.jdobrowolski.tictactoe;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;
import com.google.firebase.messaging.FirebaseMessaging;

import edu.neu.mad_sea.jdobrowolski.R;

/**
 * A simple {@link Fragment} subclass.
 * Use the {@link TicTacToeLoginFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class TicTacToeLoginFragment extends Fragment {
    private Button startButton;
    private String gameNumber;
    private final FirebaseDatabase database = FirebaseDatabase.getInstance();
    private DatabaseReference dbRef;
    private Context context;

    public void setContext(Context context) {
        this.context = context;
    }

    public TicTacToeLoginFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     * @return A new instance of fragment TicTacToeLoginFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static TicTacToeLoginFragment newInstance() {
        TicTacToeLoginFragment fragment = new TicTacToeLoginFragment();
        Bundle args = new Bundle();
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        final View view = inflater.inflate(R.layout.fragment_tic_tac_toe_login, container, false);

        dbRef = database.getReference();

        // Inflate the layout for this fragment
        startButton = (Button) view.findViewById(R.id.ttt_login_start);
        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final TextView passCode = (TextView) view.findViewById(R.id.ttt_passcode_box);
                final TextView userName = (TextView) view.findViewById(R.id.player_name_field);
                final String userNameText = userName.getText().toString();
                final String gameNumber = passCode.getText().toString();

                dbRef = database.getReference().child("game").child(gameNumber);
                dbRef.addListenerForSingleValueEvent(new ValueEventListener() {
                    @Override
                    public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                        if (dataSnapshot.exists()) {
                            if (!dataSnapshot.child("p2Name").getValue().toString().equals("")) {
                                if(userNameText.equals(dataSnapshot.child("p1Name").getValue().toString())) {
                                    startGame(gameNumber, true);
                                }
                                else if(userNameText.equals(dataSnapshot.child("p2Name").getValue().toString())) {
                                    startGame(gameNumber, false);
                                } else {
                                    //TODO: popup for error message
                                    passCode.setText(""); // clear text if game full
                                }

                            } else {
                                // start game if valid
                                if (!userNameText.equals(dataSnapshot.child("p1Name").getValue().toString())) {
                                    dbRef.child("p2Name").setValue(userName.getText().toString());
                                    startGame(gameNumber, false); // false sets player 2
                                } else {
                                    //TODO: popup for error message
                                    passCode.setText(""); // clear text if game full
                                }
                            }
                        } else {
                            GameState gs = new GameState();
                            gs.setActivePlayers(1);
                            gs.setP1Name(userName.getText().toString());
                            gs.setGameNumber(gameNumber);

                            dbRef.setValue(gs);
                            startGame(gameNumber, true); // true sets player 1
                        }
                    }

                    @Override
                    public void onCancelled(@NonNull DatabaseError databaseError) {

                    }
                });
            }
        });

        return view;
    }

    private void startGame(String gameNumber, Boolean player) {
        TicTacToeMultiplayerFragment boardLayout;

        boardLayout = new TicTacToeMultiplayerFragment();
        boardLayout.setNumberAndPlayer(gameNumber, player, context);
        FragmentChangeListener fc = (FragmentChangeListener)getActivity();
        fc.replaceFragment(boardLayout);

    }
}