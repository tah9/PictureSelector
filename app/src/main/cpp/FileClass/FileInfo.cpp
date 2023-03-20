#include <string>
#include <vector>

using namespace std;

//
// Created by tah9 on 2023/3/6.
//
class FileInfo {
public:
    string path;
    long time{};

    FileInfo() = default;

    FileInfo(string p, long t) : path(std::move(p)), time(t) {
    };
};

class Folder {
public:
    string name;//文件夹名
//    string path;//完整路径
    string first_path;//第一张图片路径
//    long s_time;//排序时间
    long m_time;//目录修改时间
//    vector<FileInfo> *pics = nullptr;

    Folder() =default;

    ~Folder() {
//        pics->shrink_to_fit();
//        name= nullptr;
//        first_path= nullptr;
    }
};