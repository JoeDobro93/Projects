package edu.neu.mad_sea.jdobrowolski.altassign1;


import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import java.util.HashMap;
import java.util.LinkedList;

// heavily uses https://guides.codepath.com/android/local-databases-with-sqliteopenhelper
public class SMSData extends SQLiteOpenHelper {
    private static SMSData sInstance;

    private static String TAG = "SMSData";

    private static String DB_NAME = "smsdata.db";

    private static int DB_VERSION = 3;

    public static String MESSAGE_TABLE = "messages";

    public static String MESSAGE_ID_COL = "message_id";

    public static String SENDER_COL = "sender";

    public static String BODY_COL = "body";

    public static String LATITUDE_COL = "latitude";

    public static String LONGITUDE_COL = "longitude";

    // Wrap the constructor with values we know relevant to this dbΩ
    private SMSData(Context context) {
        super(context, DB_NAME, null, DB_VERSION);
    }

    public static synchronized SMSData getInstance(Context context) {
        // Use the application context, which will ensure that you
        // don't accidentally leak an Activity's context.
        // See this article for more information: http://bit.ly/6LRzfx
        if (sInstance == null) {
            sInstance = new SMSData(context.getApplicationContext());
        }
        return sInstance;
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_MESSAGE_TABLE = "CREATE TABLE " + MESSAGE_TABLE +
                "(" +
                MESSAGE_ID_COL + " INTEGER PRIMARY KEY," + // Define a primary key
                SENDER_COL + " TEXT,"  + // Define a foreign key
                BODY_COL + " TEXT," +
                LATITUDE_COL + " DOUBLE," +
                LONGITUDE_COL + " DOUBLE" +
                ")";

        db.execSQL(CREATE_MESSAGE_TABLE);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        if (oldVersion != newVersion) {
            // Simplest implementation is to drop all old tables and recreate them
            db.execSQL("DROP TABLE IF EXISTS " + MESSAGE_TABLE);
            onCreate(db);
        }
    }

    // Insert a post into the database
    public void addSMS(SMSMessage smsMessage) {
        // Create and/or open the database for writing
        SQLiteDatabase db = getWritableDatabase();

        // It's a good idea to wrap our insert in a transaction. This helps with performance and ensures
        // consistency of the database.
        db.beginTransaction();
        try {
            ContentValues values = new ContentValues();
            values.put(SENDER_COL, smsMessage.getSender());
            values.put(BODY_COL, smsMessage.getBody());
            values.put(LATITUDE_COL, smsMessage.getLatitude());
            values.put(LONGITUDE_COL, smsMessage.getLongitude());

            // Notice how we haven't specified the primary key. SQLite auto increments the primary key column.
            db.insertOrThrow(MESSAGE_TABLE, null, values);
            db.setTransactionSuccessful();
        } catch (Exception e) {
            Log.d(TAG, "Error while trying to add post to database");
        } finally {
            db.endTransaction();
        }
    }

    public String querySms() {
        String POSTS_SELECT_QUERY =
                String.format("SELECT * FROM %s",
                        MESSAGE_TABLE);

        String result = "";

        SQLiteDatabase db = getReadableDatabase();
        Cursor cursor = db.rawQuery(POSTS_SELECT_QUERY, null);

        try {
            if (cursor.moveToFirst()) {
                do {
                    SMSMessage smsMessage = new SMSMessage(
                            cursor.getInt(cursor.getColumnIndex(MESSAGE_ID_COL)),
                            cursor.getString(cursor.getColumnIndex(SENDER_COL)),
                            cursor.getString(cursor.getColumnIndex(BODY_COL)),
                            cursor.getDouble(cursor.getColumnIndex(LATITUDE_COL)),
                            cursor.getDouble(cursor.getColumnIndex(LONGITUDE_COL))
                    );

                    result += String.format("-----\n" +
                                    "Key      : %d\n" +
                                    "Sender   : %s\n" +
                                    "Body     : %s\n" +
                                    "Latitude : %.6f\n" +
                                    "Longitude: %.6f\n",
                            smsMessage.getKey(),
                            smsMessage.getSender(),
                            smsMessage.getBody(),
                            smsMessage.getLatitude(),
                            smsMessage.getLongitude());
                } while(cursor.moveToNext());
            }
        } catch (Exception e) {
            Log.d(TAG, "Error while trying to get posts from database");
        } finally {
            if (cursor != null && !cursor.isClosed()) {
                cursor.close();
            }
        }

        return result;
    }

    public HashMap<Integer, Double[]> getLocations() {
        HashMap<Integer, Double[]> result = new HashMap<>();

        String COORDINATE_SELECT =
                String.format("SELECT * FROM %s",
                        MESSAGE_TABLE);

        SQLiteDatabase db = getReadableDatabase();
        Cursor cursor = db.rawQuery(COORDINATE_SELECT, null);

        try {
            if (cursor.moveToFirst()) {
                do {
                    SMSMessage smsMessage = new SMSMessage(
                            cursor.getInt(cursor.getColumnIndex(MESSAGE_ID_COL)),
                            cursor.getString(cursor.getColumnIndex(SENDER_COL)),
                            cursor.getString(cursor.getColumnIndex(BODY_COL)),
                            cursor.getDouble(cursor.getColumnIndex(LATITUDE_COL)),
                            cursor.getDouble(cursor.getColumnIndex(LONGITUDE_COL))
                    );

                    result.put(smsMessage.getKey(),
                                new Double[]{smsMessage.getLatitude(),
                                             smsMessage.getLongitude()
                                            }
                               );
                } while(cursor.moveToNext());
            }
        } catch (Exception e) {
            Log.d(TAG, "Error while trying to get lat/long from database");
        } finally {
            if (cursor != null && !cursor.isClosed()) {
                cursor.close();
            }
        }

        return result;
    }

    public void clearDatabase() {
        SQLiteDatabase db = getWritableDatabase();
        db.execSQL("DROP TABLE IF EXISTS " + MESSAGE_TABLE);
        onCreate(db);
    }
}
