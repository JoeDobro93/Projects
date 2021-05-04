package edu.neu.mad_sea.jdobrowolski.altassign1;

import androidx.appcompat.app.AppCompatActivity;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import org.w3c.dom.Text;

import edu.neu.mad_sea.jdobrowolski.altassign1.MyBroadcastReceiver;

import edu.neu.mad_sea.jdobrowolski.R;

public class AltAssign1Main extends AppCompatActivity implements MyBroadcastReceiver.OnSmsReceivedListener {

    private MyBroadcastReceiver myBroadcastReceiver;

    private static final String TAG = "AltAssignmentApp";

    private Button queryButton;
    private Button clearButton;
    private TextView dataDisplay;
    private SMSData smsData;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_alt_assign1_main);

        smsData = SMSData.getInstance(this);

        dataDisplay = (TextView) findViewById(R.id.data_display);
        dataDisplay.setMovementMethod(new ScrollingMovementMethod());
        dataDisplay.setText(smsData.querySms());

        queryButton = (Button) findViewById(R.id.query_button);
        queryButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dataDisplay.setText(smsData.querySms());
            }
        });

        clearButton = (Button) findViewById(R.id.clear_data_button);
        clearButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                smsData.clearDatabase();
            }
        });
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