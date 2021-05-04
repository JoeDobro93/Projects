package edu.neu.mad_sea.jdobrowolski.dictionary;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatDialogFragment;


/**
 * Creates error dialog box to tell user what may have gone wrong.  I only really found enough info
 * to get this to work, so I don't fully understand the DialogFragments in depth nor do I understand
 * why IJ added Nullable/NonNull when I began writing this code.
 */
public class ErrorDialog extends AppCompatDialogFragment {
    // local param to hold the message.  Default to blank string
    private String message = "";

    public void setMessage(String message) {
        this.message = message;
    }

    /**
     * Don't fully understand this, I liked in my Acknowledgement page to the video I referenced to
     * create this, but I don't entirely understand what a Bundle is yet.
     *
     */
    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setTitle("Warning!");
        builder.setMessage(message);
        builder.setPositiveButton("ok", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {

            }
        });
        return builder.create();
    }
}
