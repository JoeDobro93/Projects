package edu.neu.mad_sea.jdobrowolski;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import edu.neu.mad_sea.jdobrowolski.altassign1.AltAssign1Main;
import edu.neu.mad_sea.jdobrowolski.altassign2.MapDisplay;
import edu.neu.mad_sea.jdobrowolski.tictactoe.MultiplayerTTTMain;

public class
MainActivity extends AppCompatActivity {
    private Button aboutButton;
    private Button errorButton;
    private Button dictionaryButton;
    private Button ticTacToeButton;
    private Button altAssign1Button;
    private Button mapDisplayButton;

    public static final String CHANNEL_NAME = "cs5520channel";
    private static final int MY_PERMISSIONS_REQUEST_RECEIVE_SMS = 1;
    private static final int MY_PERMISSIONS_REQUEST_RECEIVE_LOCATION = 34;
    private static final int MY_PERMISSIONS_REQUEST_RECEIVE_LOCATION_FINE = 35;
    private static final String TAG = "MainActivity";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (ActivityCompat.checkSelfPermission(this,
                Manifest.permission.RECEIVE_SMS) !=
                PackageManager.PERMISSION_GRANTED) {
            Log.d(TAG, getString(R.string.permission_not_granted));
            // Permission not yet granted. Use requestPermissions().
            // MY_PERMISSIONS_REQUEST_SEND_SMS is an
            // app-defined int constant. The callback method gets the
            // result of the request.
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.RECEIVE_SMS},
                    MY_PERMISSIONS_REQUEST_RECEIVE_SMS);
        }

        if (ActivityCompat.checkSelfPermission(this,
                Manifest.permission.ACCESS_COARSE_LOCATION) !=
                PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, getString(R.string.permission_not_granted));
            // Permission not yet granted. Use requestPermissions().
            // MY_PERMISSIONS_REQUEST_SEND_SMS is an
            // app-defined int constant. The callback method gets the
            // result of the request.
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.ACCESS_COARSE_LOCATION},
                    MY_PERMISSIONS_REQUEST_RECEIVE_LOCATION);
        } else {
            Log.e(TAG,"coarse permission already granted");
        }

        if (ActivityCompat.checkSelfPermission(this,
                Manifest.permission.ACCESS_FINE_LOCATION) !=
                PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "fine permission NOT already granted");
            // Permission not yet granted. Use requestPermissions().
            // MY_PERMISSIONS_REQUEST_SEND_SMS is an
            // app-defined int constant. The callback method gets the
            // result of the request.
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                    MY_PERMISSIONS_REQUEST_RECEIVE_LOCATION_FINE);
        } else {
            Log.e(TAG,"fine permission already granted");
        }

        aboutButton = (Button) findViewById(R.id.about_button);
        aboutButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openAboutActivity();
            }
        });

        errorButton = (Button) findViewById(R.id.error_button);
        errorButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int x = 5/0;
            }
        });

        dictionaryButton = (Button) findViewById(R.id.dictionary_button);
        dictionaryButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openDictionaryActivity();
            }
        });

        ticTacToeButton = (Button) findViewById(R.id.tictactoe_button);
        ticTacToeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openTTTActivity();
            }
        });

        altAssign1Button = (Button) findViewById(R.id.alt_assign_1_button);
        altAssign1Button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openAltAssign1Activity();
            }
        });

        mapDisplayButton = (Button) findViewById(R.id.map_display_button);
        mapDisplayButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openMapDisplayActivity();
            }
        });
    }

    public void openAboutActivity(){
        Intent intent = new Intent(this, About.class);
        startActivity(intent);
    }

    public void openDictionaryActivity(){
        Intent intent = new Intent(this, Dictionary.class);
        startActivity(intent);
    }

    public void openTTTActivity(){
        Intent intent = new Intent(this, MultiplayerTTTMain.class);
        startActivity(intent);
    }

    public void openAltAssign1Activity(){
        Intent intent = new Intent(this, AltAssign1Main.class);
        startActivity(intent);
    }

    public void openMapDisplayActivity(){
        Intent intent = new Intent(this, MapDisplay.class);
        startActivity(intent);
    }

    private void createNotificationChannel() {
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = getString(R.string.channel_name);
            String description = getString(R.string.channel_description);
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(CHANNEL_NAME, name, importance);
            channel.setDescription(description);
            // Register the channel with the system; you can't change the importance
            // or other notification behaviors after this
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

}