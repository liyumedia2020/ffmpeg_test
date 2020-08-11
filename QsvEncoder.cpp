//
// Created by Administrator on 2020/7/25.
//

#include "QsvEncoder.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
};

void encodeVideo(AVFrame*frame,AVCodecContext*enc_ctx,AVPacket *pkt,FILE*fOut){
    int ret;
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }
        printf("Write packet pts:%I64u,dts:%I64u (size=%5d)\n", pkt->pts, pkt->dts,pkt->size);
        fwrite(pkt->data, 1, pkt->size, fOut);
        av_packet_unref(pkt);
    }
}

int  initCodec(AVCodecContext** codec_ctx,char*codec_name,int src_w,int src_h){
    AVCodec *codec = NULL;
    AVCodecContext* c = NULL;
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        return -1;
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        return-1;
    }

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = src_w;
    c->height = src_h;
    /* frames per second */
    //c->time_base = (AVRational){1, 25};
    c->time_base.num = 1;
    c->time_base.den = 25;
    //c->framerate = (AVRational){25, 1};
    c->framerate.num =25;
    c->framerate.den =1;
    c->gop_size = 100;
    c->max_b_frames = 1;

    if(strcmp(codec_name,"h264_qsv")==0||strcmp(codec_name,"hevc_qsv")==0){
        c->pix_fmt = AV_PIX_FMT_NV12;
        av_opt_set(c->priv_data, "preset", "slow", 0);
        av_opt_set_int(c->priv_data, "bf", 4, 0);
        c->bit_rate = 3400000;
    }else{
        c->pix_fmt = AV_PIX_FMT_YUV420P;
        av_opt_set(c->priv_data, "preset", "fast", 0);
        //av_opt_set_int(c->priv_data, "crf", 30, 0);
        //av_opt_set_int(c->priv_data, "crf_max", 40, 0);
        c->bit_rate = 1500000;
    }

    /* open it */
    int ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        printf("Could not open codec \n");
        return -1;
    }
    *codec_ctx = c;
    return 0;
}
void QsvEncoder::Test() {

    char *src_filename = "c:\\out.yuv";
    char *dtst_filename = "c:\\out.265";
    FILE *fIn = nullptr;
    FILE *fOut = nullptr;
    AVFrame *frame = nullptr;
    int src_w = 1920;
    int src_h = 1080;

    AVCodecContext *c= NULL;
    AVPacket *pkt = NULL;
    //init codec
    char*codec_name="hevc_qsv";
    //char*codec_name="libx265";
    if(initCodec(&c,codec_name,src_w,src_h)!=0){
        return;
    }
    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);
    fIn = fopen(src_filename, "rb");
    if (!fIn) {
        fprintf(stderr, "Could not open %s\n", src_filename);
        exit(1);
    }

    fOut = fopen(dtst_filename, "wb");
    if (!fOut) {
        fprintf(stderr, "Could not open %s\n", dtst_filename);
        exit(1);
    }
    frame = av_frame_alloc();
    frame->width = src_w;
    frame->height =src_h;
    frame->format = AV_PIX_FMT_YUV420P;
    //给frame分配空间
    int ret = av_frame_get_buffer(frame, 16);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }
    int ySize = src_w*src_h;
    int uvSize = ySize/4;
    //一直读数据
    static int frameIndex = 1;
    while(true){
        //read y
        if(fread(frame->data[0],1,ySize,fIn)<ySize){
            fprintf(stderr, "read y data error\n");
            break;
        }
        if(fread(frame->data[1],1,uvSize,fIn)<uvSize){
            fprintf(stderr, "read u data error\n");
            break;
        }
        if(fread(frame->data[2],1,uvSize,fIn)<uvSize){
            fprintf(stderr, "read v data error\n");
            break;
        }
        printf("read frame:%d\n",frameIndex++);
        if(strcmp(codec_name,"h264_qsv")==0||strcmp(codec_name,"hevc_qsv")==0){
            static AVFrame *dst_frame = nullptr;
            static struct SwsContext *sws_ctx = nullptr;
            if(sws_ctx==nullptr){
                sws_ctx = sws_getContext(src_w, src_h, AV_PIX_FMT_YUV420P,
                                         src_w, src_h, c->pix_fmt,
                                         SWS_BILINEAR, NULL, NULL, NULL);
                dst_frame = av_frame_alloc();
                dst_frame->width = frame->width;
                dst_frame->height = frame->height;
                dst_frame->format = c->pix_fmt;
                av_image_alloc(dst_frame->data,dst_frame->linesize,src_w,src_h,c->pix_fmt,16);
            }
            sws_scale(sws_ctx, frame->data,frame->linesize, 0, src_h, dst_frame->data, dst_frame->linesize);
            encodeVideo(dst_frame,c,pkt,fOut);
        }else{
            encodeVideo(frame,c,pkt,fOut);
        }
    }
    printf("read file eof,close it\n");
    fclose(fIn);
    fclose(fOut);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}