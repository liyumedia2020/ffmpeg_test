//
// Created by Administrator on 2020/7/25.
//

#include "QsvDecoder.h"
#include<iostream>
#include<thread>
#include<chrono>
#include<stdio.h>
#define INBUF_SIZE 4096*10


void saveYuv420ToFile(AVFrame *frame,FILE*fOut){
    if(frame->linesize[0]!=frame->width){
        uint8_t *p = frame->data[0];
        for(int i = 0;i<frame->height;i++){
            fwrite(p, 1,frame->width, fOut);
            p+=frame->linesize[0];
        }
    }else{
        fwrite(frame->data[0], 1,frame->linesize[0]*frame->height, fOut);
    }

    if (frame->linesize[1] != frame->height) {
        uint8_t *p = frame->data[1];
        for (int i = 0; i < frame->height / 2; i++) {
            fwrite(p, 1, frame->width / 2, fOut);
            p += frame->linesize[1];
        }
        p = frame->data[2];
        for (int i = 0; i < frame->height / 2; i++) {
            fwrite(p, 1, frame->width / 2, fOut);
            p += frame->linesize[2];
        }

    } else {
        fwrite(frame->data[1], 1, frame->linesize[1] * frame->height / 2, fOut);
        fwrite(frame->data[2], 1, frame->linesize[2] * frame->height / 2, fOut);
    }
    fflush(fOut);
}
static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE*fOut)
{
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3d,width:%d,height:%d,%d,%d,%d,%I64u,%I64u\n", dec_ctx->frame_number,frame->width,frame->height,
               frame->linesize[0],frame->linesize[1],frame->linesize[2],pkt->pts,pkt->dts);
        if(dec_ctx->pix_fmt == AV_PIX_FMT_YUV420P){
            saveYuv420ToFile(frame,fOut);
        }else{
            //转换成yuv420p,再写文件
            printf("frame pixel_format:%d\n",dec_ctx->pix_fmt);
            int src_w = frame->width;
            int src_h  =frame->height;
            static AVFrame *dst_frame = nullptr;
            static struct SwsContext *sws_ctx = nullptr;
            if(sws_ctx==nullptr){
                sws_ctx = sws_getContext(src_w, src_h, dec_ctx->pix_fmt,
                                         src_w, src_h, AV_PIX_FMT_YUV420P,
                                         SWS_BILINEAR, NULL, NULL, NULL);
                dst_frame = av_frame_alloc();
                dst_frame->width = frame->width;
                dst_frame->height = frame->height;
                av_image_alloc(dst_frame->data,dst_frame->linesize,src_w,src_h,AV_PIX_FMT_YUV420P,16);
            }
            sws_scale(sws_ctx, frame->data,frame->linesize, 0, src_h, dst_frame->data, dst_frame->linesize);
            saveYuv420ToFile(dst_frame,fOut);

        }
    }
}

void QsvDecoder::Test(){
    std::cout<<"qsv decoder test"<<std::endl;
    const char *filename, *outfilename;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c= NULL;
    FILE *f;
    FILE *fOut = nullptr;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t   data_size;
    int ret;
    AVPacket *pkt;


    filename    = "c:\\b.264";
    outfilename = "c:\\out.yuv";

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    //codec = avcodec_find_decoder_by_name("h264");
    codec = avcodec_find_decoder_by_name("h264_qsv");
    //codec = avcodec_find_decoder_by_name("hevc");
    //codec = avcodec_find_decoder_by_name("hevc_qsv");
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    fOut = fopen(outfilename, "wb");
    if (!fOut) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    while (!feof(f)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if (pkt->size)
                decode(c, frame, pkt, fOut);
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if(c->frame_number>500)
            break;
    }

    /* flush the decoder */
    decode(c, frame, NULL, fOut);

    fclose(f);

    av_parser_close(parser);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}