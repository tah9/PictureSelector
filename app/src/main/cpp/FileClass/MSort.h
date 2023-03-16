#ifndef __MSORT_H
#define __MSORT_H

#include <thread>
#include <future>
#include <algorithm>
#include <cassert>
#include <vector>
#include <cmath>
#include "./SimpleThreadPool.cpp"

namespace mcp {
//这两个参数可以适当调整 ，主要用于数据量少时直接在当前线程排序
#define MINIMUM_FOR_MULTITHREAD 64  //must not be too small!  支持多线程的最小元素个数

#define MINIMUM_FOR_ONE_FRAGMENT 2   //单个线程片段最小处理个数（否则会采用单线程)




}

#endif