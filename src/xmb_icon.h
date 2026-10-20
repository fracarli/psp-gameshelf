/* Convert a square RGB565 XMB cover to a PSP XMB ICON0 PNG.
 * Uses an uncompressed zlib block: no PNG decoder or new PSP dependencies. */
#define XMB_ICON_WIDTH 80
#define XMB_ICON_HEIGHT 80
#define XMB_ICON_RAW ((XMB_ICON_WIDTH * 3 + 1) * XMB_ICON_HEIGHT)
static void png_be32(unsigned char *p, unsigned int v)
{
    p[0]=v>>24; p[1]=v>>16; p[2]=v>>8; p[3]=v;
}
static unsigned int png_crc(const unsigned char *p, unsigned int length)
{
    unsigned int crc=~0u, i, bit;
    for(i=0;i<length;i++) {
        crc^=p[i];
        for(bit=0;bit<8;bit++) crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);
    }
    return ~crc;
}
static unsigned int png_chunk(unsigned char *p, const char *type,
                               const unsigned char *data, unsigned int size)
{
    png_be32(p,size); memcpy(p+4,type,4);
    if(size) memcpy(p+8,data,size);
    png_be32(p+8+size,png_crc(p+4,size+4));
    return size+12;
}
static unsigned char *xmb_cover_png(const char *rom, int mode, unsigned int *length)
{
    unsigned char *texture=NULL, *raw=NULL, *compressed=NULL, *png=NULL;
    char name[256], path[384];
    unsigned int x,y,pixel,offset,a=1,b=0,pos=8;
    unsigned int source_width=80, source_height=80, texture_size=80*80*2;
    unsigned char ihdr[13]={0};
    SceIoStat st;
    SceUID f;
    strncpy(name,rom,sizeof(name)-1); name[255]=0;
    { char *dot=strrchr(name,'.'); if(dot) *dot=0; }
    snprintf(path,sizeof(path),"ms0:/PSP/GAME/GAMESHELF/boxart/%s/%s.xmb.gsh",mode==3?"gba":"nes",name);
    *length=0;
    if(sceIoGetstat(path,&st)<0 || st.st_size!=texture_size) {
        /* Compatibility with cover caches prepared before version 1.02. */
        snprintf(path,sizeof(path),"ms0:/PSP/GAME/GAMESHELF/boxart/%s/%s.gsh",mode==3?"gba":"nes",name);
        source_width=128; source_height=160;texture_size=source_width*source_height*2;
        if(sceIoGetstat(path,&st)<0 || st.st_size!=texture_size) return NULL;
    }
    texture=malloc(texture_size); raw=malloc(XMB_ICON_RAW);
    compressed=malloc(XMB_ICON_RAW+11); png=malloc(XMB_ICON_RAW+80);
    if(!texture || !raw || !compressed || !png) goto fail;
    f=sceIoOpen(path,PSP_O_RDONLY,0);
    if(f<0) goto fail;
    offset=sceIoRead(f,texture,texture_size); sceIoClose(f);
    if(offset!=texture_size) goto fail;
    for(y=0;y<XMB_ICON_HEIGHT;y++) {
        unsigned int row=y*(XMB_ICON_WIDTH*3+1);
        raw[row]=0; /* Square icon, without an opaque rectangular frame. */
        for(x=0;x<XMB_ICON_WIDTH;x++) {
            unsigned char *rgb=raw+row+1+x*3;
            unsigned int crop=source_width<source_height?source_width:source_height;
            unsigned int sx=(source_width-crop)/2+x*crop/XMB_ICON_WIDTH;
            unsigned int sy=(source_height-crop)/2+y*crop/XMB_ICON_HEIGHT;
            offset=(sy*source_width+sx)*2;
            pixel=texture[offset]|(texture[offset+1]<<8);
            rgb[0]=((pixel>>11)&31)*255/31;
            rgb[1]=((pixel>>5)&63)*255/63;
            rgb[2]=(pixel&31)*255/31;
        }
    }
    compressed[0]=0x78;compressed[1]=0x01;compressed[2]=1;
    compressed[3]=XMB_ICON_RAW&255;compressed[4]=XMB_ICON_RAW>>8;
    compressed[5]=(~XMB_ICON_RAW)&255;compressed[6]=((~XMB_ICON_RAW)>>8)&255;
    memcpy(compressed+7,raw,XMB_ICON_RAW);
    for(x=0;x<XMB_ICON_RAW;x++) { a=(a+raw[x])%65521;b=(b+a)%65521; }
    png_be32(compressed+7+XMB_ICON_RAW,(b<<16)|a);
    memcpy(png,"\x89PNG\r\n\x1a\n",8);
    png_be32(ihdr,XMB_ICON_WIDTH);png_be32(ihdr+4,XMB_ICON_HEIGHT);
    ihdr[8]=8;ihdr[9]=2;
    pos+=png_chunk(png+pos,"IHDR",ihdr,13);
    pos+=png_chunk(png+pos,"IDAT",compressed,XMB_ICON_RAW+11);
    pos+=png_chunk(png+pos,"IEND",NULL,0);
    *length=pos;
    free(texture);free(raw);free(compressed);return png;
fail:
    free(texture);free(raw);free(compressed);free(png);return NULL;
}

static unsigned int png_read_be32(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];
}
/* PIC1 is an optional native XMB background, prepared from the same cover. */
static unsigned char *xmb_background_png(const char *rom, int mode, unsigned int *length)
{
    char name[256], path[384];
    SceIoStat st;
    SceUID file;
    unsigned char *png;
    unsigned int size, pos=8, has_data=0;
    *length=0;
    strncpy(name,rom,sizeof(name)-1);name[255]=0;
    { char *dot=strrchr(name,'.');if(dot)*dot=0; }
    snprintf(path,sizeof(path),"ms0:/PSP/GAME/GAMESHELF/boxart/%s/%s.pic1.png",mode==3?"gba":"nes",name);
    if(sceIoGetstat(path,&st)<0 || st.st_size<57 || st.st_size>524288) return NULL;
    size=st.st_size;png=malloc(size);
    if(!png)return NULL;
    file=sceIoOpen(path,PSP_O_RDONLY,0);
    if(file<0) { free(png);return NULL; }
    pos=sceIoRead(file,png,size);sceIoClose(file);
    if(pos!=size || memcmp(png,"\x89PNG\r\n\x1a\n",8)!=0 ||
       png_read_be32(png+8)!=13 || memcmp(png+12,"IHDR",4)!=0 ||
       png_read_be32(png+16)!=480 || png_read_be32(png+20)!=272 ||
       png[24]!=8 || png[25]!=2 || png[26] || png[27] || png[28]) goto invalid;
    pos=8;
    while(pos<=size-12) {
        unsigned int chunk=png_read_be32(png+pos);
        if(chunk>size-pos-12 || png_crc(png+pos+4,chunk+4)!=png_read_be32(png+pos+8+chunk)) goto invalid;
        if(memcmp(png+pos+4,"IDAT",4)==0)has_data=1;
        if(memcmp(png+pos+4,"IEND",4)==0) {
            if(chunk || pos+12!=size || !has_data)goto invalid;
            *length=size;return png;
        }
        pos+=chunk+12;
    }
invalid:
    free(png);return NULL;
}
