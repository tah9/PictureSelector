#include <iostream>
#include <utility>
#include <vector>
#include <dirent.h>
#include <jni.h>
#include <string>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

#include "demo2_23.h"

#include <ctime>

#define CALLBACK_COUNT 50
static jclass jcls;
static jclass pc_cls;
static jobject jobj;
jmethodID jCallbackMid;
jmethodID file_costruct;
long sTime;

long start, end;

int i, j, k;
long a;


class FileInfo {
public:
    string path;
    long time;

    FileInfo() {

    };
};


class MMScanner {
public:
    JNIEnv *env;
    FileInfo *fileList = new FileInfo[CALLBACK_COUNT];//开辟内存，多个位置缓冲

    void startScan(const string &rootPath);

    void doCallback();

    void Release();

    MMScanner(JNIEnv *v) {
        env = v;
    }

    ~MMScanner() {
        LOGI("~MMScanner()");
    }
};

vector<string> mDirQueue;
list<FileInfo> tempFileList;
int curFileIndex = 0;
MMScanner *mmScanner = nullptr;

int isPicture(string name) {
    string ends[5] = {".jpg", ".png", ".jpeg", ".webp", ".gif"};
    for (int i = 0; i < 5; i++) {
        if (name.find(ends[i]) != string::npos) {
            return 1;
        }
    }
    return 0;
}


void MMScanner::startScan(const std::string &rootPath) {

    mDirQueue.push_back(rootPath);
    while (!mDirQueue.empty()) {
        std::string curDir = mDirQueue.back();
        mDirQueue.pop_back();

        DIR *dir = opendir(curDir.c_str());
        if (dir == nullptr) {
            LOGI("dir is NULL =%s", curDir.c_str());
            continue;
        }
        //循环该目录下每个文件
        struct dirent *dirp;
        while ((dirp = readdir(dir)) != nullptr) {
            //忽略文件目录中前两个文件.和..
            if (strcmp(dirp->d_name, ".") == 0
                || strcmp(dirp->d_name, "..") == 0) {
                continue;
            }

                //目录内有.nomedia文件，不扫描（安卓缓存文件夹声明标准）
                /**
                 * 有的目录有非用户媒体的声明文件（.nomedia），该声明文件在目录内出现的位置不固定，
//                 * 所以每次扫描目录都要记录下标，
//                 * 扫描到声明文件时，清除文件集合内的该目录文件。
                 非媒体下标和回调阈值冲突，废弃，使用deque
                 */
            else if (strcmp(dirp->d_name, ".nomedia") == 0) {
                LOGI("》》%s", (curDir + "/" + dirp->d_name).c_str());
                // noMediaIndex和CallBackCount逻辑有bug
//                while (--noMediaFileIndex > 0) {
//                    delete &fileList[--curFileIndex];
//                }
//                tempFileList.clear();
                break;
            }


            if (dirp->d_type == DT_DIR
                //忽略 名字为.XXX格式的目录（缓存文件夹）
                && string(dirp->d_name).find('.') == string::npos) {
                //将目录压入deque
                mDirQueue.push_back(curDir + "/" + dirp->d_name);

            } else if (dirp->d_type == DT_REG
                       && isPicture(dirp->d_name)) {


//                continue;
//                noMediaFileIndex++;
                FileInfo *fileInfo = &fileList[curFileIndex++];
                fileInfo->path = curDir + "/" + dirp->d_name;

                struct stat fileStat;
                lstat(fileInfo->path.c_str(), &fileStat);
//                int AT_SYMLINK_NOFOLLOW = 0;
//                fstatat(dirfd(dir), dirp->d_name, &fileStat, AT_SYMLINK_NOFOLLOW);
                fileInfo->time = fileStat.st_mtim.tv_sec;

                if (curFileIndex >= CALLBACK_COUNT) {
                    doCallback(); //这里把扫描到的文件信息回调到java层，500文件回调一次
//                sleep(3);
                }
            }
        }
        //该目录扫描完成
//        noMediaFileIndex = 0;

        closedir(dir);
    }

    //所有目录扫描完成，回调剩下的小部分文件，释放内存
    if (curFileIndex > 0) {
        doCallback();
    }
    LOGI("结束时间戳%ld", std::time(nullptr));
    LOGI("消耗时间%ld", std::time(nullptr) - sTime);

    long end = clock();
    LOGI("clock-end%ld", end);
    LOGI("clock-spend%ld", end - start);

}


void MMScanner::doCallback() {


    //获得ArrayList类引用，结束后释放
    jclass list_cls = env->FindClass("java/util/ArrayList");

    if (list_cls == NULL) {
        cout << "listcls is null \n";
    }
    jmethodID list_costruct = env->GetMethodID(list_cls, "<init>", "()V"); //获得集合构造函数Id

    //创建list局部引用，结束后释放
    jobject list_obj = env->NewLocalRef(
            env->NewObject(list_cls, list_costruct)); //创建一个Arraylist集合对象
    //或得Arraylist类中的 add()方法ID，其方法原型为： boolean add(Object object) ;
    jmethodID list_add = env->GetMethodID(list_cls, "add", "(Ljava/lang/Object;)Z");
//    jmethodID list_clear = env->GetMethodID(list_cls, "clear", "()V");

    //根据时间排序
//    sort(imgVector.begin(), imgVector.end(), comp);

//    jvm->AttachCurrentThread(reinterpret_cast<JNIEnv **>(reinterpret_cast<void **>(&env)), nullptr);
//    jvm->AttachCurrentThread(&env, 0); //绑定当前线程，获取当前线程的JNIEnv
    FileInfo *info;
    while (curFileIndex > 0) {

        info = &fileList[--curFileIndex];
        jstring path = env->NewStringUTF(info->path.c_str());
        jlong time = info->time;
        //构造一个javabean文件对象
        jobject java_PcBean = env->NewObject(pc_cls, file_costruct,
                                             path, time);

        //执行Arraylist类实例的add方法，添加一个对象
        env->CallBooleanMethod(list_obj, list_add, java_PcBean);

        //释放局部引用
//        java_PcBean= nullptr;
//        env->DeleteLocalRef((jobject)oo);
        env->DeleteLocalRef(path);
        env->DeleteLocalRef(java_PcBean);
    }
//
    //调用java回调方法
    env->CallVoidMethod(jobj, jCallbackMid, list_obj);
    //释放局部引用
//    env->CallVoidMethod(list_obj,list_clear);
    env->DeleteLocalRef(list_cls);
    env->DeleteLocalRef(list_obj);
//    jvm->DetachCurrentThread();

}

/**
 * 动态注册的方法一定要有  JNIEnv env, jobject thiz 两个参数
 * @param env
 * @param obj
 * @param root_path
 */
void scan(JNIEnv *env, jobject thiz, jstring root_path) {
    // TODO: implement scan()
    mmScanner = new MMScanner(env);
    auto path = env->GetStringUTFChars(root_path, nullptr);
    mmScanner->startScan(path);
    //释放字符串，放入jstring和env创建的字符串
    env->ReleaseStringUTFChars(root_path, path);

    env->DeleteGlobalRef(jobj);
    env->DeleteGlobalRef(jcls);
    env->DeleteGlobalRef(pc_cls);

//    for (int i = 0; i < CALLBACK_COUNT; ++i) {
//        delete &mmScanner->fileList[i].path;
//    }
//    delete mmScanner->env;
//    delete mmScanner;
    LOGI("Release");
}


void instanceJObj(JNIEnv *env, jobject thiz) {
    sTime = std::time(nullptr);
    LOGI("开始时间戳%ld", sTime);
    //start time
    start = clock();
    LOGI("clock-start%ld", start);


    jobj = env->NewGlobalRef(thiz);
    jcls = (jclass) env->NewGlobalRef(env->FindClass("com/school/demo2_23/MainActivity"));
//    jmethodID mainId = env->GetMethodID(jcls, "<init>", "()V");
//    jobj = env->NewGlobalRef(env->NewObject(jcls, mainId));

    //获取回调方法ID
    jCallbackMid = env->GetMethodID(jcls, "nativeCallback", "(Ljava/util/ArrayList;)V");


    pc_cls = (jclass) (env->NewGlobalRef(
            env->FindClass("com/school/demo2_23/PcPathBean")));//获得类引用
    //获得该类型的构造函数  函数名为 <init> 返回类型必须为 void 即 V
    file_costruct = env->GetMethodID(pc_cls, "<init>", "(Ljava/lang/String;J)V");

    jmethodID instance_finish = env->GetMethodID(jcls, "instanceNative_finish", "()V");
    env->CallVoidMethod(jobj, instance_finish);
}

#define JNIREG_CLASS "com/school/demo2_23/MainActivity"  //Java类的路径：包名+类名
#define NUM_METHOES(x) ((int) (sizeof(x) / sizeof((x)[0]))) //获取方法的数量
static JNINativeMethod method_table[] = {
        // 第一个参数a 是java native方法名，
        // 第二个参数 是native方法参数,括号里面是传入参的类型，外边的是返回值类型，
        // 第三个参数 是c/c++方法参数,括号里面是返回值类型，建议填void*
        {"native_scan",    "(Ljava/lang/String;)V", (void *) scan},
        {"instanceNative", "()V",                   (void *) instanceJObj},

};

static int registerMethods(JNIEnv *env, const char *className,
                           JNINativeMethod *gMethods, int numMethods) {
    jclass clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    //注册native方法
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
/**
 * 初始化回调所需方法，类，静态处理，防止GC回收
 * 在android ndk编程时，要使用到.so文件，so文件使用c语言编写的。当我在c文件中调用java类时，第一次调用时没问题的，但第二次调用的时候就失败了。上网搜了很多资料，大概原因是在jni中，使用指针指向某一个java对象的时候，由于android的垃圾回收机制（Garbage Collector)，如果java对象被回收的话，那么指针指向的对象就会为空或者不存在，从而提示JNI ERROR:accessed stale(陈旧的，落后的） local reference 大概的意思就是变量已经不存在了。所以要解决这个问题，就要求把java对象定义成静态的，这样可以避免被被回收（在Android4.0以后，静态变量也会被回收，但概率较小），从而导致错误的产生。
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != NULL);

    // 注册native方法
    if (!registerMethods(env, JNIREG_CLASS, method_table, NUM_METHOES(method_table))) {
        return JNI_ERR;
    }
    LOGI("JNI_OnLoad");

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI("JNI_OnUnload");
}




