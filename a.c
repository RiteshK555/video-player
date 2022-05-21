//for encoding and decoding data (video as well as audio)
#include "libavcodec/avcodec.h"
//multiplexing and demultiplexing data
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

//saveframe fn
void SaveFrame(AVFrame *pFrame,int width, int height, int iFrame){
    char filename[32];
    //name of the file
    sprintf(filename,"frame%d.ppm",iFrame);
    //b in wb stands for binary
    FILE *pFile = fopen(filename,"wb");
    if(pFile==NULL)return;
    //p6 means it the image format is byte format
    //here we are writing the header (to follow format)
    fprintf(pFile,"P6\n%d %d\n255\n",width,height);
    //write the pixel data
    for(int y=0;y<height;y++){
        //write data
        //1. pointer to the start of each line of frame
        //2. size of each write
        fwrite(pFrame->data[0]+y*pFrame->linesize[0],1,width*3,pFile);
    }
    fclose(pFile);
}

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
    int videoStream = -1;AVCodecParameters *pCodecPar=NULL;
    for(int i=0;i<pFormatCtx->nb_streams;i++){
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
            break;
        }
    }
    if(videoStream == -1)return -1;
    pCodecPar = pFormatCtx->streams[videoStream]->codecpar;

    //decoder using codec id
    AVCodec *pDec = avcodec_find_decoder(pCodecPar->codec_id);
    if(pDec==NULL){
        //fprintf used to output file data
        fprintf(stderr,"unsupported codec\n"); 
        return -1;
    }
    //decoder context
    //sets the decoder context feilds to codec specified defaults
    AVCodecContext *pCodecCtx =  avcodec_alloc_context3(pDec);
    if(pCodecCtx==NULL){
        fprintf(stderr,"couldn't allocate codec context\n");
        return -1;
    }
    //copy params
    if(avcodec_parameters_to_context(pCodecCtx,pCodecPar)<0){
        fprintf(stderr,"couldn't copy codec parameters\n");
        return -1;
    }
    //initialize decoder context to use given decoder
    if(avcodec_open2(pCodecCtx,pDec,NULL)<0){
        fprintf(stderr,"couldn't open codec\n");
        return -1;
    }
    //frame 
    //sets its values to defaults
    AVFrame *pFrame = av_frame_alloc();
    if(pFrame==NULL){
        fprintf(stderr,"couldn't allocate pframe\n");
        return -1;
    }
    //allocating new frame(we will be outputing in rgb format)
    AVFrame *pFrameRGB = av_frame_alloc();
    if(pFrameRGB==NULL){
        fprintf(stderr,"couldn't allocate pframeRGB\n");
        return -1;
    }
    //we need to store raw bytes of the frame
    uint8_t *buffer = NULL;
    //number of bytes needed
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height,1);
    // printf("%d\n",numBytes);
    if(numBytes<0){
        fprintf(stderr,"couldn't get buffer size\n");
        return -1;
    }
    //allocate the bytes to buffer
    buffer = (uint8_t *)av_malloc(numBytes*(sizeof(uint8_t)));
    //fill the frame
    int ret = av_image_fill_arrays(pFrameRGB->data,pFrameRGB->linesize,buffer,AV_PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height,1);
    if(ret<0){
        fprintf(stderr,"couldn't fill frame\n");
        return -1;
    }

    struct SwsContext *sws_ctx = NULL;
    int frameFinished;
    //packet contains compressed frame data
    //in video it generally contains one frame
    AVPacket packet;
    //context necessary for sws_scale
    sws_ctx = sws_getContext(
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,NULL,NULL
    );
    int i=0;
    while(av_read_frame(pFormatCtx,&packet)==0){
        //is this packet from video stream?
        if(packet.stream_index == videoStream){
            //decode the packet to a frame
            ret = avcodec_send_packet(pCodecCtx,&packet);
            if(ret<0){
                fprintf(stderr,"couldn't send packet\n");
                return -1;
            }
            ret = avcodec_receive_frame(pCodecCtx,pFrame);
            //the frame is in native format we need to convert it into our rgb
            if(ret<0){
                fprintf(stderr,"couldn't receive frame\n");
                return -1;
            }
            sws_scale(
                sws_ctx,(const uint8_t * const *)pFrame->data,
                pFrame->linesize,
                0,
                pCodecCtx->height,
                pFrameRGB->data,
                pFrameRGB->linesize
            );
        }
        if(++i<=5){
            //5 frames
            SaveFrame(pFrameRGB,pCodecCtx->width,pCodecCtx->height,i);
        }
        //wipe and set default values
        av_packet_unref(&packet);
    }
    //free rgb
    av_free(buffer);
    av_free(pFrameRGB);
    //free default frame
    av_free(pFrame);
    //close codecs
    avcodec_close(pCodecCtx);
    //free params
    avcodec_parameters_free(pCodecPar);
    //close format ctx
    avformat_free_context(pFormatCtx);
}