
#include <jni.h>
#include "ScanToJni.cpp"


#define JNIREG_CLASS "com/school/demo2_23/GalleryMain"  //Java类的路径：包名+类名
#define NUM_METHOES(x) ((int) (sizeof(x) / sizeof((x)[0]))) //获取方法的数量
static JNINativeMethod method_table[] = {
        // 第一个参数a 是java native方法名，
        // 第二个参数 是native方法参数,括号里面是传入参的类型，外边的是返回值类型，
        // 第三个参数 是c/c++方法参数,括号里面是返回值类型，建议填void*
        {"native_scan", "(Ljava/lang/String;)V", (void *) scan}
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
JNIEXPORT jint

JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGI("JNI_OnLoad== time> %ld", getMs());
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
//    assert(env != NULL);

    // 注册native方法
    if (!registerMethods(env, JNIREG_CLASS, method_table, NUM_METHOES(method_table))) {
        return JNI_ERR;
    }
    LOGI("JNI_OnLoad");
    ::jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI("JNI_OnUnload");
}




