#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <psputility_sysparam.h>
#include "../src/language.h"
#include "install.h"
PSP_MODULE_INFO("GameShelfSetup",0,1,9);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
static volatile int exit_requested;
static int exit_callback(int a,int b,void *data)
{
    (void)a;(void)b;(void)data;exit_requested=1;return 0;
}
static int callback_thread(SceSize args,void *data)
{
    int callback;(void)args;(void)data;
    callback=sceKernelCreateCallback("GameShelfSetupExit",exit_callback,NULL);
    sceKernelRegisterExitCallback(callback);sceKernelSleepThreadCB();return 0;
}
static void setup_callbacks(void)
{
    int thread=sceKernelCreateThread("GameShelfSetupCallbacks",callback_thread,0x11,0x1000,0,NULL);
    if(thread>=0)sceKernelStartThread(thread,0,NULL);
}
static void progress(unsigned int percent)
{
    unsigned int i;pspDebugScreenSetXY(0,7);
    pspDebugScreenPrintf("[");
    for(i=0;i<40;i++)pspDebugScreenPrintf(i<percent*40/100?"=":" ");
    pspDebugScreenPrintf("] %3u%%",percent);
}
static void wait_button(void)
{
    SceCtrlData pad;
    do {sceCtrlReadBufferPositive(&pad,1);sceKernelDelayThread(20000);}while(pad.Buttons && !exit_requested);
    do {sceCtrlReadBufferPositive(&pad,1);sceKernelDelayThread(20000);}while(!(pad.Buttons&(PSP_CTRL_CROSS|PSP_CTRL_CIRCLE)) && !exit_requested);
}
int main(int argc,char **argv)
{
    SceCtrlData pad;int language=1,result;
    setup_callbacks();pspDebugScreenInit();sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&language);gsh_set_language(language);
    pspDebugScreenPrintf("%s",gsh_text("GameShelf Setup 1.09 - first installation\n\nSeven systems included. Existing files are preserved.\nGames and GBA BIOS are supplied by you.\n\nX: install   Circle: exit\n"));
    do {sceCtrlReadBufferPositive(&pad,1);sceKernelDelayThread(20000);}while(!(pad.Buttons&(PSP_CTRL_CROSS|PSP_CTRL_CIRCLE)) && !exit_requested);
    if(exit_requested||(pad.Buttons&PSP_CTRL_CIRCLE)||argc<1||!argv[0]) {sceKernelExitGame();return 0;}
    result=install_payload(argv[0],"ms0:",progress);
    if(exit_requested) {sceKernelExitGame();return 0;}
    pspDebugScreenSetXY(0,10);
    if(result)pspDebugScreenPrintf("%s\n%s\n",gsh_text("Installation stopped. Existing files were preserved."),install_error);
    else pspDebugScreenPrintf("%s",gsh_text("Installation complete.\nAdd your own games and GBA BIOS, then open GameShelf.\nPress X to create the game entries.\n"));
    pspDebugScreenPrintf("%s",gsh_text("\nX / Circle: return home\n"));wait_button();sceKernelExitGame();return 0;
}
