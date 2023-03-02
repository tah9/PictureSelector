package com.school.demo2_23;

import android.content.Context;
import android.util.DisplayMetrics;
import android.view.WindowManager;

/**
 * ->  tah9  2023/2/27 12:56
 */
public class Tools {
    public static int getWidth(Context context) {
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics dm = new DisplayMetrics();
        wm.getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;// 屏幕宽度（像素）
        int height = dm.heightPixels; // 屏幕高度（像素）
        int[] wh = new int[2];
        wh[0] = width;
        wh[1] = height;
        return width;
    }
}
