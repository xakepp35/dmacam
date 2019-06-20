#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include <libcamera/libcamera.h>

#include <iostream>

int clamp(int x, int m0, int m1) {
    return x > m1 ? m1 : (x < m0 ? m0 : x);
}

void YUV444toRGB888(uint8_t* r, uint8_t* g, uint8_t* b, int y, int v, int u) {
    y -= 16;
    u -= 128;
    v -= 128;
    int cb = u;
    int cr = v;
    //int rTmp = y + (1.370705 * v);
    //int gTmp = y - (0.698001 * v) - (0.337633 * u);
    //int bTmp = y + (1.732446 * u);
    int rTmp = y + (1.4065 * cr);
    int gTmp = y - (0.3455 * cb) - (0.7169 * cr);
    int bTmp = y + (1.7790 * cb);
    *r = clamp(rTmp, 0, 255);
    *g = clamp(gTmp, 0, 255);
    *b = clamp(bTmp, 0, 255);
}

void arr_yuv422_to_rgb888(uint8_t* rgbArray, const uint8_t* yuv422Array, size_t w, size_t h) {
    size_t numSamples = w*h;
    for(size_t i = 0; i < numSamples/2; ++i) {
        int y1 = yuv422Array[i*4+0];
        int u  = yuv422Array[i*4+1];
        int y2 = yuv422Array[i*4+2];
        int v  = yuv422Array[i*4+3];
        int8_t r1, g1, b1, r2, g2, b2;
        YUV444toRGB888(rgbArray+i*6+0, rgbArray+i*6+1, rgbArray+i*6+2, y1, u, v);
        YUV444toRGB888(rgbArray+i*6+3, rgbArray+i*6+4, rgbArray+i*6+5, y2, u, v);
    }
}

/*
img = (unsigned char *)malloc(3*w*h);
memset(img,0,3*w*h);

for(int i=0; i<w; i++)
{
    for(int j=0; j<h; j++)
    {
        x=i; y=(h-1)-j;
        r = red[i][j]*255;
        g = green[i][j]*255;
        b = blue[i][j]*255;
        if (r > 255) r=255;
        if (g > 255) g=255;
        if (b > 255) b=255;
        img[(x+y*w)*3+2] = (unsigned char)(r);
        img[(x+y*w)*3+1] = (unsigned char)(g);
        img[(x+y*w)*3+0] = (unsigned char)(b);
    }
}
*/

void file_buffer_write(const char* fileName, const void* dataBuffer, size_t dataSize) {
    FILE* fOut = fopen(fileName, "wb");
    int writeSize = fwrite(dataBuffer, 1, dataSize, fOut);
    if(writeSize != dataSize) {
        printf("writeSize!=dataSize\n");
    }
    //fflush(fOut);
    fclose(fOut);
}

void write_bmp(const char* bmpName, const uint8_t* img, size_t w, size_t h) {
    FILE *f;
    int filesize = 54 + 3*w*h;  //w is your image width, h is image height, both int

    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[3] = {0,0,0};

    bmpfileheader[ 2] = (unsigned char)(filesize    );
    bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);

    bmpinfoheader[ 4] = (unsigned char)(       w    );
    bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
    bmpinfoheader[ 6] = (unsigned char)(       w>>16);
    bmpinfoheader[ 7] = (unsigned char)(       w>>24);
    bmpinfoheader[ 8] = (unsigned char)(       h    );
    bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
    bmpinfoheader[10] = (unsigned char)(       h>>16);
    bmpinfoheader[11] = (unsigned char)(       h>>24);

    f = fopen(bmpName, "wb");
    fwrite(bmpfileheader, 1, 14, f);
    fwrite(bmpinfoheader, 1, 40, f);
    for(size_t i=0; i<h; i++) {
        fwrite(img+(w*(h-i-1)*3),3,w,f);
        fwrite(bmppad,1,(4-(w*3)%4)%4,f);
    }
    fclose(f);
}



int main(int argc, const char* argv[]) {
    auto cm_ = libcamera::CameraManager::instance();
    cm_->start();
    auto camera_ = cm_->cameras()[0];
    if (camera_->acquire()) {
        std::cout << "Failed to acquire camera" << std::endl;
        camera_.reset();
        cm_->stop();
        return -EINVAL;
    }

    //std::shared_ptr<libcamera::Camera> camera_;
	//std::unique_ptr<libcamera::CameraConfiguration> config_;
	//libcamera::EventLoop *loop_;

    FILE* fInput = fopen(argv[1], "rb");
    fseek(fInput, 0L, SEEK_END);
    size_t fileSize = ftell(fInput);
    fseek(fInput, 0L, SEEK_SET);
    auto fileBuf = (uint8_t*)malloc(fileSize);
    size_t readSize = fread(fileBuf, 1, fileSize, fInput);
    fclose(fInput);
    if(readSize != fileSize) {
        printf("fileSize != readSize\n");
        return -1;
    }
    size_t w = 1280;
    size_t h = 1024;
    size_t numSamples = w*h;
    auto rgbArray = (uint8_t*)malloc(numSamples*3);
    arr_yuv422_to_rgb888(rgbArray, fileBuf, w, h);
    free(fileBuf);

    write_bmp(argv[2], rgbArray, w, h);
    free(rgbArray);

    /*   file_buffer_write(argv[2], rgbArray, numSamples*3); */
    
    return 0;
}