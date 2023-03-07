package com.school.demo2_23.adapter;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.school.demo2_23.PcPathBean;
import com.school.demo2_23.R;
import com.school.demo2_23.Tools;

import java.util.ArrayList;

/**
 * ->  tah9  2023/3/6 10:26
 */

public class TestAdapter extends RecyclerView.Adapter<TestAdapter.ViewHolder> {
    private static final String TAG = "TestAdapter";
    private ArrayList<PcPathBean> imgPaths;


    public static int width;
    public TestAdapter(ArrayList<PcPathBean> imgPaths, Context context) {
        this.imgPaths = imgPaths;
        width = Tools.getWidth(context);
    }

    @NonNull
    @Override
    public TestAdapter.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
//        FrameLayout frameLayout = new FrameLayout(parent.getContext());
//        ImageView pic = new ImageView(frameLayout.getContext());
//        pic.setScaleType(ImageView.ScaleType.CENTER_CROP);
//        frameLayout.addView(pic, -1, (int) (width / 3f));
        return new TestAdapter.ViewHolder(
                LayoutInflater.from(parent.getContext()).inflate(R.layout.pic, null));
    }

    @Override
    public void onBindViewHolder(@NonNull TestAdapter.ViewHolder holder, int position) {
        PcPathBean pcPathBean = imgPaths.get(position);
        ImageView pic = holder.pic;
//            Log.d(TAG, "onBindViewHolder: "+pcPathBean.path);
        if (pcPathBean.path.endsWith("gif")) {
                /*
                Glide加载gif机制是从bitmapPool获取新的bitmap对象，会创建很多bitmap占用内存
                 */
            Glide.with(holder.itemView).asBitmap().load(pcPathBean.getPath())
                    .skipMemoryCache(true)
                    .diskCacheStrategy(DiskCacheStrategy.NONE).into(pic);
        } else {
            Glide.with(holder.itemView).load(pcPathBean.getPath())
                    .skipMemoryCache(true)
                    .diskCacheStrategy(DiskCacheStrategy.NONE).into(pic);
        }
        holder.tip.setText(pcPathBean.getPath());

/*
//                    .apply(new RequestOptions().override(200,200))*/
    }


    @Override
    public int getItemCount() {
        return imgPaths.size();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        ImageView pic;
        TextView tip;
        public ViewHolder(@NonNull View itemView) {
            super(itemView);
//            FrameLayout frame = (FrameLayout) itemView;
//            frame.measure(width/4,width/4);

//            this.pic = (ImageView) ((FrameLayout) itemView).getChildAt(0);
            this.pic = itemView.findViewById(R.id.pic);
            this.pic.getLayoutParams().width=width/4;
            this.pic.getLayoutParams().height=width/4;
            this.tip = itemView.findViewById(R.id.tip);
        }
    }
}
