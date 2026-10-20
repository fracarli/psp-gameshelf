/* Startup status and preparation timing; no firmware-delay estimates. */
static int gsh_launch_active;
static unsigned long long gsh_launch_started;
static unsigned int gsh_copied_bytes;
static unsigned int gsh_rom_copies;
static void gsh_launch_stage(const char *stage)
{
    if(!gsh_launch_active)return;
    pspDebugScreenSetXY(0,5);
    pspDebugScreenPrintf("%-62s\n",gsh_text(stage));
    sceDisplayWaitVblankStart();
}
static void gsh_launch_begin(const char *name)
{
    gsh_launch_active=1;gsh_copied_bytes=0;gsh_rom_copies=0;
    gsh_launch_started=sceKernelGetSystemTimeWide();
    pspDebugScreenClear();
    pspDebugScreenPrintf("GameShelf 1.06\n\n%.60s\n",name);
    gsh_launch_stage("Preparing game...");
}
static void gsh_copy_progress(unsigned int copied,unsigned int total)
{
    if(!gsh_launch_active)return;
    pspDebugScreenSetXY(0,6);
    pspDebugScreenPrintf(gsh_text("Copying ROM: %3u%%                         \n"),
                         total?(unsigned int)((unsigned long long)copied*100/total):0);
}
static void gsh_launch_log(const char *name)
{
    char log[512];int length;SceUID f;
    unsigned int ms=(unsigned int)((sceKernelGetSystemTimeWide()-gsh_launch_started)/1000);
    length=snprintf(log,sizeof(log),"GameShelf 1.06 preparation\nROM: %s\nPreparation_ms: %u\nROM_copies: %u\nCopied_file_bytes: %u\nFirmware/emulator hand-off time is not included.\n",name,ms,gsh_rom_copies,gsh_copied_bytes);
    f=sceIoOpen("ms0:/PSP/GAME/GAMESHELF/STARTUP_LOG.TXT",PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC,0777);
    if(f>=0){sceIoWrite(f,log,length);sceIoClose(f);}
}
