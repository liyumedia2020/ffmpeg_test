#include <iostream>
#include "QsvDecoder.h"
#include "QsvEncoder.h"
#include "TsMuxer.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    //QsvDecoder::Test();
    //QsvEncoder::Test();
    //TsMuxer::TestWriteToMem();
    TsMuxer::PushToNetwork();
    getchar();
    return 0;
}
