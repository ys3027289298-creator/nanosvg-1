#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "/tmp/snap-stb/stb_image.h"
int main(int argc, char** argv){
    int w,h,c;
    unsigned char* d = stbi_load(argv[1], &w, &h, &c, 4);
    if(!d){printf("load fail\n");return 1;}
    int n=0,x0=w,y0=h,x1=-1,y1=-1;
    for(int i=0;i<w*h;i++) if(d[i*4+3]){n++;int x=i%w,y=i/w;
        if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;}
    printf("%d (%d,%d,%d,%d)\n", n, x0,y0,x1,y1);
    return 0;
}
