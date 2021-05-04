package edu.neu.mad_sea.jdobrowolski.dictionary;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatDialogFragment;

public class Acknowledgments extends AppCompatDialogFragment {
    private String message =
            "https://stackoverflow.com/a/46709914 to hide the top bar\n\n"
            + "https://www.youtube.com/watch?v=Bsm-BlXo2SI to write the dialag code\n\n"
            + "Other understanding came from class materials and Google searches, often leading " +
                    "to StackOverflow or GeeksForGeeks."
            + "General TST logic learned from algo book code from Robert Sedgewick's Algortithms" +
                    "4th edition\n\n"
            + "I figured out much of the visual portions by playing around with the GUI in" +
                    "Android Studio, but I'm sure there are parts I don't understand properly.\n\n"
            + "I would also like to *acknowledge* the disorganization of some parts... my excuse " +
                    "is that I was away from home and didn't have as much time to focus on making " +
                    "things as clean as I'd like and focus more on understanding the parts. " +
                    "Future projects will be more organized now that I have more understanding.";

    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setTitle("Acknowledgments");

        builder.setMessage(message);
        builder.setPositiveButton("ok", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {

            }
        });
        return builder.create();
    }
}