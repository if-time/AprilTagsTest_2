#include <jni.h>

#include <android/bitmap.h>
#include <android/log.h>
#include "common/matd.h"

#include "apriltag.h"
#include "apriltag_pose.h"
#include "tag36h11.h"
#include "tag25h9.h"
#include "tag16h5.h"

static struct {
    // 初始化
    apriltag_detector_t *td;
    apriltag_family_t *tf;
    void(*tf_destroy)(apriltag_family_t*);

    // 姿态估计
    apriltag_detection_info_t info;
    apriltag_pose_t pose;

    jint rec_id;

    jclass al_cls;
    jmethodID al_constructor, al_add;

    jclass ad_cls;
    jmethodID ad_constructor;

    jclass am_cls;
    jmethodID am_constructor;

    jclass ap_cls;
    jmethodID ap_constructor;

    jfieldID ad_id_field, ad_hamming_field, ad_c_field, ad_p_field, am_nrows_field, am_ncols_field,
            am_data_field, ap_r_field, ap_name_field, ap_t_field, ap_apd_field;
} state;

JNIEXPORT void JNICALL Java_com_example_apriltagstest_12_ApriltagNative_native_1init
    (JNIEnv *env, jclass cls)
{
    // Just do method lookups once and cache the results
    // Get ArrayList methods
    jclass al_cls = (*env)->FindClass(env, "java/util/ArrayList");
    if (!al_cls) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ArrayList class");
        return;
    }
    state.al_cls = (*env)->NewGlobalRef(env, al_cls);

    state.al_constructor = (*env)->GetMethodID(env, al_cls, "<init>", "()V");
    state.al_add = (*env)->GetMethodID(env, al_cls, "add", "(Ljava/lang/Object;)Z");
    if (!state.al_constructor || !state.al_add) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ArrayList methods");
        return;
    }

    // Get ApriltagDetection methods
    jclass ad_cls = (*env)->FindClass(env, "com/example/apriltagstest_2/ApriltagDetection");
    if (!ad_cls) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagDetection class");
        return;
    }
    state.ad_cls = (*env)->NewGlobalRef(env, ad_cls);

    state.ad_constructor = (*env)->GetMethodID(env, ad_cls, "<init>", "()V");
    if (!state.ad_constructor) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagDetection constructor");
        return;
    }

    jclass am_cls = (*env)->FindClass(env, "com/example/apriltagstest_2/ApriltagMatd");
    if (!am_cls) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagMatd class");
        return;
    }
    state.am_cls = (*env)->NewGlobalRef(env, am_cls);

    state.am_constructor = (*env)->GetMethodID(env, am_cls, "<init>", "()V");
    if (!state.am_constructor) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagMatd constructor");
        return;
    }
    jclass ap_cls = (*env)->FindClass(env, "com/example/apriltagstest_2/ApriltagPose");
    if (!ap_cls) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagPose class");
        return;
    }
    state.ap_cls = (*env)->NewGlobalRef(env, ap_cls);

    state.ap_constructor = (*env)->GetMethodID(env, ap_cls, "<init>", "()V");
    if (!state.ap_constructor) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagPose constructor");
        return;
    }
    state.ad_id_field = (*env)->GetFieldID(env, ad_cls, "id", "I");
    state.ad_hamming_field = (*env)->GetFieldID(env, ad_cls, "hamming", "I");
    state.ad_c_field = (*env)->GetFieldID(env, ad_cls, "c", "[D");
    state.ad_p_field = (*env)->GetFieldID(env, ad_cls, "p", "[D");
    if (!state.ad_id_field ||
            !state.ad_hamming_field ||
            !state.ad_c_field ||
            !state.ad_p_field) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagDetection fields");
        return;
    }

    state.am_nrows_field = (*env)->GetFieldID(env, am_cls, "nrows", "I");
    state.am_ncols_field = (*env)->GetFieldID(env, am_cls, "ncols", "I");
    state.am_data_field = (*env)->GetFieldID(env, am_cls, "data", "[D");
    if (!state.am_nrows_field ||
        !state.am_ncols_field ||
        !state.am_data_field) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagMatd fields");
        return;
    }
    //state.ap_name_field = (*env)->GetFieldID(env, ap_cls, "name", "Ljava/lang/String;");
    state.ap_r_field = (*env)->GetFieldID(env, ap_cls, "R", "Lcom/example/apriltagstest_2/ApriltagMatd;");
    state.ap_t_field = (*env)->GetFieldID(env, ap_cls, "t", "Lcom/example/apriltagstest_2/ApriltagMatd;");
    state.ap_apd_field = (*env)->GetFieldID(env, ap_cls, "apd", "Lcom/example/apriltagstest_2/ApriltagDetection;");
    if (!state.ap_r_field ||
        !state.ap_t_field ||
        !state.ap_apd_field) {
        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
                            "couldn't find ApriltagPose fields");
        return;
    }
}

///*
// * Class:     edu_umich_eecs_april_apriltag_ApriltagNative
// * Method:    yuv_to_rgb
// * Signature: ([BIILandroid/graphics/Bitmap;)V
// */
//JNIEXPORT void JNICALL Java_com_example_apriltagstest_12_ApriltagNative_yuv_1to_1rgb
//    (JNIEnv *env, jclass cls, jbyteArray _src, jint width, jint height, jobject _dst)
//{
//    // NV21 Format
//    // width*height    luma (Y) bytes followed by
//    // width*height/2  chroma (UV) bytes interleaved as V,U
//
//    jbyte *src = (*env)->GetByteArrayElements(env, _src, NULL);
//    jint *dst = NULL;
//    AndroidBitmap_lockPixels(env, _dst, &dst);
//
//    if (!dst) {
//        __android_log_write(ANDROID_LOG_ERROR, "apriltag_jni",
//                            "couldn't lock bitmap");
//        return;
//    }
//
//    AndroidBitmapInfo bmpinfo;
//    if (AndroidBitmap_getInfo(env, _dst, &bmpinfo) ||
//        bmpinfo.width*bmpinfo.height != width*height ||
//        bmpinfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
//        __android_log_print(ANDROID_LOG_ERROR, "apriltag_jni",
//                            "incorrect bitmap format: %d x %d  %d",
//                            bmpinfo.width, bmpinfo.height, bmpinfo.format);
//        return;
//    }
//
//    int uvstart = width * height;
//    for (int j = 0; j < height; j += 1) {
//        for (int i = 0; i < width; i += 1) {
//            int y = (unsigned char)src[j*width + i];
//            int offset = uvstart + (j >> 1)*width + (i & ~1);
//            int u = (unsigned char)src[offset + 1];
//            int v = (unsigned char)src[offset + 0];
//
//            y = y < 16 ? 16 : y;
//
//            int a0 = 1192 * (y - 16);
//            int a1 = 1634 * (v - 128);
//            int a2 = 832 * (v - 128);
//            int a3 = 400 * (u - 128);
//            int a4 = 2066 * (u - 128);
//
//            int r = (a0 + a1) >> 10;
//            int g = (a0 - a2 - a3) >> 10;
//            int b = (a0 + a4) >> 10;
//
//            r = r < 0 ? 0 : (r > 255 ? 255 : r);
//            g = g < 0 ? 0 : (g > 255 ? 255 : g);
//            b = b < 0 ? 0 : (b > 255 ? 255 : b);
//
//            // Output image in portrait orientation
//            dst[(i+1)*height - j-1] = 0xff000000 | (r << 16) | (g << 8) | b;
//        }
//    }
//
//    (*env)->ReleaseByteArrayElements(env, _src, src, 0);
//    AndroidBitmap_unlockPixels(env, _dst);
//}

/*
 * Class:     edu_umich_eecs_april_apriltag_ApriltagNative
 * Method:    apriltag_init
 * Signature: (Ljava/lang/String;IDDI)V
 */
JNIEXPORT void JNICALL Java_com_example_apriltagstest_12_ApriltagNative_apriltag_1init
        (JNIEnv *env, jclass cls, jstring _tfname, jint errorbits, jdouble decimate,
         jdouble sigma, jint nthreads, jdouble tagsize, jdouble fx, jdouble fy, jdouble cx, jdouble cy, jint id) {
    __android_log_write(ANDROID_LOG_INFO, "apriltag_jni",
                        "enter_detect_jni1");
    state.rec_id = id;
    state.info.tagsize = tagsize;
    state.info.fx = fx;
    state.info.fy = fy;
    state.info.cx = cx;
    state.info.cy = cy;
    __android_log_write(ANDROID_LOG_INFO, "apriltag_jni",
                        "enter_detect_jni2");
    // Do cleanup in case we're already initialized
    if (state.td) {
        apriltag_detector_destroy(state.td);
        state.td = NULL;
    }
    if (state.tf) {
        state.tf_destroy(state.tf);
        state.tf = NULL;
    }
    __android_log_write(ANDROID_LOG_INFO, "apriltag_jni",
                        "enter_detect_jni3");
    // Initialize state
    const char *tfname = (*env)->GetStringUTFChars(env, _tfname, NULL);

    if (!strcmp(tfname, "tag36h11")) {
        state.tf = tag36h11_create();
        state.tf_destroy = tag36h11_destroy;
    } else if (!strcmp(tfname, "tag36h10")) {

    } else if (!strcmp(tfname, "tag36artoolkit")) {

    } else if (!strcmp(tfname, "tag25h9")) {
        state.tf = tag25h9_create();
        state.tf_destroy = tag25h9_destroy;
    } else if (!strcmp(tfname, "tag25h7")) {

    } else if (!strcmp(tfname, "tag16h5")) {
        state.tf = tag16h5_create();
        state.tf_destroy = tag16h5_destroy;
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "apriltag_jni",
                            "invalid tag family: %s", tfname);
        (*env)->ReleaseStringUTFChars(env, _tfname, tfname);
        return;
    }
    (*env)->ReleaseStringUTFChars(env, _tfname, tfname);

    state.td = apriltag_detector_create();
    apriltag_detector_add_family_bits(state.td, state.tf, errorbits);
    state.td->quad_decimate = decimate;
    state.td->quad_sigma = sigma;
    state.td->nthreads = nthreads;
}

/*
 * Class:     edu_umich_eecs_april_apriltag_ApriltagNative
 * Method:    apriltag_detect_yuv
 * Signature: ([BII)Ljava/util/ArrayList;
 */
//JNIEXPORT jobject JNICALL Java_edu_umich_eecs_april_apriltag_ApriltagNative_apriltag_1detect_1yuv
JNIEXPORT jobject JNICALL Java_com_example_apriltagstest_12_ApriltagNative_apriltag_1detect_1yuv
        (JNIEnv *env, jclass cls, jbyteArray _buf, jint width, jint height) {
    // If not initialized, init with default settings
    if (!state.td) {
        state.tf = tag36h11_create();
        state.td = apriltag_detector_create();
        apriltag_detector_add_family_bits(state.td, state.tf, 2);
        state.td->quad_decimate = 2.0;
        state.td->quad_sigma = 0.0;
        state.td->nthreads = 4;
        __android_log_write(ANDROID_LOG_INFO, "apriltag_jni",
                            "using default parameters");
    }

    // Use the luma channel (the first width*height elements)
    // as grayscale input image
    jbyte *buf = (*env)->GetByteArrayElements(env, _buf, NULL);
    image_u8_t im = {
            .buf = (uint8_t*)buf,
            .height = height,
            .width = width,
            .stride = width
    };
    zarray_t *detections = apriltag_detector_detect(state.td, &im);
    (*env)->ReleaseByteArrayElements(env, _buf, buf, 0);

    // al = new ArrayList();

    jobject al = (*env)->NewObject(env, state.al_cls, state.al_constructor);

    for (int i = 0; i < zarray_size(detections); i += 1) {
        apriltag_detection_t *det;
        zarray_get(detections, i, &det);
        if(det->id == state.rec_id){
            jobject ap = (*env)->NewObject(env, state.ap_cls, state.ap_constructor);
            jobject ad = (*env)->NewObject(env, state.ad_cls, state.ad_constructor);
            jobject am_r = (*env)->NewObject(env, state.am_cls, state.am_constructor);
            jobject am_t = (*env)->NewObject(env, state.am_cls, state.am_constructor);

            state.info.det = det;
            //state.info.cx = det->c[0];
            //state.info.cy = det->c[1];
            double err = estimate_tag_pose(&(state.info), &(state.pose));
            (*env)->SetIntField(env, ad, state.ad_id_field, det->id);
            (*env)->SetIntField(env, ad, state.ad_hamming_field, det->hamming);
            jdoubleArray ad_c = (*env)->GetObjectField(env, ad, state.ad_c_field);
            (*env)->SetDoubleArrayRegion(env, ad_c, 0, 2, det->c);
            jdoubleArray ad_p = (*env)->GetObjectField(env, ad, state.ad_p_field);
            (*env)->SetDoubleArrayRegion(env, ad_p, 0, 8, (double*)det->p);

            jobject ap_apd = (*env)->GetObjectField(env, ap, state.ap_apd_field);
            (*env)->SetObjectField(env, ap_apd, state.ap_apd_field, ad);

            (*env)->SetIntField(env, am_r, state.am_nrows_field, state.pose.R->nrows);
            (*env)->SetIntField(env, am_r, state.am_ncols_field, state.pose.R->ncols);
            jdoubleArray am_r_a = (*env)->GetObjectField(env, am_r, state.am_data_field);
            (*env)->SetDoubleArrayRegion(env, am_r_a, 0, state.pose.R->nrows*state.pose.R->ncols, state.pose.R->data);

            (*env)->SetIntField(env, am_t, state.am_nrows_field, state.pose.t->nrows);
            (*env)->SetIntField(env, am_t, state.am_ncols_field, state.pose.t->ncols);
            jdoubleArray am_t_a = (*env)->GetObjectField(env, am_t, state.am_data_field);
            (*env)->SetDoubleArrayRegion(env, am_t_a, 0, state.pose.t->nrows*state.pose.t->ncols, state.pose.t->data);

            jobject ap_r = (*env)->GetObjectField(env, ap, state.ap_r_field);
            (*env)->SetObjectField(env, ap_r, state.ap_r_field, am_r);
            jobject ap_t = (*env)->GetObjectField(env, ap, state.ap_t_field);
            (*env)->SetObjectField(env, ap_t, state.ap_t_field, am_t);

            (*env)->CallBooleanMethod(env, al, state.al_add, ap);

            (*env)->DeleteLocalRef(env, am_r);
            (*env)->DeleteLocalRef(env, am_r_a);
            (*env)->DeleteLocalRef(env, am_t);
            (*env)->DeleteLocalRef(env, am_t_a);

            (*env)->DeleteLocalRef(env, ad);
            (*env)->DeleteLocalRef(env, ad_c);
            (*env)->DeleteLocalRef(env, ad_p);

            (*env)->DeleteLocalRef(env, ap);
            (*env)->DeleteLocalRef(env, ap_apd);
            (*env)->DeleteLocalRef(env, ap_r);
            (*env)->DeleteLocalRef(env, ap_t);
            break;
        }

          //ad = new_ApriltagDetection();
//        jobject ad = (*env)->NewObject(env, state.ad_cls, state.ad_constructor);

//        (*env)->SetIntField(env, ad, state.ad_id_field, det->id);
//        (*env)->SetIntField(env, ad, state.ad_hamming_field, det->hamming);
//        jdoubleArray ad_c = (*env)->GetObjectField(env, ad, state.ad_c_field);
//        (*env)->SetDoubleArrayRegion(env, ad_c, 0, 2, det->c);
//        jdoubleArray ad_p = (*env)->GetObjectField(env, ad, state.ad_p_field);
//        (*env)->SetDoubleArrayRegion(env, ad_p, 0, 8, (double*)det->p);
//
//        // al.add(ad);
//        (*env)->CallBooleanMethod(env, al, state.al_add, ad);
//
//        // Need to respect the local reference limit
//        (*env)->DeleteLocalRef(env, ad);
//        (*env)->DeleteLocalRef(env, ad_c);
//        (*env)->DeleteLocalRef(env, ad_p);
    }

    // Cleanup
    apriltag_detections_destroy(detections);
    return al;
}
