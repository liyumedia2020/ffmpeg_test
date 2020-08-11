//
// Created by Administrator on 2020/7/25.
//

#ifndef QSV_TEST_QSVDECODER_H
#define QSV_TEST_QSVDECODER_H
extern "C"{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
};

class QsvDecoder {
public:
    static void Test();
};


#endif //QSV_TEST_QSVDECODER_H
