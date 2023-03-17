//
// Created by tah9 on 2023/3/6.
//
#include <iostream>
#include <utility>
#include <vector>
#include <dirent.h>
#include <jni.h>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "./FileInfo.cpp"
#include "../JNI_LOG.h"
#include <map>
#include <memory>
#include <algorithm>
#include <execution>
#include <chrono>
#include <cstdint>
#include "./SimpleThreadPool.cpp"

#define CALLBACK_COUNT 50
const static string pic_extension[5] = {".jpg", ".png", ".jpeg", ".webp", ".gif"};
using namespace std;
unsigned int len_r_path;

long int getMs() {
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

//判断文件是否是图片（根据文件名后缀）
inline int isPicture(const string &name) {
    if (name.length() < 5)return 0;
    for (int i = 0; i < pic_extension->length(); ++i) {
        if (name.rfind(pic_extension[i]) != std::string::npos) return 1;
    }
    return 0;
}

inline int sortByPic(const FileInfo &f1, const FileInfo &f2) {
    return f1.time > f2.time;
}


vector<Folder> v_folder;
fixed_thread_pool pool;
vector<FileInfo> allFile;
mutex mx;


void doScan(const string &path) {
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }
    //循环该目录下每个文件
    struct dirent *dirp;
    vector<FileInfo> tempFileList;
    while ((dirp = readdir(dir)) != nullptr) {
        //忽略文件目录中前两个文件.和..
        if (strcmp(dirp->d_name, ".") == 0
            || strcmp(dirp->d_name, "..") == 0) {
            continue;
        }

            //目录内有.nomedia文件或目录，停止该循环
        else if (strcmp(dirp->d_name, ".nomedia") == 0) {
            return;
        }

        if (dirp->d_type == DT_DIR
            //忽略第一个字符是点的文件夹
            && dirp->d_name[0] != '.'
            //忽略 Android目录
            && strcmp(dirp->d_name, "Android") != 0) {
            string temp = path + "/" + dirp->d_name;
            pool.execute(doScan, temp);
        } else if (dirp->d_type == DT_REG
                   && isPicture(dirp->d_name)) {
            struct stat fileStat;
//                lstat((path + "/" + dirp->d_name).c_str(), &fileStat);
            fstatat(dirfd(dir), dirp
                    ->d_name, &fileStat, 0);
            tempFileList.emplace_back(
                    path.substr(len_r_path, path.length() - len_r_path) + "/" + dirp->d_name,
                    fileStat.st_mtim.tv_sec);
        }
    }
    //该目录扫描完成
    closedir(dir);
//    std::sort(tempFileList.begin(), tempFileList.end(), sortByPic);

    mx.lock();
    allFile.insert(allFile.end(), tempFileList.begin(), tempFileList.end());
    mx.unlock();
}


class Scanner {

public:

    Scanner(const string &path) {
        len_r_path = path.length();
        allFile.reserve((int) pow(2, 13));//预分配8192大小
        long startTime = getMs();
        LOGI("createPool time> %ld", startTime);
        startTime = getMs();
        pool.execute(doScan, path);
        pool.waitFinish();
        LOGI("scanEnd spendTime%ld", getMs() - startTime);
        sortAndBack();
    }

    ~Scanner() {
        LOGI("扫描类销毁 %ld", getMs());
    }

    void doCallback(size_t left, size_t right);

    /**
   * 将目录向量按排序时间排序，
   * 从目录向量弹出最后一个目录，取出最后一个向量（最新图片），填入图片数组，下标++，
   * 修改该目录结构排序时间为（倒数第二个向量图片的修改时间）。
   * 若目录的图片向量size==0，从目录向量内弹出该目录，将目录向量按排序时间排序，递归执行该操作。
   */

    /* 对vector按时间从小到大排序
     * 2023.3.7
     */
    void sortAndBack() {


        allFile.shrink_to_fit();
        long start = getMs();
        /*
         * 在排序前先回调topK个，k是回调阈值，若集合小于k，不操作（0）
         */
        int topK = CALLBACK_COUNT > allFile.size() ? 0 : CALLBACK_COUNT;
        LOGI("topK  %d", topK);

        std::partial_sort(allFile.begin(), allFile.begin() + topK, allFile.end(), sortByPic);
        doCallback(0, topK);
        LOGI("partial_sort_callback spendTime %ld", getMs() - start);
        start = getMs();
        LOGI("size %d", allFile.size());
        std::sort(allFile.begin() + topK, allFile.end(), sortByPic);
        LOGI("sort spendTime %ld", getMs() - start);
        size_t num = 0, end = allFile.size() - allFile.size() % CALLBACK_COUNT;
        for (size_t i = topK; i < end; ++i) {
            if (++num >= CALLBACK_COUNT) {
                doCallback(i - CALLBACK_COUNT, i);
                num = 0;
            }
        }
        doCallback(end, allFile.size());
        LOGI("last_callback spendTime %ld", getMs() - start);
    }
};

