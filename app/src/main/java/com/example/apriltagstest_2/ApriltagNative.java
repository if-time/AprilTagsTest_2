package com.example.apriltagstest_2;

import android.graphics.Bitmap;

import java.util.ArrayList;

/**
 * Interface to native C AprilTag library.
 */

public class ApriltagNative {
    static {
        System.loadLibrary("native-lib");
        native_init();
    }

    public static native void native_init();

    public static native void yuv_to_rgb(byte[] src, int width, int height, Bitmap dst);

//    public static native void apriltag_init(String tagFamily, int errorBits, double decimateFactor,
//                                            double blurSigma, int nthreads);
public static native void apriltag_init(String tagFamily, int errorBits, double decimateFactor,
                                        double blurSigma, int nthreads,
                                        double tagsize, double fx, double fy, double cx, double cy, int id);

//    public static native ArrayList<ApriltagDetection> apriltag_detect_yuv(byte[] src, int width, int height);

    public static native ArrayList<ApriltagPose> apriltag_detect_yuv(byte[] src, int width, int height);
}
