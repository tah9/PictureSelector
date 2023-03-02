package com.school.demo2_23;

/**
 * @author tah9  2021/3/14 20:58
 */
public class PcPathBean {
    public String path;
    public long time;

    public PcPathBean() {
    }

    public PcPathBean(String path, long time) {
        this.path = path;
        this.time = time;
    }

    @Override
    public String toString() {
        return "PcPathBean{" +
                "path='" + path + '\'' +
                ", time=" + time +
                '}';
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public long getTime() {
        return time;
    }

    public void setTime(long time) {
        this.time = time;
    }
}
