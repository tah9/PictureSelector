//
// Created by tah9 on 2023/3/6.
//
#include "./Scanner.h"

using namespace std;

fixed_thread_pool *pool;
std::mutex *mx;

std::vector<FileInfo> allFile;
std::vector<Folder> v_folder;

unsigned int len_r_path;

long int getMs() {
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

inline int sortByPic(const FileInfo &f1, const FileInfo &f2) {
    return f1.time > f2.time;
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
                   && ets::isPicture(dirp->d_name)) {
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

    mx->lock();
    allFile.insert(allFile.end(), tempFileList.begin(), tempFileList.end());
    mx->unlock();
}

Scanner::Scanner(const string &path) {
    len_r_path = path.length();
    LOGI("path >%s", path.c_str());
    allFile.reserve((int) pow(2, 13));//预分配8192大小
    long startTime = getMs();
    LOGI("createPool time> %ld", startTime);
    startTime = getMs();
    mx = new std::mutex;
    pool = new fixed_thread_pool();
    pool->execute(doScan, path);
    pool->waitFinish();
    LOGI("scanEnd spendTime%ld", getMs() - startTime);
    sortAndBack();
}

Scanner::~Scanner() {
    allFile.resize(0);
    delete pool;
    delete mx;
    LOGI("扫描类销毁 %ld", getMs());
}

void Scanner::sortAndBack() {
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
