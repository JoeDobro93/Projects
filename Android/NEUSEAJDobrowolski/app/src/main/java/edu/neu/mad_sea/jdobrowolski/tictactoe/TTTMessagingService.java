package edu.neu.mad_sea.jdobrowolski.tictactoe;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.NonNull;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import edu.neu.mad_sea.jdobrowolski.R;

public class TTTMessagingService extends FirebaseMessagingService {
    private static final String TAG = TTTMessagingService.class.getSimpleName();

    @Override
    public void onMessageReceived(RemoteMessage remoteMessage) {
        //Log.d(TAG, "From: ", remoteMessage.getFrom());

        if (remoteMessage.getData().size() > 0) {
            Log.d(TAG, "Message Notification Body: " + remoteMessage.getNotification());
        }

        if (remoteMessage.getNotification() != null) {
            Log.d(TAG, "Message Notification Body: " + remoteMessage.getNotification().getBody());

            sendNotification(remoteMessage.getNotification().getBody());
        }
    }

    public void sendNotification(String messageBody) {
        Intent intent = new Intent(this, MultiplayerTTTMain.class);
        PendingIntent pIntent = PendingIntent.getActivity(this, (int)System.currentTimeMillis(),
                intent, 0);

        Notification notification = new Notification.Builder(this).setContentTitle("Move made!")
                .setSmallIcon(R.drawable.ic_foo)
                .setContentText(messageBody)
                .setContentIntent(pIntent)
                .build();

        NotificationManager notificationManager =
                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

        notification.flags |= Notification.FLAG_AUTO_CANCEL;

        notificationManager.notify(0, notification);
    }

}
