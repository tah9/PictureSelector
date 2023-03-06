#include <string>
#include <vector>

using namespace std;

//
// Created by tah9 on 2023/3/6.
//
class FileInfo {
public:
    string path;
    long time;

    FileInfo() {

    };
};

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