//
// Created by tah9 on 2023/3/19.
//

#ifndef DEMO2_23_SCANNER_H
#define DEMO2_23_SCANNER_H

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
#include "./FileExtension.h"

#define CALLBACK_COUNT 50

class Scanner {

public:

    Scanner(const string &path);

    ~Scanner();

    /**
   * 将目录向量按排序时间排序，
   * 从目录向量弹出最后一个目录，取出最后一个向量（最新图片），填入图片数组，下标++，
   * 修改该目录结构排序时间为（倒数第二个向量图片的修改时间）。
   * 若目录的图片向量size==0，从目录向量内弹出该目录，将目录向量按排序时间排序，递归执行该操作。
   */

    /* 对vector按时间从小到大排序
     * 2023.3.7
     */
    void sortAndBack();

    void doCallback(size_t left, size_t right);

};

#endif //DEMO2_23_SCANNER_H
