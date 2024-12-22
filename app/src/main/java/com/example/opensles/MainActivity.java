package com.example.opensles;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import com.example.opensles.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'opensles' library on application startup.
    static {
        System.loadLibrary("opensles");
    }


    private static final int RECORD_AUDIO_PERMISSION_CODE = 1;

    private ActivityMainBinding binding;

    @Override
    @SuppressLint("MissingInflatedId")
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

         Button startRecording = findViewById(R.id.startRecording);
        Button stopRecording = findViewById(R.id.stopRecording);
        Button startPlaying = findViewById(R.id.startPlaying);
        Button stopPlaying = findViewById(R.id.stopPlaying);

        // Request microphone permission
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO}, RECORD_AUDIO_PERMISSION_CODE);
        }

        startRecording.setOnClickListener(v -> startRecording(7));
        stopRecording.setOnClickListener(v -> stopRecording());
        startPlaying.setOnClickListener(v -> startPlaying());
        stopPlaying.setOnClickListener(v -> stopPlaying());

        // Initialize the audio engine
        initAudioEngine();
    }

    // Declare native methods
    public native void initAudioEngine();
    public native void startRecording(int inputSource);
    public native void stopRecording();
    public native void startPlaying();
    public native void stopPlaying();


}