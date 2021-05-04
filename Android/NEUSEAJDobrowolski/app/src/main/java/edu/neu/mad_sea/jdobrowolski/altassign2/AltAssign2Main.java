package edu.neu.mad_sea.jdobrowolski.altassign2;

import android.location.Location;
import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import edu.neu.mad_sea.jdobrowolski.MainActivity;
import edu.neu.mad_sea.jdobrowolski.R;
import edu.neu.mad_sea.jdobrowolski.altassign1.MyBroadcastReceiver;
import edu.neu.mad_sea.jdobrowolski.altassign1.SMSData;

public class AltAssign2Main extends AppCompatActivity implements MyBroadcastReceiver.OnSmsReceivedListener {

    private MyBroadcastReceiver myBroadcastReceiver;

    private static final String TAG = AltAssign2Main.class.getSimpleName();

    private SMSData smsData;

    /**
     * Represents a geographical location.
     */
    protected Location mLastLocation;

    private String mLatitudeLabel;
    private String mLongitudeLabel;
    private TextView mLatitudeText;
    private TextView mLongitudeText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_alt_assign2_main);

        smsData = SMSData.getInstance(this);
    }

    public void setFields(String sender, String message) {
    }

    @Override
    public void onSmsReceived(String sender, String message) {
    }

    @Override
    protected void onResume() {
        super.onResume();
        //registerReceiver(myBroadcastReceiver, new IntentFilter("android.provider.Telephony.SMS_RECEIVED"));
    }

    @Override
    protected void onPause() {
        super.onPause();
        //unregisterReceiver(myBroadcastReceiver);
    }
}