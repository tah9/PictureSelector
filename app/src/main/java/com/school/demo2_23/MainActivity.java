package com.school.demo2_23;

import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.school.demo2_23.adapter.TestAdapter;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("scanner_pic");
    }

    public Context context = this;
    private static final String TAG = "MainActivity";
    private RecyclerView recyclerView;
    private int width;
    private TestAdapter picAdapter;
    private String rootPath = Environment.getExternalStorageDirectory().getAbsolutePath();

    ArrayList<PcPathBean> imgPaths = new ArrayList<>();
    ArrayList<PcPathBean> conPaths = new ArrayList<>();
    private long stime;
    private GridLayoutManager gridLayoutManager;


    @RequiresApi(api = Build.VERSION_CODES.N)
    private void scanFolder(@NonNull File rootFile) {
        // 不扫描应用缓存文件夹 .XXX
        if (rootFile.getPath().contains(".")) {
            return;
        }
        File[] fileArray = rootFile.listFiles();
        if (fileArray == null) {
            return;
        }

        //目录内图片集
        ArrayList<PcPathBean> tempList = new ArrayList<>();

        //开始扫描目录下文件
        for (int i = 0; i < fileArray.length; i++) {
            File file = fileArray[i];
            String fileName = file.getName();
            //不扫描声明了非媒体的文件夹 （含.nomedia文件的文件夹）
            if (fileName.equals(".nomedia")) {
                return;
            } else if (file.isDirectory()
                    && fileName.charAt(0) != '.'
                    && !fileName.equals("Android")) {
//                Log.d(TAG, "scanFolder: " + file.getPath());
                scanFolder(file);
            } else if (fileName.endsWith(".jpeg") || fileName.endsWith(".jpg") || fileName.endsWith(".png")
                    || fileName.endsWith(".webp") || fileName.endsWith(".gif")) {
                tempList.add(new PcPathBean(file.getAbsolutePath(), file.lastModified()));
            }
//            Log.d(TAG, "scanFolder: "+file.getName());
        }
        if (tempList.size() == 0) {
            return;
        }
        //扫描结束，添加目录和图片集
//        imgFolders.add(new PcDirBean(
//                rootFile.getAbsolutePath(),
//                tempList.get(0).getPath(),
//                rootFile.getName(),
//                tempList.size()));
        imgPaths.addAll(tempList);

    }

    public native void native_scan(String rootPath);


    int flag = 0;

    //native回调
    @RequiresApi(api = Build.VERSION_CODES.N)
    @SuppressLint("NotifyDataSetChanged")
    public void nativeCallback(ArrayList<PcPathBean> nativeList) {
        imgPaths.addAll(nativeList);
        if (picAdapter == null) {
            return;
        }
        runOnUiThread(() -> {
            setTitle("" + imgPaths.size());
//            if (imgPaths.size() < gridLayoutManager.findLastVisibleItemPosition()) {
//                picAdapter.notifyItemRangeInserted(
//                        imgPaths.size() - nativeList.size(), nativeList.size());
//                Log.d(TAG, "nativeCallback: "+gridLayoutManager.findLastVisibleItemPosition());
//            }
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


    @RequiresApi(api = Build.VERSION_CODES.N)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.d(TAG, "onCreate: " + System.currentTimeMillis());
        //native后台扫描线程
        new Thread(() -> native_scan(rootPath)).start();


        findViewById(R.id.btn).setOnClickListener(v -> {
            //app重新启动
            Intent intent = getPackageManager()
                    .getLaunchIntentForPackage(getApplication().getPackageName());
            PendingIntent restartIntent = PendingIntent.getActivity(getApplicationContext(), 0, intent, 0);
            AlarmManager mgr = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
            mgr.set(AlarmManager.RTC, System.currentTimeMillis() + 1000, restartIntent); // 1秒钟后重启应用
            System.exit(0);
        });

        initView();


        recyclerView.setHasFixedSize(true);
        gridLayoutManager = new GridLayoutManager(context, 4);
        recyclerView.setLayoutManager(gridLayoutManager);
        picAdapter = new TestAdapter(imgPaths, context, rootPath);
        recyclerView.setAdapter(picAdapter);
        recyclerView.setItemViewCacheSize(30);

        // 开始时间
        stime = System.currentTimeMillis();

        //广搜
//        scanFolder(new File(Environment.getExternalStorageDirectory().getAbsolutePath()));
////
//
//        List<PcDirBean> pictureFolders = getPictureFolders();
////        Log.d(TAG, "pictureFolders.size= " + pictureFolders.size());
//        Log.d(TAG, "pic.size= " + conPaths.size());
////
//        stime=System.currentTimeMillis();
//        Collections.sort(conPaths, new Comparator<PcPathBean>() {
//            @Override
//            public int compare(PcPathBean pcPathBean, PcPathBean t1) {
//                if (pcPathBean.time == t1.time) {
//                    return 0;
//                } else if (pcPathBean.time > t1.time) {
//                    return -1;
//                } else {
//                    return 1;
//                }
//            }
//        });
//
//        expendTime();
//
//        runOnUiThread(() -> {
//            picAdapter.notifyDataSetChanged();
//        });

    }

    ArrayList<PcDirBean> imgFolders = new ArrayList<>();


    public List<PcDirBean> getPictureFolders() {
        List<PcDirBean> folders = new ArrayList<>();
        // 扫描图片
        ContentResolver mContentResolver = context.getContentResolver();

        Cursor c = mContentResolver.query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, null,
                MediaStore.Images.Media.MIME_TYPE + "= ? or " + MediaStore.Images.Media.MIME_TYPE + "= ?",
                new String[]{"image/jpeg", "image/png"}, null);
        ArrayList<String> mDirs = new ArrayList<>();//用于保存已经添加过的文件夹目录
        while (c.moveToNext()) {
            @SuppressLint("Range")
            String path = c.getString(c.getColumnIndex(MediaStore.Images.Media.DATA));
            File parentFile = new File(path).getParentFile();
            if (parentFile == null)
                continue;
            String dir = parentFile.getAbsolutePath().toLowerCase();
            if (mDirs.contains(dir)) {//如果已经添加过
                continue;
            }
            mDirs.add(dir);//添加到保存目录的集合中
//            parentFile.list((d,n)->).length
            int count = 0;
            for (File file : parentFile.listFiles()) {
                String filename = file.getName();
                if (filename.endsWith(".jpeg") || filename.endsWith(".jpg") || filename.endsWith(".png")
                        || filename.endsWith(".webp") || filename.endsWith(".gif")) {
                    count++;
                    conPaths.add(new PcPathBean(file.getPath(), file.lastModified()));
                }
            }
            folders.add(new PcDirBean(dir, path, parentFile.getName(), count));
        }

        c.close();

        return folders;
    }

    //检测图片
    public boolean checkIsImageFile(String name) {
        return name.endsWith("jpg") || name.endsWith("png") || name.endsWith("webp")/* || name.equals("gif")*/
                || name.endsWith("jpeg") || name.endsWith("bmp");
    }

    private void initView() {
        recyclerView = findViewById(R.id.recyclerView);
    }
}