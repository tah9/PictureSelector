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
#include "./ThreadPool.cpp"

#define CALLBACK_COUNT 20

using namespace std;

long int getMs() {
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

//判断文件是否是图片（根据文件名后缀）
int isPicture(string name) {
    string ends[5] = {".jpg", ".png", ".jpeg", ".webp", ".gif"};
    for (int i = 0; i < 5; i++) {
        if (name.find(ends[i]) != string::npos) {
            return 1;
        }
    }
    return 0;
}

int sortByPic(FileInfo f1, FileInfo f2) {
    return f1.time < f2.time;
}

int sortByFolder(Folder f1, Folder f2) {
    return f1.s_time < f2.s_time;
}

int cpuNumber;

vector<Folder> v_folder;
ThreadPool *mPool = nullptr;
pthread_t ts[10000];

void doScan(string path) {
    string curDir = path;
    DIR *dir = opendir(curDir.c_str());
    if (dir == nullptr) {
        return;
    }
    //循环该目录下每个文件
    struct dirent *dirp;
    auto *tempFileList = new vector<FileInfo>;
    while ((dirp = readdir(dir)) != nullptr) {
        //忽略文件目录中前两个文件.和..
        if (strcmp(dirp->d_name, ".") == 0
            || strcmp(dirp->d_name, "..") == 0) {
            continue;
        }

            //目录内有.nomedia文件或目录，停止该循环
        else if (strcmp(dirp->d_name, ".nomedia") == 0) {
            tempFileList->clear();
            break;
        }


        if (dirp->d_type == DT_DIR
            //忽略带点的文件夹
            //                    && string(dirp->d_name).find(".gs") == string::npos
            && dirp->d_name[0] != '.'
            //忽略 Android目录
            && strcmp(dirp->d_name, "Android") != 0
                ) {
            //将目录压入deque
//                mDirQueue.emplace_back(curDir + "/" + dirp->d_name);

            string temp = curDir + "/" + dirp->d_name;
//            mPool->enqueue(doScan, temp);
//            mPool->enqueue([temp] {
                doScan(temp);
//            });
//            doScan(temp);
//            LOGI("dir>>> ");

//            pthread_create(&ths[curThread++], nullptr, );
//                doScan(curDir + "/" + dirp->d_name);
        } else if (dirp->d_type == DT_REG
                   && isPicture(dirp->d_name)) {
            FileInfo fileInfo;
            fileInfo.path = curDir + "/" + dirp->d_name;
            struct stat fileStat;
//                lstat(fileInfo.path.c_str(), &fileStat);
            fstatat(dirfd(dir), dirp->d_name, &fileStat, 0);
            fileInfo.time = fileStat.st_mtim.tv_sec;
            tempFileList->push_back(fileInfo);
        }
    }
    //该目录扫描完成
    closedir(dir);

    if (tempFileList->empty()) {
//        tempFileList.shrink_to_fit();
        return;
    }

    std::sort(tempFileList->begin(), tempFileList->end(), sortByPic);
    Folder folder;
    FileInfo firstPic = tempFileList->back();
    folder.pics = tempFileList;
    folder.first_path = firstPic.path;
    folder.name = curDir.substr(curDir.find_last_of('/') + 1);
    folder.m_time = firstPic.time;
    folder.s_time = folder.m_time;

    v_folder.emplace_back(folder);
}

class MMScanner {

public:

    JNIEnv *env;
    FileInfo *fileList = new FileInfo[CALLBACK_COUNT];//开辟内存
    int curFileIndex = 0;

    void doCallback();

    MMScanner(JNIEnv *v, string path) {
        env = v;
        cpuNumber = sysconf(0x0060);
        long startTime = getMs();
        LOGI("scanFolder time> %ld", startTime);
//        for (int i = 0; i < 10000; ++i) {
//            pthread_create(&ts[i],nullptr,)
//        }
//        mPool = new ThreadPool(cpuNumber * 2);
//        mPool->enqueue([path] {
            doScan(path);
//        });
//        future.wait();
//        mPool->enqueue(doScan, path);
//        mPool->waitAll();
        LOGI("scanFolderEnd time> %ld", getMs());
        sortAndBack();
        LOGI("sortAndBackEnd time> %ld", getMs());
        delete this;
    }

    ~MMScanner() {
        LOGI("扫描类销毁");
        delete[] fileList;
        delete env;
    }

    void sortAndBack() {
        /**
       * 将目录向量按排序时间排序，
       * 从目录向量弹出最后一个目录，取出最后一个向量（最新图片），填入图片数组，下标++，
       * 修改该目录结构排序时间为（倒数第二个向量图片的修改时间）。
       * 若目录的图片向量size==0，从目录向量内弹出该目录，将目录向量按排序时间排序，递归执行该操作。
       */

        while (!v_folder.empty()) {
            std::sort(v_folder.begin(), v_folder.end(), sortByFolder);
            Folder *folder = &v_folder.back();
            vector<FileInfo> *pics = folder->pics;
            fileList[curFileIndex++] = pics->back();
            pics->pop_back();
            if (pics->empty()) {
                v_folder.pop_back();
            } else {
                folder->s_time = pics->back().time;
            }
            if (curFileIndex >= CALLBACK_COUNT) {
                doCallback(); //这里把扫描到的文件信息回调到java层，500文件回调一次
//                sleep(3);
            }
        }



        //所有目录扫描完成，回调剩下的小部分文件，释放内存
        if (curFileIndex > 0) {
            doCallback();
        }

    }


    /*
     * 递归方法
     * 进入方法，创建线程进行扫描。
     * 扫描到目录，将目录路径放入该方法递归操作。
     */
    void scanFolder(string rootPath) {


//        pthread_create(&ths[curThread++], nullptr, doScan, &rootPath);
//
//        for (int i = 0; i < curThread; ++i) {
//            pthread_join(ths[i], nullptr);
//        }

        //创建目录向量
//        vector<string> mDirQueue;
//        mDirQueue.emplace_back(rootPath);
        /**
         * IO扫描目录（广度优先遍历），
         * 创建临时向量保存图片，遇到nomedia清空向量并停止扫描。
         * 该目录扫描结束按时间将临时向量排序，size>0 创建目录结构，将该目录压入目录向量。
         */
//        while (!mDirQueue.empty()) {

//            std::string curDir = mDirQueue.back();
//            mDirQueue.pop_back();



//        }

    }
};

