cmake_minimum_required(VERSION 3.16)
project(qsv_test)

set(CMAKE_CXX_STANDARD 14)
include_directories("C:\\ffmpeg-20200724-21442a8-win32-shared\\include")
link_directories("C:\\ffmpeg-20200724-21442a8-win32-shared\\lib")
add_executable(qsv_test main.cpp QsvDecoder.cpp QsvDecoder.h QsvEncoder.cpp QsvEncoder.h TsMuxer.cpp TsMuxer.h)
target_link_libraries(qsv_test avcodec.lib avutil.lib avformat.lib swscale.lib swresample.lib)