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
    return f1.time < f2.time;
}


vector<Folder> v_folder;
fixed_thread_pool *pool = nullptr;
vector<FileInfo> allFile;
mutex mx;

inline void sortItem(const __wrap_iter<FileInfo *> &begin, const __wrap_iter<FileInfo *> &end) {
    std::sort(begin, end, sortByPic);
}

inline void cellMerge(const __wrap_iter<FileInfo *> &begin, const __wrap_iter<FileInfo *> &mid,
                      const __wrap_iter<FileInfo *> &end) {
    std::inplace_merge(begin, mid, end, sortByPic);
}

void sortFile() {
    short int numThread = pool->num_thread;
    auto begin = allFile.begin();
    auto end = allFile.end();
    int siz = std::distance(begin, end);

    //将siz均分为numThread个部分，分别排序
    int basicFragSiz = siz / numThread;

    //如果resiude>0,则把它们平摊到前residue个线程上
    int residue = siz - basicFragSiz * numThread;

    //确定开始多线程处理
    //1 分片;
    std::vector<std::pair<__wrap_iter<FileInfo *>, __wrap_iter<FileInfo *>>> prs(numThread);
    for (int i = 0; i < numThread; ++i) {
        auto &temp = prs[i];
        temp.first = begin;
        temp.second = begin + basicFragSiz + (residue-- > 0 ? 1 : 0);
        begin = prs[i].second;
    }

    //2，异步处理
    for (int i = 0; i < numThread; ++i) {
        pool->execute(sortItem, prs[i].first, prs[i].second);
    }
    //等待片段结束
    pool->waitFinish();

    //开始多线程归并
    int k = (int) log2(numThread);
    int d = 2;
    int sum = 0;
    for (int i = 0; i < k; ++i) {
        for (int j = 0; j < numThread; j += d) {
            sum++;
            auto &start = prs[j].first;
            auto &last = prs[j + d - 1].second;
            auto &mid = prs[j + (d >> 1)].first;
            pool->execute(cellMerge, start, mid, last);
        }
        d = d << 1;
        pool->waitFinish();
    }
    LOGI("sum %d", sum);
}

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
            tempFileList.clear();
            return;
        }

        if (dirp->d_type == DT_DIR
            //忽略第一个字符是点的文件夹
            && dirp->d_name[0] != '.'
            //忽略 Android目录
            && strcmp(dirp->d_name, "Android") != 0) {
            string temp = path + "/" + dirp->d_name;
            pool->execute(doScan, temp);
        } else if (dirp->d_type == DT_REG
                   && isPicture(dirp->d_name)) {
            struct stat fileStat;
//                lstat((path + "/" + dirp->d_name).c_str(), &fileStat);
            fstatat(dirfd(dir), dirp
                    ->d_name, &fileStat, 0);
            tempFileList.emplace_back(path.substr(len_r_path, path.length() - len_r_path) + "/" + dirp->d_name, fileStat.st_mtim.tv_sec);
        }
    }
    //该目录扫描完成
    closedir(dir);


    mx.lock();
//    long start = getMs();
    allFile.insert(allFile.end(), tempFileList.begin(), tempFileList.end());
//    LOGI("insert spendTime%ld", getMs() - start);
    mx.unlock();

}


class Scanner {

private:
    FileInfo fileList[CALLBACK_COUNT];//开辟内存

public:

    int curFileIndex = 0;
    Scanner *m;


    Scanner(const string &path) {
        len_r_path=path.length();
        allFile.reserve((int) pow(2, 13));//预分配8192大小
        long startTime = getMs();
        pool = new fixed_thread_pool();
        LOGI("createPool time> %ld", startTime);
        startTime = getMs();
        pool->execute(doScan, path);
        pool->waitFinish();
        LOGI("scanEnd spendTime%ld", getMs() - startTime);
        sortAndBack();
    }

    ~Scanner() {
        LOGI("扫描类销毁 %ld", getMs());
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

        for (int i = 0; i < 4; ++i) {
            allFile.insert(allFile.end(), allFile.begin(), allFile.end());
        }

        LOGI("size %d", allFile.size());

//        if(allFile.size()>(int)pow(2,14))
//        sortFile();
//        else
        std::sort(allFile.begin(), allFile.end(), sortByPic);
        LOGI("sort spendTime %ld", getMs() - start);
        start = getMs();
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
        LOGI("doCallback spendTime %ld", getMs() - start);
    }
};

