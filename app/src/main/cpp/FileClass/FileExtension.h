//
// Created by tah9 on 2023/3/19.
//

#ifndef DEMO2_23_FILEEXTENSION_H
#define DEMO2_23_FILEEXTENSION_H


namespace ets {
    const static std::string pic_extension[5] = {".jpg", ".png", ".jpeg", ".webp", ".gif"};

    //判断文件是否是图片（根据文件名后缀）
    inline int isPicture(const std::string &name) {
        if (name.length() < 5)return 0;
        for (int i = 0; i < pic_extension->length(); ++i) {
            if (name.rfind(pic_extension[i]) != std::string::npos) return 1;
        }
        return 0;
    }
}
#endif //DEMO2_23_FILEEXTENSION_H
