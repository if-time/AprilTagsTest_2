package com.example.apriltagstest_2;

import android.Manifest;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.preference.PreferenceManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private static final String       TAG = "AprilTag";
    private              Camera       camera;
    private              AprilTagView apriltagView;

    private static final int MY_PERMISSIONS_REQUEST_CAMERA = 77;
    private              int has_camera_permissions        = 0;

    private void verifyPreferences() {
        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);

        int nthreads = Integer.parseInt(sharedPreferences.getString("nthreads_value", "0"));
        if (nthreads <= 0) {
            int nproc = Runtime.getRuntime().availableProcessors();
            if (nproc <= 0) {
                nproc = 1;
            }
            Log.i(TAG, "available processors: " + nproc);
            PreferenceManager.getDefaultSharedPreferences(this).edit().putString("nthreads_value", Integer.toString(nproc)).apply();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA},
                    MY_PERMISSIONS_REQUEST_CAMERA);
        } else {
            this.has_camera_permissions = 1;
        }

        apriltagView = new AprilTagView(this);
        FrameLayout layout = (FrameLayout) findViewById(R.id.tag_view);

        layout.addView(apriltagView);

        // Add toolbar/actionbar
        Toolbar myToolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(myToolbar);

        // Make the screen stay awake
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    /**
     * Release the camera when application focus is lost
     */
    protected void onPause() {
        super.onPause();
        apriltagView.onPause();

        if (camera != null) {
            apriltagView.setCamera(null);
            camera.release();
            camera = null;
        }
    }

    /**
     * (Re-)initialize the camera
     */
    protected void onResume() {
        super.onResume();
        apriltagView.onResume();

        if (this.has_camera_permissions == 0) {
            return;
        }

        // Re-initialize the Apriltag detector as settings may have changed
        verifyPreferences();
        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        double decimation = Double.parseDouble(sharedPreferences.getString("decimation_list", "1"));
        double sigma = Double.parseDouble(sharedPreferences.getString("sigma_value", "0"));
        int nthreads = Integer.parseInt(sharedPreferences.getString("nthreads_value", "0"));
        String tagFamily = sharedPreferences.getString("tag_family_list", "tag36h11");
        boolean useRear = (sharedPreferences.getString("device_settings_camera_facing", "1").equals("1")) ? true : false;
        Log.i(TAG, String.format("decimation: %f | sigma: %f | nthreads: %d | tagFamily: %s | useRear: %b",
                decimation, sigma, nthreads, tagFamily, useRear));
        ApriltagNative.apriltag_init(tagFamily, 2, decimation, sigma, nthreads);

        // Find the camera index of front or rear camera
        int camidx = 0;
        Camera.CameraInfo info = new Camera.CameraInfo();
        for (int i = 0; i < Camera.getNumberOfCameras(); i += 1) {
            Camera.getCameraInfo(i, info);
            int desiredFacing = useRear ? Camera.CameraInfo.CAMERA_FACING_BACK :
                    Camera.CameraInfo.CAMERA_FACING_FRONT;
            if (info.facing == desiredFacing) {
                camidx = i;
                break;
            }
        }

        Camera.getCameraInfo(camidx, info);
        Log.i(TAG, "using camera " + camidx);
        Log.i(TAG, "camera rotation: " + info.orientation);

        try {
            camera = Camera.open(camidx);
        } catch (Exception e) {
            Log.d(TAG, "Couldn't open camera: " + e.getMessage());
            return;
        }

        Log.i(TAG, "supported resolutions:");
        Camera.Parameters params = camera.getParameters();
        for (Camera.Size s : params.getSupportedPreviewSizes()) {
            Log.i(TAG, " " + s.width + "x" + s.height);
        }

        apriltagView.setCamera(camera);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.settings:
                verifyPreferences();
                Intent intent = new Intent();
                intent.setClassName(this, "com.example.apriltagstest_2.SettingsActivity");
                startActivity(intent);
                return true;
            case R.id.reset:
                // Reset all shared preferences to default values
                PreferenceManager.getDefaultSharedPreferences(this).edit().clear().commit();

                // Restart the camera preview
                onPause();
                onResume();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case MY_PERMISSIONS_REQUEST_CAMERA: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.i(TAG, "App GRANTED camera permissions");

                    // Set flag
                    this.has_camera_permissions = 1;

                    // Restart the TagViewer
                    apriltagView = new AprilTagView(this);
                    FrameLayout layout = (FrameLayout) findViewById(R.id.tag_view);
                    layout.addView(apriltagView);

                    // Restart the camera
                    onPause();
                    onResume();
                } else {
                    Log.i(TAG, "App DENIED camera permissions");
                    this.has_camera_permissions = 0;
                }
                return;
            }
        }
    }
}
