package com.example.apriltagstest_2;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class AprilTagView extends GLSurfaceView implements Camera.PreviewCallback {

    private              boolean                      isPrint;
    private static final String                       TAG             = "AprilTag";
    private              Camera                       mCamera;
    private              Camera.Size                  mPreviewSize;
    private              List<Camera.Size>            mSupportedPreviewSizes;
    private              ByteBuffer                   mYuvBuffer;
    private              SurfaceTexture               mSurfaceTexture = new SurfaceTexture(0);
    private              Renderer                     mRenderer;
    private              ArrayList<ApriltagDetection> mDetections;

    public AprilTagView(Context context) {
        super(context);
    }

    public void setCamera(Camera camera)
    {
        if (camera == mCamera)
            return;
        // Stop the previous camera preview
        if (mCamera != null) {
            try {
                mCamera.stopPreview();
                //Log.i(TAG, "Camera stop");
            } catch (Exception e) { }
        }

        // Start the new mCamera preview
        if (camera != null) {
            setHighestCameraPreviewResolution(camera);

            // Ensure space for frame (12 bits per pixel)
            mPreviewSize = camera.getParameters().getPreviewSize();
            int nbytes = mPreviewSize.width * mPreviewSize.height * 3 / 2; // XXX: What's the 3/2 scaling for?
            if (mYuvBuffer == null || mYuvBuffer.capacity() < nbytes) {
                // Allocate direct byte buffer so native code access won't require a copy
                Log.i(TAG, "Allocating buf of mPreviewSize " + nbytes);
                mYuvBuffer = ByteBuffer.allocateDirect(nbytes);
            }

            // Scale the rectangle on which the image texture is drawn
            // Here, the image is displayed without rescaling
            Matrix.setIdentityM(mRenderer.M, 0);
            Matrix.scaleM(mRenderer.M, 0, mPreviewSize.height, mPreviewSize.width, 1.0f);

            camera.addCallbackBuffer(mYuvBuffer.array());
            try {
                // Give the mCamera an off-screen GL texture to render on
                camera.setPreviewTexture(mSurfaceTexture);
            } catch (IOException e) {
                Log.d(TAG, "Couldn't set preview display");
                return;
            }
            camera.setPreviewCallbackWithBuffer(this);
            camera.startPreview();
            //Log.i(TAG, "Camera start");
        }
        mCamera = camera;
    }

    private void setHighestCameraPreviewResolution(Camera camera)
    {
        Camera.Parameters parameters = camera.getParameters();
        List<Camera.Size> sizeList = camera.getParameters().getSupportedPreviewSizes();

        Camera.Size bestSize = sizeList.get(0);
        for (int i = 1; i < sizeList.size(); i++){
            if ((sizeList.get(i).width * sizeList.get(i).height) > (bestSize.width * bestSize.height)){
                bestSize = sizeList.get(i);
            }
        }

        parameters.setPreviewSize(bestSize.width, bestSize.height);
        Log.i(TAG, "Setting " + bestSize.width + " x " + bestSize.height);

        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
        camera.setParameters(parameters);
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {

    }
}
