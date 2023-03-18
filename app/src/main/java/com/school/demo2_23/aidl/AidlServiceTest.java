package com.school.demo2_23.aidl;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import androidx.annotation.Nullable;

import com.school.demo2_23.GalleryMain;
import com.school.demo2_23.IMyAidlInterface;
import com.school.demo2_23.PcPathBean;

/**
 * ->  tah9  2023/3/18 19:33
 */
public class AidlServiceTest extends Service {
    private static final String TAG = "AidlTest";

    public AidlServiceTest() {
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return messageSender;
    }

    IBinder messageSender=new IMyAidlInterface.Stub() {
        @Override
        public void sendMessage(PcPathBean pic) throws RemoteException {
            Log.d(TAG, "sendMessage: " + pic.toString());
            Log.d(TAG, "sendMessage: 收到消息");
            startActivity(new Intent(getApplicationContext(), GalleryMain.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        }
    };
}
