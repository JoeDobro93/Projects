package edu.neu.mad_sea.jdobrowolski.altassign2;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;

import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.MarkerOptions;

import java.util.HashMap;
import java.util.Map;

import edu.neu.mad_sea.jdobrowolski.R;
import edu.neu.mad_sea.jdobrowolski.altassign1.SMSData;

public class MapDisplay extends AppCompatActivity implements OnMapReadyCallback {

    SupportMapFragment mapFragment;

    SMSData smsData;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_map_display);

        mapFragment = (SupportMapFragment) getSupportFragmentManager()
                .findFragmentById(R.id.map);

        assert mapFragment != null;
        mapFragment.getMapAsync(this);

        smsData = SMSData.getInstance(this);

    }

    @Override
    public void onMapReady(GoogleMap googleMap) {
        googleMap.setMapType(GoogleMap.MAP_TYPE_NORMAL);


        HashMap<Integer, Double[]> locations;

        locations = smsData.getLocations();

        for (Map.Entry e : locations.entrySet()) {
            Double[] coord = (Double[]) e.getValue();
            int key = (int) e.getKey();

            String str = key + " " + coord;

            Log.e("MapDisplay", str);

            googleMap.addMarker(new MarkerOptions()
                    .position(new LatLng(coord[0], coord[1]))
                    .title("Key: " + key));
        }


    }


}