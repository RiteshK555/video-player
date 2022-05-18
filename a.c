//for encoding and decoding data (video as well as audio)
#include "libavcodec/avcodec.h"
//multiplexing and demultiplexing data
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
int main(int argc,char **argv){
    //structure pointer
    AVFormatContext *pFormatCtx = NULL;
    //string is a character array
    //file name 
    const char *filename = "test.mp4";
    //open input container
    //arguments - 1.avformatcontext 2.filename 3.format(if not specified,it will be autodetected) 
    //this auto detects the format of the file, reads the header info and allocates the data to the context
    if(avformat_open_input(&pFormatCtx,filename,NULL,NULL)<0){
        printf("couldn't open file\n");
        return -1;
    }
    //some formats don't have a header(or some data in context may be missing), so we need to call this function.
    if(avformat_find_stream_info(pFormatCtx,NULL)<0) {
        printf("couldn't find stream info\n");
        return -1;
    }
    //print some info present in the context
    //arguments 1.context 2.index of stream to dump info abt 3.url of file 4.whether context is input or output
    //av_dump_format(pFormatCtx,0,filename,0);
    
    //iterating through streams
    int videoStream = -1;AVCodecContext *pCodecCtxOrig=NULL,*pCodecCtx=NULL;
    for(int i=0;i<pFormatCtx->nb_streams;i++){
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
            break;
        }
    }
    if(videoStream == -1)return -1;
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
}