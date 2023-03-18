package com.school.demo2_23;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;

import com.school.demo2_23.aidl.AidlServiceTest;

public class MainActivity extends AppCompatActivity {
    public Context context = this;
    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.act_main);
        initView();

    }

    @Override
    protected void onStop() {
        super.onStop();

        Log.d(TAG, "onStop: ");

        try {
            unbindService(serviceConnection);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private void setupService() {
        Intent intent = new Intent(context, AidlServiceTest.class);
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
        startService(intent);
    }


    ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
            Log.d(TAG, "onServiceConnected: ");
            IMyAidlInterface iMyAidlInterface = IMyAidlInterface.Stub.asInterface(iBinder);
            PcPathBean pcPathBean = new PcPathBean();
            pcPathBean.setPath("测试路径");
            pcPathBean.setTime(1324234);
            try {
                Log.d(TAG, "onServiceConnected: " + System.currentTimeMillis());
                iMyAidlInterface.sendMessage(pcPathBean);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            Log.d(TAG, "onServiceDisconnected: ");
        }
    };

    private void initView() {
        findViewById(R.id.btn).setOnClickListener(v -> {
            setupService();
        });
    }
}