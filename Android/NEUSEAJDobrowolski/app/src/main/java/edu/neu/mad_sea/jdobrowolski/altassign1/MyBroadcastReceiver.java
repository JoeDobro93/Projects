package edu.neu.mad_sea.jdobrowolski.altassign1;

import android.Manifest;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.provider.ContactsContract;
import android.telephony.SmsMessage;
import android.util.Log;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.tasks.OnSuccessListener;

import edu.neu.mad_sea.jdobrowolski.MainActivity;

/* https://google-developer-training.github.io/android-developer-phone-sms-course/Lesson%202/2_p_2_sending_sms_messages.html
    most of the code started from here
 */
public class MyBroadcastReceiver extends BroadcastReceiver {
    private static final String TAG =
            MyBroadcastReceiver.class.getSimpleName();
    public static final String pdu_type = "pdus";

    // https://stackoverflow.com/a/26268569
    private OnSmsReceivedListener listener = null;

    public interface OnSmsReceivedListener {
        public void onSmsReceived(String sender, String message);
    }

    public void setOnSmsReceivedListener(Context context) {
        this.listener = (OnSmsReceivedListener) context;
    }

    /**
     * I borrowed much of this code from the google-developer-training website and there wasn't much
     * that I felt good about changing in that code, however I was able to add the functionality I
     * needed as well
     */
    @Override
    public void onReceive(Context context, Intent intent) {
        // Get the SMS message.
        Bundle bundle = intent.getExtras();
        SmsMessage[] msgs;
        String format = bundle.getString("format");
        Log.d(TAG, "onReceive called");
        SMSData smsData = SMSData.getInstance(context);
        LocationManager locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        FusedLocationProviderClient client;

        double latitude = 0;
        double longitude = 0;

        if (ActivityCompat.checkSelfPermission(
                context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(
                context, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "no location permission");
            //ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, REQUEST_LOCATIOM);
        } else {
            Location location = locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
            if (location != null) {
                latitude = location.getLatitude();
                longitude = location.getLongitude();
            } else {
                Log.e(TAG, "location result has no result");
            }
        }


        // Retrieve the SMS message received.
        Object[] pdus = (Object[]) bundle.get(pdu_type);
        if (pdus != null) {
            Log.e(TAG, "non null message");
            // Check the Android version.
            boolean isVersionM =
                    (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M);
            // Fill the msgs array.
            msgs = new SmsMessage[pdus.length];
            for (int i = 0; i < msgs.length; i++) {
                // Check Android version and use appropriate createFromPdu.
                if (isVersionM) {
                    // If Android version M or newer:
                    msgs[i] = SmsMessage.createFromPdu((byte[]) pdus[i], format);
                } else {
                    // If Android version L or older:
                    msgs[i] = SmsMessage.createFromPdu((byte[]) pdus[i]);
                }

                smsData.addSMS(new SMSMessage(msgs[i].getOriginatingAddress(), msgs[i].getMessageBody(), latitude, longitude));


                // This didn't get used, but left it in case it will be useful later
                if (listener != null) {
                    listener.onSmsReceived(msgs[i].getOriginatingAddress(), msgs[i].getMessageBody());
                }
            }
        }
    }
}
