
#子目录CMakeList

# 查找当前目录下的所有源文件，并保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
# 根据c文件产生动态库
ADD_LIBRARY(file-class SHARED ${DIR_LIB_SRCS})


set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/../jniLibs)

include_directories(${JNI_LIBS_DIR}/includes)


target_link_libraries( # Specifies the target library.
        file-class
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} ${android-lib})