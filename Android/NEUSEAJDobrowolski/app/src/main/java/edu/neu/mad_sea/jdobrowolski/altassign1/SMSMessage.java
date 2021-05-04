package edu.neu.mad_sea.jdobrowolski.altassign1;

import android.content.IntentFilter;

public class SMSMessage {
    private int key;
    private String sender;
    private String body;
    private double latitude;
    private double longitude;

    public SMSMessage(String sender, String body, double latitude, double longitude) {
        key = -1;
        this.sender = sender;
        this.body = body;
        this.latitude = latitude;
        this.longitude = longitude;
    }

    public SMSMessage(int key, String sender, String body, double latitude, double longitude) {
        this.key = key;
        this.sender = sender;
        this.body = body;
        this.latitude = latitude;
        this.longitude = longitude;
    }

    public void setKey(int key) {
        this.key = key;
    }

    public int getKey() {
        return key;
    }

    public String getSender() {
        return sender;
    }

    public String getBody() {
        return body;
    }

    public double getLatitude() {
        return latitude;
    }

    public double getLongitude() {
        return longitude;
    }
}
