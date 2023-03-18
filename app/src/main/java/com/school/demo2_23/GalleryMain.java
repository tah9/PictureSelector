package com.school.demo2_23;

import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.school.demo2_23.adapter.TestAdapter;

import java.util.ArrayList;

public class GalleryMain extends AppCompatActivity {
    static {
        System.loadLibrary("my-native");
    }

    public Context context = this;
    private static final String TAG = "GalleryMain";
    private RecyclerView recyclerView;
    private TestAdapter picAdapter;
    private String rootPath = Environment.getExternalStorageDirectory().getAbsolutePath();

    ArrayList<PcPathBean> imgPaths = new ArrayList<>();
    ArrayList<PcPathBean> conPaths = new ArrayList<>();
    private long stime;
    private GridLayoutManager gridLayoutManager;

    public native void native_scan(String rootPath);


    int flag = 0;

    //native回调
    public void nativeCallback(ArrayList<PcPathBean> nativeList) {
        imgPaths.addAll(nativeList);
        if (picAdapter == null) {
            return;
        }
        Log.d(TAG, "nativeCallback: "+System.currentTimeMillis());
        runOnUiThread(() -> {
//            setTitle("" + imgPaths.size());
            picAdapter.notifyItemRangeInserted(
                    imgPaths.size() - nativeList.size(), nativeList.size());
            picAdapter.notifyItemRangeChanged(
                    imgPaths.size() - nativeList.size(), nativeList.size());
        });
    }

    void expendTime() {
        // 结束时间
        long etime = System.currentTimeMillis();
        // 计算执行时间
        Log.d(TAG, "消耗时间: " + (etime - stime));
    }

    public void instanceNative_finish() {
        Log.d(TAG, "instanceNative_finish ");
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.gallery_main);
        Log.d(TAG, "onCreate: " + System.currentTimeMillis());
        //native后台扫描线程
        new Thread(() -> {
            native_scan(rootPath);
        }).start();


        initView();


    }

    ArrayList<PcDirBean> imgFolders = new ArrayList<>();

    private void initView() {
//        findViewById(R.id.btn).setOnClickListener(v -> {
//            //app重新启动
//            Intent intent = getPackageManager()
//                    .getLaunchIntentForPackage(getApplication().getPackageName());
//            PendingIntent restartIntent = PendingIntent.getActivity(getApplicationContext(), 0, intent, 0);
//            AlarmManager mgr = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
//            mgr.set(AlarmManager.RTC, 0, restartIntent); // 0秒钟后重启应用
//            System.exit(0);
//        });

        recyclerView = findViewById(R.id.recyclerView);
        recyclerView.setHasFixedSize(true);
        recyclerView.setItemAnimator(null);
        gridLayoutManager = new GridLayoutManager(context, 4);
        recyclerView.setLayoutManager(gridLayoutManager);
        picAdapter = new TestAdapter(imgPaths, context, rootPath);
        recyclerView.setAdapter(picAdapter);
        recyclerView.setItemViewCacheSize(30);
    }
}