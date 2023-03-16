//
// Created by tah9 on 2023/3/1.
//
#ifndef JNI_LOG_H
#define JNI_LOG_H
#define __DEBUG__ANDROID__ON
//write debug images
#ifdef  __DEBUG__ANDROID__ON
#include <android/log.h>
// Define the LOGI and others for print debug infomation like the log.i in java
#define LOG_TAG    "JNI_LOG"
//#undef LOG
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG, __VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, __VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, __VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG, __VA_ARGS__)
#else
#ifdef __DEBUG__WIN__ON
#define LOGI(...)  printf( __VA_ARGS__); printf("\n")
#define LOGD(...)  printf( __VA_ARGS__); printf("\n")
#define LOGW(...)  printf( __VA_ARGS__); printf("\n")
#define LOGE(...)  printf( __VA_ARGS__); printf("\n")
#define LOGF(...)  printf( __VA_ARGS__); printf("\n")
#else
#define LOGI(...)
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)
#define LOGF(...)
#endif
#endif

#endif //DEMO2_23_DEMO_H
