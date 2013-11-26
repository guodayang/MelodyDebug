package com.tencent.debugwebview.tools;

import java.text.SimpleDateFormat;
import java.util.Locale;

import android.util.Log;

public class MLogger {
    private static final String TAG = MConstants.APP_NAME;

    /**
     * get current time
     * 
     * @return
     */
    private static String getTime() {
        return new SimpleDateFormat("yyyy-MM-dd HH:mm:ss:ms", Locale.CHINA).format(new java.util.Date(System
                .currentTimeMillis()));
    }

    /**
     * call system function to show log
     * 
     * @param message
     */
    public static void v(String message) {
        Log.v(TAG, message);
    }

    /**
     * call system function to show log, while user can choose to show the
     * current time
     * 
     * @param message
     * @param isWithTime
     */
    public static void v(String message, boolean isWithTime) {
        if (isWithTime)
            Log.v(TAG, message + ", " + getTime());
        else
            Log.v(TAG, message);
    }

    /**
     * call system function to show log
     * 
     * @param message
     */
    public static void d(String message) {
        Log.d(TAG, message);
    }

    /**
     * call system function to show log, while user can choose to show the
     * current time
     * 
     * @param message
     * @param isWithTime
     */
    public static void d(String message, boolean isWithTime) {
        if (isWithTime)
            Log.d(TAG, message + ", " + getTime());
        else
            Log.d(TAG, message);
    }

    /**
     * call system function to show log
     * 
     * @param message
     */
    public static void i(String message) {
        Log.i(TAG, message);
    }

    /**
     * call system function to show log, while user can choose to show the
     * current time
     * 
     * @param message
     * @param isWithTime
     */
    public static void i(String message, boolean isWithTime) {
        if (isWithTime)
            Log.i(TAG, message + ", " + getTime());
        else
            Log.i(TAG, message);
    }

    /**
     * call system function to show log
     * 
     * @param message
     */
    public static void w(String message) {
        Log.w(TAG, message);
    }

    /**
     * call system function to show log, while user can choose to show the
     * current time
     * 
     * @param message
     * @param isWithTime
     */
    public static void w(String message, boolean isWithTime) {
        if (isWithTime)
            Log.w(TAG, message + ", " + getTime());
        else
            Log.w(TAG, message);
    }

    /**
     * call system function to show log
     * 
     * @param message
     */
    public static void e(String message) {
        Log.e(TAG, message);
    }

    /**
     * call system function to show log, while user can choose to show the
     * current time
     * 
     * @param message
     * @param isWithTime
     */
    public static void e(String message, boolean isWithTime) {
        if (isWithTime)
            Log.e(TAG, message + ", " + getTime());
        else
            Log.e(TAG, message);
    }
}
