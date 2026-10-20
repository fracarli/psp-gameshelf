/* Startup status and preparation timing; no firmware-delay estimates. */
static int gsh_launch_active;
static unsigned long long gsh_launch_started;
static unsigned int gsh_copied_bytes;
static unsigned int gsh_rom_copies;
/* A bounded stage bar, not an estimate of firmware or emulator duration. */
static void gsh_launch_bar(unsigned int percent)
{
    void *frame; int stride,format,x,y;
    if(!gsh_launch_active)return;
    if(percent>100)percent=100;
    if(sceDisplayGetFrameBuf(&frame,&stride,&format,0)<0||!frame)return;
    for(y=129;y<143;y++)for(x=110;x<370;x++){
        int filled=x<110+(int)(percent*260/100);
        unsigned int r=filled?90:36,g=filled?190:36,b=filled?230:36;
        if(format==PSP_DISPLAY_PIXEL_FORMAT_8888)
            ((unsigned int *)frame)[y*stride+x]=r|(g<<8)|(b<<16)|0xff000000;
        else {
            unsigned short c=format==PSP_DISPLAY_PIXEL_FORMAT_565 ?
                (r>>3)|((g>>2)<<5)|((b>>3)<<11) :
                format==PSP_DISPLAY_PIXEL_FORMAT_5551 ?
                (r>>3)|((g>>3)<<5)|((b>>3)<<10)|0x8000 :
                (r>>4)|((g>>4)<<4)|((b>>4)<<8)|0xf000;
            ((unsigned short *)frame)[y*stride+x]=c;
        }
    }
}
static void gsh_launch_stage(const char *stage)
{
    if(!gsh_launch_active)return;
    gsh_launch_bar(!strcmp(stage,"Starting emulator...")?70:
                   !strcmp(stage,"Checking emulator files...")?60:5);
}
static void gsh_launch_begin(const char *name)
{
    (void)name;
    gsh_launch_active=1;gsh_copied_bytes=0;gsh_rom_copies=0;
    gsh_launch_started=sceKernelGetSystemTimeWide();
    pspDebugScreenClear();
    gsh_launch_stage("Preparing game...");
}
static void gsh_copy_progress(unsigned int copied,unsigned int total)
{
    if(!gsh_launch_active)return;
    gsh_launch_bar(5+(total?(unsigned int)((unsigned long long)copied*50/total):0));
}
static void gsh_launch_log(const char *name)
{
    char log[512];int length;SceUID f;
    unsigned int ms=(unsigned int)((sceKernelGetSystemTimeWide()-gsh_launch_started)/1000);
    length=snprintf(log,sizeof(log),"GameShelf 1.07 preparation\nROM: %s\nPreparation_ms: %u\nROM_copies: %u\nCopied_file_bytes: %u\nFirmware/emulator hand-off time is not included.\n",name,ms,gsh_rom_copies,gsh_copied_bytes);
    f=sceIoOpen("ms0:/PSP/GAME/GAMESHELF/STARTUP_LOG.TXT",PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC,0777);
    if(f>=0){sceIoWrite(f,log,length);sceIoClose(f);}
}
