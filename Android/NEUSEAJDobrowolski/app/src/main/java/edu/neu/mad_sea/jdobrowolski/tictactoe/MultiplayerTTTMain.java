package edu.neu.mad_sea.jdobrowolski.tictactoe;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;

import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;

import java.util.Scanner;

import edu.neu.mad_sea.jdobrowolski.R;

public class MultiplayerTTTMain extends FragmentActivity implements FragmentChangeListener{

    private FragmentManager fm;
    private FragmentTransaction ft;
    private Button start;
    protected FirebaseDatabase database;
    protected DatabaseReference dbRef;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_multiplayer_t_t_t_main);

        fm = getSupportFragmentManager();
        ft = fm.beginTransaction();
        TicTacToeLoginFragment login = new TicTacToeLoginFragment();
        login.setContext(this);
        ft.replace(R.id.TicTacToe_display, login);
        ft.commit();

    }

    // https://stackoverflow.com/a/21229014
    @Override
    public void replaceFragment(Fragment fragment) {
        FragmentManager fragmentManager = getSupportFragmentManager();;
        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        fragmentTransaction.replace(R.id.TicTacToe_display, fragment, fragment.toString());
        fragmentTransaction.commit();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}