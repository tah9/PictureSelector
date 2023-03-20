//
// Created by tah9 on 2023/3/19.
//
#include <ctime>
#include <iostream>
#include <utility>
#include <vector>
#include <dirent.h>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "./FileClass/Scanner.cpp"
#include "unistd.h"
#include "FileClass/Scanner.h"

static jclass jcls;
static jclass pc_cls;
static jobject jobj;
static jmethodID jCallbackMid;
static jmethodID file_costruct;
static jclass list_cls;
JNIEnv *env = nullptr;
JavaVM *jvm = nullptr;

/*
* env包含java线程
*/
void Scanner::doCallback(size_t left, size_t right) {
    if (right == 0) return;
    //Attach主线程
//    jvm->AttachCurrentThread(reinterpret_cast<JNIEnv **>(reinterpret_cast<void **>(&env)),
//                             nullptr);
//    jvm->AttachCurrentThread(&env, nullptr); //绑定当前线程，获取当前线程的JNIEnv

    //获得ArrayList类引用，结束后释放
    list_cls = env->FindClass("java/util/ArrayList");
    if (list_cls == nullptr) {
        cout << "listcls is null \n";
    }
    jmethodID list_costruct = env->GetMethodID(list_cls, "<init>", "()V"); //获得集合构造函数Id
//
//    //创建list局部引用，结束后释放
    jobject list_obj = env->NewLocalRef(
            env->NewObject(list_cls, list_costruct)); //创建一个Arraylist集合对象
    //或得Arraylist类中的 add()方法ID，其方法原型为： boolean add(Object object) ;
    jmethodID list_add = env->GetMethodID(list_cls, "add", "(Ljava/lang/Object;)Z");

    for (size_t i = left; i < right; ++i) {
        auto &file = allFile[i];
        jstring path = env->NewStringUTF(file.path.c_str());
        jlong time = file.time;
        //构造一个javabean文件对象
        jobject java_PcBean = env->NewObject(pc_cls, file_costruct,
                                             path, time);
        //执行Arraylist类实例的add方法，添加一个对象
        env->CallBooleanMethod(list_obj, list_add, java_PcBean);

        //释放局部引用
        env->DeleteLocalRef(path);
        env->DeleteLocalRef(java_PcBean);
    }
    //调用java回调方法
    env->CallVoidMethod(jobj, jCallbackMid, list_obj);

    //释放局部引用
    env->DeleteLocalRef(list_cls);
    env->DeleteLocalRef(list_obj);


//    jvm->DetachCurrentThread();
}


/**
 * 动态注册的方法一定要有  JNIEnv env, jobject thiz 两个参数
 */
void scan(JNIEnv *env, jobject thiz, jstring root_path) {
    ::env = env;

    LOGI("scan begin time> %ld", getMs());
    jobj = env->NewGlobalRef(thiz);
    jcls = (jclass) env->NewGlobalRef(env->FindClass("com/school/demo2_23/GalleryMain"));
//    jmethodID mainId = env->GetMethodID(jcls, "<init>", "()V");
//    jobj = env->NewGlobalRef(env->NewObject(jcls, mainId));
    //获取回调方法ID
    jCallbackMid = env->GetMethodID(jcls, "nativeCallback", "(Ljava/util/ArrayList;)V");


    pc_cls = (jclass) (env->NewGlobalRef(
            env->FindClass("com/school/demo2_23/PcPathBean")));//获得类引用
    //获得该类型的构造函数  函数名为 <init> 返回类型必须为 void 即 V
    file_costruct = env->GetMethodID(pc_cls, "<init>", "(Ljava/lang/String;J)V");



    // TODO: implement scan()
    auto path = env->GetStringUTFChars(root_path, nullptr);
    Scanner scanner(path);
    //释放字符串，放入jstring和env创建的字符串
    env->ReleaseStringUTFChars(root_path, path);

    env->DeleteGlobalRef(jobj);
    env->DeleteGlobalRef(jcls);
    env->DeleteGlobalRef(pc_cls);

    LOGI("Release");
    JNI_OnUnload(jvm, nullptr);
}

