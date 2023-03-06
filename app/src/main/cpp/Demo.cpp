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

#include "JNI_LOG.h"

#include <ctime>

#define CALLBACK_COUNT 5
static jclass jcls;
static jclass pc_cls;
static jobject jobj;
jmethodID jCallbackMid;
jmethodID file_costruct;

long int getMs() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

class FileInfo {
public:
    string path;
    long time;

    FileInfo() {

    };
};


class MMScanner {
private:


public:
    JNIEnv *env;
    FileInfo *fileList = new FileInfo[CALLBACK_COUNT];//开辟内存
    int curFileIndex = 0;

    void startScan(const string &rootPath);

    void doCallback();

    MMScanner(JNIEnv *v) {
        env = v;
    }

    ~MMScanner() {
        LOGI("~MMScanner()");
    }
};

MMScanner *mmScanner = nullptr;

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


class Folder {
public:
    string name;//文件名名
//    string path;//完整路径
    string first_path;//第一张图片路径
    long s_time;//排序时间
    long m_time;//目录修改时间
    vector<FileInfo> *pics = nullptr;

    Folder() {

    }

    ~Folder() {
        pics->shrink_to_fit();
//        name= nullptr;
//        first_path= nullptr;
    }
};

int sortByPic(FileInfo f1, FileInfo f2) {
    return f1.time < f2.time;
}

int sortByFolder(Folder f1, Folder f2) {
    return f1.s_time < f2.s_time;
}

vector<Folder> v_folder;

void MMScanner::startScan(const std::string &rootPath) {
    long startTime = getMs();
    LOGI("MMScanner::startScan time> %ld", startTime);
    //创建目录向量
    vector<string> mDirQueue;
    mDirQueue.emplace_back(rootPath);
    /**
     * IO扫描目录（广度优先遍历），
     * 创建临时向量保存图片，遇到nomedia清空向量并停止扫描。
     * 该目录扫描结束按时间将临时向量排序，size>0 创建目录结构，将该目录压入目录向量。
     */
    while (!mDirQueue.empty()) {
        std::string curDir = mDirQueue.back();
        mDirQueue.pop_back();

        DIR *dir = opendir(curDir.c_str());
        if (dir == nullptr) {
            continue;
        }
        //循环该目录下每个文件
        struct dirent *dirp;
        vector<FileInfo> *tempFileList = new vector<FileInfo>;
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
                && string(dirp->d_name).find(".gs") == string::npos
                //忽略 Android目录
                && strcmp(dirp->d_name, "Android") != 0
                    ) {
                //将目录压入deque
                mDirQueue.emplace_back(curDir + "/" + dirp->d_name);

            } else if (dirp->d_type == DT_REG
                       && isPicture(dirp->d_name)) {
                FileInfo fileInfo;
                fileInfo.path = curDir + "/" + dirp->d_name;
                struct stat fileStat;
//                lstat(fileInfo.path.c_str(), &fileStat);
                fstatat(dirfd(dir), dirp->d_name, &fileStat, 0);
                fileInfo.time = fileStat.st_mtim.tv_sec;
                tempFileList->emplace_back(fileInfo);
            }
        }
        //该目录扫描完成
        closedir(dir);

        if (tempFileList->empty()) {
            delete tempFileList;
            continue;
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


//    for (const auto &item: v_folder){
//        LOGI("目录 %s", item.name.c_str());
//        LOGI("第一张 %s", item.first_path.c_str());
//    }


    /**
     * 将目录向量按排序时间排序，
     * 从目录向量弹出最后一个目录，取出最后一个向量（最新图片），填入图片数组，下标++，
     * 修改该目录结构排序时间为（倒数第二个向量图片的修改时间）。
     * 若目录的图片向量size==0，从目录向量内弹出该目录，将目录向量按排序时间排序，递归执行该操作。
     */

    LOGI("ScanOver time> %ld", getMs() - startTime);
    int a = 0;
//    return;
    while (!v_folder.empty() && a == 0) {
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
            a = 1;
            doCallback(); //这里把扫描到的文件信息回调到java层，500文件回调一次
            LOGI("doback time> %ld", getMs());
//                sleep(3);
        }
    }



    //所有目录扫描完成，回调剩下的小部分文件，释放内存
//    if (curFileIndex > 0) {
//        doCallback();
//    }
    LOGI("startScanOver time> %ld", getMs());

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
        env->DeleteLocalRef(path);
        env->DeleteLocalRef(java_PcBean);
    }
//
    //调用java回调方法
    env->CallVoidMethod(jobj, jCallbackMid, list_obj);
    //释放局部引用
    env->DeleteLocalRef(list_cls);
    env->DeleteLocalRef(list_obj);

//    LOGI("dobackEnd time> %ld",getMs());
//    curFileIndex=0;

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

    LOGI("Release");
    LOGI("scanOver time> %ld", getMs());
}


void instanceJObj(JNIEnv *env, jobject thiz) {
    LOGI("instanceJObj time> %ld", getMs());


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




