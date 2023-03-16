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
//#include "./MSort.h"
#include "./SimpleThreadPool.cpp"

#define CALLBACK_COUNT 50
const static string pic_extension[5] = {".jpg", ".png", ".jpeg", ".webp", ".gif"};
using namespace std;
std::vector<std::future<std::vector<FileInfo> *>> fs;

long int getMs() {
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

//判断文件是否是图片（根据文件名后缀）
int isPicture(string &&name) {
    return name.find(pic_extension[0]) != std::string::npos
           || name.find(pic_extension[1]) != std::string::npos
           || name.find(pic_extension[2]) != std::string::npos
           || name.find(pic_extension[3]) != std::string::npos
           || name.find(pic_extension[4]) != std::string::npos;
}

int sortByPic(FileInfo f1, FileInfo f2) {
    return f1.time < f2.time;
}


vector<Folder> v_folder;
fixed_thread_pool *pool = nullptr;
vector<FileInfo> allFile;
mutex *mx;


void sort(__wrap_iter<FileInfo *> begin, __wrap_iter<FileInfo *> end, std::sort<__wrap_iter<FileInfo *>, std::sort> compare) {
    short int numThread = pool->num_thread;
    int siz = std::distance(begin, end);

    //将siz均分为numThread个部分，分别排序
    int basicFragSiz = siz / numThread;

    //如果resiude>0,则把它们平摊到前residue个线程上
    int residue = siz - basicFragSiz * numThread;

    //确定开始多线程处理
    //1 分片;
    std::vector<std::pair<__wrap_iter<FileInfo *>, __wrap_iter<FileInfo *>>> prs(numThread);
    for (int i = 0; i < numThread; ++i) {
        prs[i].first = begin;
        prs[i].second = begin + basicFragSiz + (residue-- > 0 ? 1 : 0);
        begin = prs[i].second;
    }

    //2，异步处理
    std::vector<std::future<void>> futures;
    for (int i = 0; i < numThread; ++i) {
        futures.emplace_back(std::move(pool->execute(std::sort<__wrap_iter<FileInfo *>, std::sort>,
                                                     prs[i].first, prs[i].second,
                                                     compare)));
    }

    //等待片段结束
    for (int i = 0; i < numThread; ++i) {
        futures[i].get();
    }

    //开始多线程归并
    int k = (int) log2(numThread);
    int d = 2;
    for (int i = 0; i < k; ++i) {
        std::vector<std::future<void>> tmpfutures;
        for (int j = 0; j < numThread; j += d) {
            auto start = prs[j].first;
            auto end = prs[j + d - 1].second;
            auto mid = prs[j + (d >> 1)].first;
            tmpfutures.emplace_back(std::move(
                    pool->execute(std::inplace_merge<_RandomIt, _Compare>, start, mid, end,
                                  compare)));
        }
        for (int j = 0; j < numThread / d; ++j) {
            tmpfutures[j].get();
        }
        d = d << 1;
    }
}


std::vector<FileInfo> *doScan(const string &path) {
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return nullptr;
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
            delete tempFileList;
            return nullptr;
        }


        if (dirp->d_type == DT_DIR
            //忽略第一个字符是点的文件夹
            && dirp->d_name[0] != '.'
            //忽略 Android目录
            && strcmp(dirp->d_name, "Android") != 0) {
            string temp = path + "/" + dirp->d_name;
            mx->lock();
            fs.emplace_back(std::move(pool->execute(doScan, temp)));
            mx->unlock();
        } else if (dirp->d_type == DT_REG
                   && isPicture(dirp->d_name)) {
            struct stat fileStat;
//                lstat((path + "/" + dirp->d_name).c_str(), &fileStat);
            fstatat(dirfd(dir), dirp
                    ->d_name, &fileStat, 0);
            tempFileList->emplace_back(path + "/" + dirp->d_name, fileStat.st_mtim.tv_sec);
        }
    }
    //该目录扫描完成
    closedir(dir);

    return tempFileList;
//    mx->lock();
//    allFile.insert(allFile.end(), tempFileList.begin(), tempFileList.end());
//    mx->unlock();

}

class Scanner {

private:
    FileInfo *fileList = new FileInfo[CALLBACK_COUNT];//开辟内存

public:

    int curFileIndex = 0;
    Scanner *m;


    Scanner(string path) {
        sysconf(0x0060);
        mx = new mutex;
        allFile.reserve(8080);
        long startTime = getMs();
        LOGI("scanFolder time> %ld", startTime);
        startTime = getMs();
        pool = new fixed_thread_pool();

        fs.emplace_back(std::move(pool->execute(doScan, path)));
        for (int i = 0; i < fs.size(); ++i) {
            vector<FileInfo> *te = fs[i].get();
            if (te)
                allFile.insert(allFile.end(), te->begin(), te->end());
        }

        LOGI("scanEnd spendTime%ld", getMs() - startTime);
        sortAndBack();
        LOGI("sortAndBack spendTime%ld", getMs() - startTime);

    }

    ~Scanner() {
        delete mx;
        delete[] fileList;
        LOGI("扫描类销毁");
    }

    void doCallback();

    /**
   * 将目录向量按排序时间排序，
   * 从目录向量弹出最后一个目录，取出最后一个向量（最新图片），填入图片数组，下标++，
   * 修改该目录结构排序时间为（倒数第二个向量图片的修改时间）。
   * 若目录的图片向量size==0，从目录向量内弹出该目录，将目录向量按排序时间排序，递归执行该操作。
   */

    void sortAndBack() {
        allFile.shrink_to_fit();
        long start = getMs();
        std::sort(allFile.begin(), allFile.end(), sortByPic);
        LOGI("sort1 spendTime %ld", getMs() - start);
        start = getMs();
        ::sort(allFile.begin(), allFile.end(), sortByPic);
        LOGI("sort2 spendTime %ld", getMs() - start);
        for (auto it = allFile.rbegin();
             it != allFile.rend(); it++) {
            fileList[curFileIndex++] = *it;
            if (curFileIndex >= CALLBACK_COUNT) {
                doCallback(); //这里把扫描到的文件信息回调到java层
            }
        }
        //所有目录扫描完成，回调剩下的小部分文件，释放内存
        if (curFileIndex > 0) {
            doCallback();
        }
    }
};

