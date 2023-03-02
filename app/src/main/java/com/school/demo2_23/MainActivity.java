package com.school.demo2_23;

import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.bumptech.glide.request.target.BitmapImageViewTarget;
import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("demo2_23");
    }

    public Context context = this;
    private static final String TAG = "MainActivity";
    private RecyclerView recyclerView;
    private int width;
    private TestAdapter TestAdapter;


    ArrayList<PcPathBean> imgPaths = new ArrayList<>();
    private long stime;

    private void scanFolder(File rootFile) {
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
            if (file.getName().equals(".nomedia")) {
                return;
            } else if (file.isDirectory()) {
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
        imgFolders.add(new PcDirBean(
                rootFile.getAbsolutePath(),
                tempList.get(0).getPath(),
                rootFile.getName(),
                tempList.size()));
        imgPaths.addAll(tempList);

    }

    void getPicturePath() {
        File rootFile = Environment.getExternalStorageDirectory();
        Log.d(TAG, "Environment.getExternalStorageDirectory(): " + rootFile.getAbsolutePath());
        scanFolder(rootFile);
    }

    public native void native_scan(String rootPath);

    public native void instanceNative();


    //native回调
    @RequiresApi(api = Build.VERSION_CODES.N)
    @SuppressLint("NotifyDataSetChanged")
    public void nativeCallback(ArrayList<PcPathBean> nativeList) {
//        Log.d(TAG, "nativeCallback: "+imgPaths.hashCode());
//        Log.d(TAG, "nativeCallback: "+pcPathBeans.hashCode());
//        Log.d(TAG, "nativeCallback: "+pcPathBeans.size());
        imgPaths.addAll(nativeList);
        imgPaths.sort((pcPathBean, t1) -> (int) (t1.time - pcPathBean.time));
        Log.d(TAG, "nativeCallback: " + imgPaths.size());
        runOnUiThread(() -> TestAdapter.notifyDataSetChanged());
//        expendTime();
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
        // 开始时间
        stime = System.currentTimeMillis();
        findViewById(R.id.btn).setOnClickListener(v -> {
            //app重新启动
            Intent intent = getPackageManager()
                    .getLaunchIntentForPackage(getApplication().getPackageName());
            PendingIntent restartIntent = PendingIntent.getActivity(getApplicationContext(), 0, intent, 0);
            AlarmManager mgr = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
            mgr.set(AlarmManager.RTC, System.currentTimeMillis() + 1000, restartIntent); // 1秒钟后重启应用
            System.exit(0);
        });
        instanceNative();
        width = Tools.getWidth(context);

//        setTitle("消耗时间: " + (etime - stime));
        initView();


        recyclerView.setHasFixedSize(true);
        recyclerView.setLayoutManager(new GridLayoutManager(context, 3));
        TestAdapter = new TestAdapter();
        recyclerView.setAdapter(TestAdapter);
//        String idPASideBase64 = FileUtils.getFileContent(new File("/sdcard/Gyt/idPASide.txt"));
//        imgPaths = new Gson().fromJson(idPASideBase64, new TypeToken<ArrayList<PcPathBean>>() {
//        }.getType());
        recyclerView.setItemViewCacheSize(30);


//广度优先搜索
//        getPicturePath();
//        Log.d(TAG, "onCreate: "+imgPaths.size());
//        Log.d(TAG, "onCreate: imgFolders.size=" + imgFolders.size());
//        Log.d(TAG, "onCreate: imgPaths.size=" + imgPaths.size());
//        for (PcDirBean imgFolder : imgFolders) {
//            Log.d(TAG, "onCreate: " + imgFolder);
//        }


//        List<PcDirBean> pictureFolders = getPictureFolders();
//        Log.d(TAG, "pictureFolders.size= " + pictureFolders.size());


        //native后台扫描线程
        new Thread(() ->
                native_scan(Environment.getExternalStorageDirectory().getAbsolutePath()))
                .start();

//        new Handler(getMainLooper()).postDelayed(new Runnable() {
//            @Override
//            public void run() {
//                Gson gson=new Gson();
//                String s = gson.toJson(imgPaths, new TypeToken<ArrayList<PcPathBean>>() {
//                }.getType());
//                FileUtils.writeTxtToFile(s, "/sdcard/Gyt/", "idPASide.txt");
//            }
//        },5000);
    }

    ArrayList<PcDirBean> imgFolders = new ArrayList<>();

    static class ViewHolder extends RecyclerView.ViewHolder {
        ImageView pic;

        public ViewHolder(@NonNull View itemView) {
            super(itemView);
            this.pic = (ImageView) ((FrameLayout) itemView).getChildAt(0);
        }
    }

    class TestAdapter extends RecyclerView.Adapter<ViewHolder> {

//        @Override
//        public void onViewRecycled(@NonNull ViewHolder holder) {
//            super.onViewRecycled(holder);
//            ImageView imageView = holder.pic;
//            if (imageView != null) {
//                Glide.with(context).clear(imageView);
//            }
//        }

        @NonNull
        @Override
        public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            FrameLayout frameLayout = new FrameLayout(parent.getContext());
            ImageView pic = new ImageView(frameLayout.getContext());
            pic.setScaleType(ImageView.ScaleType.CENTER_CROP);
            frameLayout.addView(pic, -1, (int) (width / 3f));
            Log.d(TAG, "onCreateViewHolder: ");
            return new ViewHolder(frameLayout);
        }

        @Override
        public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
            Log.d(TAG, "onBindViewHolder: " + position);
            PcPathBean pcPathBean = imgPaths.get(position);
            ImageView pic = holder.pic;
//            Log.d(TAG, "onBindViewHolder: "+pcPathBean.path);
            if (pcPathBean.path.endsWith("gif")) {
                /*
                Glide加载gif机制是从bitmapPool获取新的bitmap对象，会创建很多bitmap占用内存
                 */
                BitmapImageViewTarget bitmapImageViewTarget = new BitmapImageViewTarget(pic) {
                    @Override
                    protected void setResource(Bitmap resource) {
                        BitmapDrawable drawable = new BitmapDrawable(resource);
                        pic.setImageDrawable(drawable);
                    }
                };
                Glide.with(holder.itemView).asBitmap().load(pcPathBean.getPath())
                        .skipMemoryCache(true)
                        .diskCacheStrategy(DiskCacheStrategy.NONE).into(bitmapImageViewTarget);
                Log.d(TAG, "onBindViewHolder: " + pcPathBean.path);
            } else {
                Glide.with(holder.itemView).load(pcPathBean.getPath())
                        .skipMemoryCache(true)
                        .diskCacheStrategy(DiskCacheStrategy.NONE).into(pic);
            }
/*
//                    .apply(new RequestOptions().override(200,200))*/
        }

        @Override
        public int getItemCount() {
            return imgPaths.size();
        }
    }

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
                Log.d(TAG, "getPictureFolders: ");
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
                    imgPaths.add(new PcPathBean(file.getPath(), file.lastModified()));
                }
            }
            folders.add(new PcDirBean(dir, path, parentFile.getName(), count));
        }

        c.close();

        return folders;
    }


    /**
     * 通过图片文件夹的路径获取该目录下的图片
     */
//    public ArrayList<PcPathBean> getImgListByDir(String dir) {
//        ArrayList<PcPathBean> imgPaths = new ArrayList<>();
//        File directory = new File(dir);
//        if (!directory.exists()) {
//            return imgPaths;
//        }
//        File[] files = directory.listFiles();
//        for (File file : files) {
//            String path = file.getAbsolutePath();
//            if (checkIsImageFile(path)) {
//            }
//        }
//        return imgPaths;
//    }

    //检测图片
    public boolean checkIsImageFile(String name) {
        return name.endsWith("jpg") || name.endsWith("png") || name.endsWith("webp")/* || name.equals("gif")*/
                || name.endsWith("jpeg") || name.endsWith("bmp");
    }

    private void initView() {
        recyclerView = findViewById(R.id.recyclerView);
    }
}