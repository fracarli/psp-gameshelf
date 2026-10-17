#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <pspinit.h>
#include <psploadexec_kernel.h>
#include <systemctrl.h>
#include <systemctrl_se.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("GAMESHELFVSH", PSP_MODULE_KERNEL, 1, 0);
PSP_MAIN_THREAD_ATTR(0);

#define REQUEST_PATH "ms0:/PSP/GAMESHELF/LAUNCH.TXT"
#define STATUS_PATH "ms0:/PSP/GAMESHELF/STATUS.TXT"
#define PATH_LENGTH 256

static int running = 1;

static void write_status(const char *message)
{
    SceUID file = sceIoOpen(STATUS_PATH,
                             PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file >= 0) {
        sceIoWrite(file, message, strlen(message));
        sceIoWrite(file, "\n", 1);
        sceIoClose(file);
    }
}

static int read_request(char *path, int size)
{
    SceUID file;
    int length;

    file = sceIoOpen(REQUEST_PATH, PSP_O_RDONLY, 0777);
    if (file < 0) {
        return -1;
    }

    length = sceIoRead(file, path, size - 1);
    sceIoClose(file);
    if (length <= 0) {
        return -1;
    }

    path[length] = '\0';
    while (length > 0 && (path[length - 1] == '\n' ||
                          path[length - 1] == '\r')) {
        path[--length] = '\0';
    }
    return length > 0 ? 0 : -1;
}

static int valid_iso_path(const char *path)
{
    size_t length = strlen(path);

    if (strncmp(path, "ms0:/ISO/", 9) != 0 || length < 13) {
        return 0;
    }
    return (strstr(path + 9, ".iso") != NULL ||
            strstr(path + 9, ".ISO") != NULL ||
            strstr(path + 9, ".cso") != NULL ||
            strstr(path + 9, ".CSO") != NULL);
}

static int launcher_thread(SceSize args, void *argp)
{
    char path[PATH_LENGTH];
    char last_path[PATH_LENGTH] = "";

    (void)args;
    (void)argp;
    write_status("GameShelf VSH plugin attivo");

    while (running) {
        if (read_request(path, sizeof(path)) == 0 &&
            valid_iso_path(path) && strcmp(path, last_path) != 0) {
            SceKernelLoadExecVSHParam param;
            int result;
            int mount_result;
            const char *boot_path = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN";

            strcpy(last_path, path);
            mount_result = sctrlSEMountUmdFromFile(path, 1, 1);
            if (mount_result < 0) {
                char status[64];
                snprintf(status, sizeof(status), "Mount error: 0x%08X",
                         (unsigned int)mount_result);
                write_status(status);
                sceKernelDelayThread(250000);
                continue;
            }

            memset(&param, 0, sizeof(param));
            param.size = sizeof(param);
            param.args = strlen(boot_path) + 1;
            param.argp = (void *)boot_path;
            param.key = "umdemu";

            result = sctrlKernelLoadExecVSHWithApitype(
                PSP_INIT_APITYPE_DISC, boot_path, &param);
            if (result < 0) {
                char status[64];
                snprintf(status, sizeof(status), "Loader error: 0x%08X",
                         (unsigned int)result);
                write_status(status);
            } else {
                write_status("ISO montato; avvio EBOOT.BIN richiesto");
            }
        }
        sceKernelDelayThread(250000);
    }
    return 0;
}

int module_start(SceSize args, void *argp)
{
    SceUID thread;

    thread = sceKernelCreateThread("gameshelf_vsh", launcher_thread,
                                   0x11, 0x1000, 0, NULL);
    if (thread >= 0) {
        sceKernelStartThread(thread, 0, NULL);
    }
    (void)args;
    (void)argp;
    return 0;
}

int module_stop(SceSize args, void *argp)
{
    running = 0;
    (void)args;
    (void)argp;
    return 0;
}
