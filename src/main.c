#include <pspuser.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <pspdisplay.h>
#include <pspinit.h>
#include <psputility_sysparam.h>
#include "language.h"
#include <systemctrl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../shared/gba_identity.h"
#include "../shared/extra_launch.h"

PSP_MODULE_INFO("GameShelf", 0, 1, 10);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define MAX_ENTRIES 32
#define MAX_ROM_ENTRIES 32
#define ENTRY_LENGTH 256
#define STATUS_LENGTH 96

/* Patched in each generated EBOOT; volatile prevents constant folding. */
static volatile struct {
    unsigned char magic[16];
    unsigned int mode;
    char rom[256];
} xmb_target = { { 'G','S','H','X','M','B','1','0','0','T','A','R','G','E','T','!' }, 0, "" };

static int done = 0;
static int refresh = 1;
static int cursor = 0;
static int list_offset = 0;
static int selected = -1;
static int view_mode = 0;
static char iso_games[MAX_ENTRIES][ENTRY_LENGTH];
static char ps1_games[MAX_ENTRIES][ENTRY_LENGTH];
static char homebrew[MAX_ENTRIES][ENTRY_LENGTH];
static char gba_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char gb_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char gbc_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char atari_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char megadrive_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char snes_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char n64_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char nes_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static int iso_count;
static int ps1_count;
static int homebrew_count;
static int gba_count;
static int gb_count;
static int gbc_count;
static int atari_count;
static int megadrive_count;
static int snes_count;
static int n64_count;
static int nes_count;
static int gba_scanned = 0;
static int gb_scanned = 0;
static int gbc_scanned = 0;
static int atari_scanned = 0;
static int megadrive_scanned = 0;
static int snes_scanned = 0;
static int n64_scanned = 0;
static int nes_scanned = 0;
static char status_message[STATUS_LENGTH] = "Ready.";
#define COVER_WIDTH 128
#define COVER_HEIGHT 160
static unsigned short cover_pixels[COVER_WIDTH * COVER_HEIGHT];

static int is_ps1_folder(const char *folder_name);
static int diagnostic_exists(const char *path, int directory);

static const char *platform_badge(void)
{
    if (view_mode == 0) return "[PSP]";
    if (view_mode == 1) return "[PS1]";
    if (view_mode == 2) return "[APP]";
    if (view_mode == 3) return "[GBA]";
    if (view_mode == 4) return "[GB]";
    if (view_mode == 5) return "[GBC]";
    if (view_mode == 6) return "[ATARI]";
    if (view_mode == 7) return "[MD]";
    if (view_mode == 8) return "[SNES]";
    if (view_mode == 10) return "[N64]";
    return "[NES]";
}

static void prepare_boxart_dirs(void)
{
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/psp", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/ps1", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/homebrew", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/gba", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/nes", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/gb", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/atari", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/megadrive", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/snes", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart/n64", 0777);
}

static int selected_cover_exists(char *cover_path, size_t cover_path_size)
{
    const char *entry = NULL;
    const char *folder = NULL;
    char base_name[ENTRY_LENGTH];
    char *extension;
    SceUID file;

    if (view_mode == 0 && cursor < iso_count) {
        entry = iso_games[cursor];
        folder = "psp";
    } else if (view_mode == 1 && cursor < ps1_count) {
        entry = ps1_games[cursor];
        folder = "ps1";
    } else if (view_mode == 2 && cursor < homebrew_count) {
        entry = homebrew[cursor];
        folder = "homebrew";
    } else if (view_mode == 3 && cursor < gba_count) {
        entry = gba_roms[cursor];
        folder = "gba";
    } else if (view_mode == 4 && cursor < gb_count) {
        entry = gb_roms[cursor];
        folder = "gb";
    } else if (view_mode == 5 && cursor < gbc_count) {
        entry = gbc_roms[cursor];
        folder = "gbc";
    } else if (view_mode == 6 && cursor < atari_count) {
        entry = atari_roms[cursor];
        folder = "atari";
    } else if (view_mode == 7 && cursor < megadrive_count) {
        entry = megadrive_roms[cursor];
        folder = "megadrive";
    } else if (view_mode == 8 && cursor < snes_count) {
        entry = snes_roms[cursor];
        folder = "snes";
    } else if (view_mode == 10 && cursor < n64_count) {
        entry = n64_roms[cursor];
        folder = "n64";
    } else if (view_mode == 9 && cursor < nes_count) {
        entry = nes_roms[cursor];
        folder = "nes";
    }

    if (entry == NULL) {
        cover_path[0] = '\0';
        return 0;
    }

    strncpy(base_name, entry, sizeof(base_name) - 1);
    base_name[sizeof(base_name) - 1] = '\0';
    extension = strrchr(base_name, '.');
    if (extension != NULL) *extension = '\0';
    snprintf(cover_path, cover_path_size,
             "ms0:/PSP/GAME/GAMESHELF/boxart/%s/%s.gsh", folder, base_name);
    file = sceIoOpen(cover_path, PSP_O_RDONLY, 0);
    if (file < 0) return 0;
    sceIoClose(file);
    return 1;
}

static int load_selected_cover(void)
{
    char path[ENTRY_LENGTH + 96];
    SceUID file;
    int expected = sizeof(cover_pixels);
    int received;

    if (!selected_cover_exists(path, sizeof(path))) return 0;
    file = sceIoOpen(path, PSP_O_RDONLY, 0);
    if (file < 0) return 0;
    received = sceIoRead(file, cover_pixels, expected);
    sceIoClose(file);
    return received == expected;
}

static void draw_selected_cover(void)
{
    void *framebuffer;
    int buffer_width;
    int pixel_format;
    int sync = PSP_DISPLAY_SETBUF_IMMEDIATE;
    int x;
    int y;
    const int left = 336;
    const int top = 48;

    if (!load_selected_cover()) return;
    if (sceDisplayGetFrameBuf(&framebuffer, &buffer_width, &pixel_format, sync) < 0) return;
    if (framebuffer == NULL) return;
    for (y = 0; y < COVER_HEIGHT; y++) {
        unsigned short *line16 = (unsigned short *)framebuffer + (top + y) * buffer_width + left;
        unsigned int *line32 = (unsigned int *)framebuffer + (top + y) * buffer_width + left;
        for (x = 0; x < COVER_WIDTH; x++) {
            unsigned short pixel = cover_pixels[y * COVER_WIDTH + x];
            unsigned int red = (pixel >> 11) & 0x1F;
            unsigned int green = (pixel >> 5) & 0x3F;
            unsigned int blue = pixel & 0x1F;
            if (pixel_format == PSP_DISPLAY_PIXEL_FORMAT_565) {
                line16[x] = pixel;
            } else if (pixel_format == PSP_DISPLAY_PIXEL_FORMAT_5551) {
                line16[x] = (unsigned short)((red << 11) | ((green >> 1) << 6) | blue | 1);
            } else if (pixel_format == PSP_DISPLAY_PIXEL_FORMAT_4444) {
                line16[x] = (unsigned short)(((red >> 1) << 12) | ((green >> 2) << 8) |
                                             ((blue >> 1) << 4) | 0xF);
            } else if (pixel_format == PSP_DISPLAY_PIXEL_FORMAT_8888) {
                line32[x] = ((red << 3) << 0) | ((green << 2) << 8) |
                            ((blue << 3) << 16) | 0xFF000000;
            }
        }
    }
    sceDisplayWaitVblankStart();
}

static int exit_callback(int arg1, int arg2, void *common)
{
    done = 1;
    return 0;
}

static int callback_thread(SceSize args, void *argp)
{
    int callback_id = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(callback_id);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void)
{
    int thread = sceKernelCreateThread("update_thread", callback_thread,
                                       0x11, 0xFA0, 0, 0);
    if (thread >= 0) {
        sceKernelStartThread(thread, 0, 0);
    }
}

static int has_extension(const char *name, const char *extension)
{
    size_t name_length = strlen(name);
    size_t extension_length = strlen(extension);
    size_t index;

    if (name_length < extension_length) {
        return 0;
    }
    index = name_length - extension_length;
    while (*extension) {
        char actual = name[index++];
        char expected = *extension++;
        if (actual >= 'A' && actual <= 'Z') actual = (char)(actual - 'A' + 'a');
        if (expected >= 'A' && expected <= 'Z') expected = (char)(expected - 'A' + 'a');
        if (actual != expected) return 0;
    }
    return 1;
}

static void copy_entry(char entries[MAX_ENTRIES][ENTRY_LENGTH], int *count,
                       const char *name)
{
    if (*count >= MAX_ENTRIES) return;
    strncpy(entries[*count], name, ENTRY_LENGTH - 1);
    entries[*count][ENTRY_LENGTH - 1] = '\0';
    (*count)++;
}

static void sort_entries(char entries[MAX_ENTRIES][ENTRY_LENGTH], int count)
{
    int left;
    int right;
    char temporary[ENTRY_LENGTH];

    for (left = 0; left < count - 1; left++) {
        for (right = left + 1; right < count; right++) {
            if (strcmp(entries[left], entries[right]) > 0) {
                strcpy(temporary, entries[left]);
                strcpy(entries[left], entries[right]);
                strcpy(entries[right], temporary);
            }
        }
    }
}

static void scan_gba_roms(void)
{
    SceUID directory;
    SceIoDirent entry;

    gba_count = 0;
    gba_scanned = 1;
    memset(gba_roms, 0, sizeof(gba_roms));
    directory = sceIoDopen("ms0:/PSP/GAME/TempGBA4PSP-Mod/roms");
    if (directory < 0) {
        return;
    }
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(directory, &entry) > 0) {
        if (!(entry.d_stat.st_mode & FIO_S_IFDIR) && entry.d_name[0] != '.' &&
            has_extension(entry.d_name, ".gba")) {
            copy_entry(gba_roms, &gba_count, entry.d_name);
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(directory);
    sort_entries(gba_roms, gba_count);
}

static void scan_nes_roms(void)
{
    SceUID directory;
    SceIoDirent entry;

    nes_count = 0;
    nes_scanned = 1;
    memset(nes_roms, 0, sizeof(nes_roms));
    directory = sceIoDopen("ms0:/PSP/GAME/Nester_AoEX_R3/roms");
    if (directory < 0) return;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(directory, &entry) > 0) {
        if (!(entry.d_stat.st_mode & FIO_S_IFDIR) && entry.d_name[0] != '.' &&
            has_extension(entry.d_name, ".nes")) {
            copy_entry(nes_roms, &nes_count, entry.d_name);
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(directory);
    sort_entries(nes_roms, nes_count);
}

static void scan_gb_roms(void)
{
    SceUID directory;
    SceIoDirent entry;

    gb_count = 0;
    gb_scanned = 1;
    memset(gb_roms, 0, sizeof(gb_roms));
    directory = sceIoDopen("ms0:/PSP/GAME/TempGBA4PSP-Mod/roms");
    if (directory < 0) return;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(directory, &entry) > 0) {
        if (!(entry.d_stat.st_mode & FIO_S_IFDIR) && entry.d_name[0] != '.' &&
            has_extension(entry.d_name, ".gb")) {
            copy_entry(gb_roms, &gb_count, entry.d_name);
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(directory);
    sort_entries(gb_roms, gb_count);
}

static void scan_gbc_roms(void)
{
    SceUID directory;
    SceIoDirent entry;

    gbc_count = 0;
    gbc_scanned = 1;
    memset(gbc_roms, 0, sizeof(gbc_roms));
    directory = sceIoDopen("ms0:/PSP/GAME/TempGBA4PSP-Mod/roms");
    if (directory < 0) return;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(directory, &entry) > 0) {
        if (!(entry.d_stat.st_mode & FIO_S_IFDIR) && entry.d_name[0] != '.' &&
            has_extension(entry.d_name, ".gbc")) {
            copy_entry(gbc_roms, &gbc_count, entry.d_name);
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(directory);
    sort_entries(gbc_roms, gbc_count);
}

static void scan_extra_roms(char entries[MAX_ROM_ENTRIES][ENTRY_LENGTH],
                            int *count, int *scanned, const char *directory_path,
                            int extra_mode)
{
    SceUID directory;
    SceIoDirent entry;

    *count = 0;
    *scanned = 1;
    memset(entries, 0, MAX_ROM_ENTRIES * ENTRY_LENGTH);
    directory = sceIoDopen(directory_path);
    if (directory < 0) return;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(directory, &entry) > 0) {
        if (!(entry.d_stat.st_mode & FIO_S_IFDIR) && entry.d_name[0] != '.' &&
            gsh_extra_extension(extra_mode,entry.d_name)) {
            copy_entry(entries, count, entry.d_name);
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(directory);
    sort_entries(entries, *count);
}

static void scan_library(void)
{
    SceUID directory;
    SceIoDirent entry;

    iso_count = 0;
    ps1_count = 0;
    homebrew_count = 0;
    memset(iso_games, 0, sizeof(iso_games));
    memset(ps1_games, 0, sizeof(ps1_games));
    memset(homebrew, 0, sizeof(homebrew));

    directory = sceIoDopen("ms0:/ISO");
    if (directory >= 0) {
        memset(&entry, 0, sizeof(entry));
        while (sceIoDread(directory, &entry) > 0) {
            if (has_extension(entry.d_name, ".iso") || has_extension(entry.d_name, ".cso")) {
                copy_entry(iso_games, &iso_count, entry.d_name);
            }
            memset(&entry, 0, sizeof(entry));
        }
        sceIoDclose(directory);
    }
    sort_entries(iso_games, iso_count);

    directory = sceIoDopen("ms0:/PSP/GAME");
    if (directory >= 0) {
        memset(&entry, 0, sizeof(entry));
        while (sceIoDread(directory, &entry) > 0) {
            if ((entry.d_stat.st_mode & FIO_S_IFDIR) &&
                strcmp(entry.d_name, ".") != 0 &&
                strcmp(entry.d_name, "..") != 0 &&
                strcmp(entry.d_name, "GAMESHELF") != 0) {
                if (is_ps1_folder(entry.d_name))
                    copy_entry(ps1_games, &ps1_count, entry.d_name);
                else
                    copy_entry(homebrew, &homebrew_count, entry.d_name);
            }
            memset(&entry, 0, sizeof(entry));
        }
        sceIoDclose(directory);
    }
    sort_entries(homebrew, homebrew_count);
    sort_entries(ps1_games, ps1_count);
}

static void set_status(const char *message)
{
    strncpy(status_message, message, STATUS_LENGTH - 1);
    status_message[STATUS_LENGTH - 1] = '\0';
}

#include "launch_progress.h"

static int copy_file(const char *source_path, const char *destination_path)
{
    SceUID source;
    SceUID destination;
    static unsigned char buffer[64 * 1024];
    int read_count;
    unsigned int copied=0,reported=0;
    SceIoStat source_stat;
    unsigned int total=sceIoGetstat(source_path,&source_stat)>=0?(unsigned int)source_stat.st_size:0;
    int is_rom=has_extension(source_path,".gba");

    if(strcmp(source_path,destination_path)==0)return 0;
    source = sceIoOpen(source_path, PSP_O_RDONLY, 0);
    if (source < 0) return -1;
    destination = sceIoOpen(destination_path,
                             PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (destination < 0) {
        sceIoClose(source);
        return -1;
    }
    if(is_rom && gsh_launch_active){gsh_rom_copies++;gsh_copy_progress(0,total);}
    while ((read_count = sceIoRead(source, buffer, sizeof(buffer))) > 0) {
        if (sceIoWrite(destination, buffer, read_count) != read_count) {
            sceIoClose(destination);
            sceIoClose(source);
            return -1;
        }
        copied+=read_count;
        if(gsh_launch_active)gsh_copied_bytes+=read_count;
        if(is_rom && (copied-reported>=512*1024 || copied==total)){
            gsh_copy_progress(copied,total);reported=copied;
        }
    }
    sceIoClose(destination);
    sceIoClose(source);
    return read_count < 0 ? -1 : 0;
}

static int copy_support_if_changed(const char *source_path,const char *destination_path)
{
    SceIoStat a,b;
    SceUID source=-1,destination=-1;
    unsigned char left[4096],right[4096];int count,identical=1;
    if(sceIoGetstat(source_path,&a)<0)return -1;
    if(sceIoGetstat(destination_path,&b)<0 || a.st_size!=b.st_size)
        return copy_file(source_path,destination_path);
    source=sceIoOpen(source_path,PSP_O_RDONLY,0);
    destination=sceIoOpen(destination_path,PSP_O_RDONLY,0);
    if(source<0 || destination<0){if(source>=0)sceIoClose(source);if(destination>=0)sceIoClose(destination);return -1;}
    while((count=sceIoRead(source,left,sizeof(left)))>0){
        if(sceIoRead(destination,right,count)!=count || memcmp(left,right,count)!=0){identical=0;break;}
    }
    sceIoClose(source);sceIoClose(destination);
    if(count<0)return -1;
    return identical?0:copy_file(source_path,destination_path);
}

static int write_gba_runtime_config(void)
{
    static const char config[] =
        "# GameShelf TempGBA runtime\n"
        "rom_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/roms\n"
        "save_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/save\n"
        "save_state_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/state\n"
        "snapshot_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/snapshot\n"
        "cheat_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/cheats\n"
        "game_config_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/cfg\n"
        "boxart_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/boxart\n"
        "folder_boxart_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/folder_boxart\n";
    SceUID file;

    file = sceIoOpen("ms0:/PSP/GAME/GAMESHELF/dir.ini",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file < 0) return -1;
    if (sceIoWrite(file, config, sizeof(config) - 1) != sizeof(config) - 1) {
        sceIoClose(file);
        return -1;
    }
    sceIoClose(file);
    return 0;
}

static int write_gba_identity(const char *rom_name)
{
    unsigned char header[256];
    char record[256];
    const char *rom="ms0:/PSP/GAME/GAMESHELF/TempGBA/roms/game.gba";
    SceIoStat st;
    SceUID file;
    int length,count;
    if(sceIoGetstat(rom,&st)<0 || st.st_size<256 || st.st_size>33554432)return -1;
    file=sceIoOpen(rom,PSP_O_RDONLY,0);
    if(file<0)return -1;
    count=sceIoRead(file,header,sizeof(header));sceIoClose(file);
    if(count!=sizeof(header))return -1;
    length=gsh_identity_encode(record,sizeof(record),rom_name,(unsigned int)st.st_size,header);
    if(length<0)return -1;
    file=sceIoOpen(GSH_IDENTITY_PATH,PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC,0777);
    if(file<0)return -1;
    count=sceIoWrite(file,record,length);sceIoClose(file);
    return count==length?0:-1;
}

static int prepare_gba_runtime(const char *rom_name)
{
    static const char config[] =
        "# GameShelf local TempGBA runtime\n"
        "rom_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/roms\n"
        "save_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/save\n"
        "save_state_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/state\n"
        "snapshot_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/snapshot\n"
        "cheat_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/cheats\n"
        "game_config_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/cfg\n"
        "boxart_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/boxart\n"
        "folder_boxart_directory = ms0:/PSP/GAME/GAMESHELF/TempGBA/folder_boxart\n";
    SceUID file;
    char source_rom[ENTRY_LENGTH];

    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/roms", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/save", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/state", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/snapshot", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/cheats", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/cfg", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/boxart", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/folder_boxart", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/roms", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/save", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/state", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/snapshot", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/cheats", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/cfg", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/boxart", 0777);
    sceIoMkdir("ms0:/PSP/GAME/GAMESHELF/TempGBA/folder_boxart", 0777);
    if(!gsh_valid_rom_name(rom_name))return -1;
    /* Invalidate any previous identity before replacing the staged ROM. */
    { SceIoStat old;
      if(sceIoGetstat(GSH_IDENTITY_PATH,&old)>=0 && sceIoRemove(GSH_IDENTITY_PATH)<0)return -1;
    }
    if (write_gba_runtime_config() < 0) return -1;
    file = sceIoOpen("ms0:/PSP/GAME/GAMESHELF/TempGBA/dir.ini",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file < 0) return -1;
    if (sceIoWrite(file, config, sizeof(config) - 1) != sizeof(config) - 1) {
        sceIoClose(file);
        return -1;
    }
    sceIoClose(file);
    snprintf(source_rom, sizeof(source_rom),
             "ms0:/PSP/GAME/TempGBA4PSP-Mod/roms/%s", rom_name);
    if (copy_file(source_rom,
                  "ms0:/PSP/GAME/GAMESHELF/TempGBA/roms/game.gba") < 0) return -1;
    /* Preserve both legacy/current launch contexts, but never rewrite an
     * identical support module. Only the unused second ROM copy is removed. */
    gsh_launch_stage("Checking emulator files...");
    {
        static const char *names[]={"gba_bios.bin","tempgba_me.prx","cp_fix_gbk.prx","exception.prx"};
        unsigned int i;char source[ENTRY_LENGTH],destination[ENTRY_LENGTH];
        for(i=0;i<sizeof(names)/sizeof(names[0]);i++){
            snprintf(source,sizeof(source),"ms0:/PSP/GAME/TempGBA4PSP-Mod/%s",names[i]);
            snprintf(destination,sizeof(destination),"ms0:/PSP/GAME/GAMESHELF/%s",names[i]);
            if(copy_support_if_changed(source,destination)<0)return -1;
            snprintf(destination,sizeof(destination),"ms0:/PSP/GAME/GAMESHELF/TempGBA/%s",names[i]);
            if(copy_support_if_changed(source,destination)<0)return -1;
        }
    }
    gsh_launch_stage("Starting emulator...");
    return write_gba_identity(rom_name);
}

static int is_ps1_folder(const char *folder_name)
{
    SceUID directory;
    SceIoDirent entry;
    char path[ENTRY_LENGTH];

    if (strstr(folder_name, "Marvel vs Capcom") != NULL ||
        strstr(folder_name, "Crash Bandicoot") != NULL ||
        strstr(folder_name, "Spyro") != NULL ||
        strncmp(folder_name, "SCUS", 4) == 0 ||
        strncmp(folder_name, "SLUS", 4) == 0 ||
        strncmp(folder_name, "SLES", 4) == 0 ||
        strncmp(folder_name, "SCES", 4) == 0 ||
        strncmp(folder_name, "SLPS", 4) == 0 ||
        strncmp(folder_name, "SLPM", 4) == 0 ||
        strncmp(folder_name, "ULUS", 4) == 0 ||
        strncmp(folder_name, "ULES", 4) == 0) return 1;
    snprintf(path, sizeof(path), "ms0:/PSP/GAME/%s", folder_name);
    directory = sceIoDopen(path);
    if (directory < 0) return 0;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(directory, &entry) > 0) {
        if ((entry.d_stat.st_mode & FIO_S_IFDIR) &&
            (strncmp(entry.d_name, "SCUS", 4) == 0 ||
             strncmp(entry.d_name, "SLUS", 4) == 0 ||
             strncmp(entry.d_name, "SLES", 4) == 0 ||
             strncmp(entry.d_name, "SCES", 4) == 0)) {
            sceIoDclose(directory);
            return 1;
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(directory);
    return 0;
}

static void write_ps1_status(const char *folder, const char *path)
{
    SceUID file;
    char status[512];
    int length = snprintf(status, sizeof(status),
                          "GameShelf PS1 launch v0.66\n"
                          "Folder: %s\nEBOOT: %s\n"
                          "Arguments: EBOOT path (POPS launch protocol)\n"
                          "API type: PSP_INIT_APITYPE_MS4 (0x143)\n"
                          "Key: pops\n"
                          "Key config: PSP_INIT_KEYCONFIG_POPS (0x300)\n",
                          folder, path);
    file = sceIoOpen("ms0:/PSP/GAMESHELF/PS1_STATUS.TXT",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file >= 0) {
        sceIoWrite(file, status, length);
        sceIoClose(file);
    }
}

static int write_launch_request(const char *path)
{
    SceUID file;
    int written;
    size_t length = strlen(path);
    const char newline = '\n';

    sceIoMkdir("ms0:/PSP/GAMESHELF", 0777);
    file = sceIoOpen("ms0:/PSP/GAMESHELF/LAUNCH.TXT",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file < 0) return -1;
    written = sceIoWrite(file, path, length);
    sceIoWrite(file, &newline, 1);
    sceIoClose(file);
    return written == (int)length ? 0 : -1;
}

static void write_iso_status(const char *path, const char *result)
{
    SceUID file;
    char status[512];
    int length;

    length = snprintf(status, sizeof(status),
                      "GameShelf ISO hand-off v0.59\n"
                      "Path: %s\n"
                      "Result: %s\n"
                      "The CFW/VSH loader must consume LAUNCH.TXT.\n",
                      path, result);
    file = sceIoOpen("ms0:/PSP/GAMESHELF/ISO_STATUS.TXT",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file >= 0) {
        sceIoWrite(file, status, length);
        sceIoClose(file);
    }
}

static int diagnostic_exists(const char *path, int directory)
{
    SceUID handle;

    if (directory) {
        handle = sceIoDopen(path);
    } else {
        handle = sceIoOpen(path, PSP_O_RDONLY, 0);
    }
    if (handle < 0) return 0;
    if (directory) sceIoDclose(handle);
    else sceIoClose(handle);
    return 1;
}

static void add_diagnostic_line(char *log, int *used, const char *label,
                                const char *path, int directory)
{
    int present = diagnostic_exists(path, directory);
    int written = snprintf(log + *used, 2048 - *used, "%s: %s\n  %s\n",
                           label, present ? "OK" : "MISSING", path);
    if (written > 0) *used += written;
    if (*used >= 2048) *used = 2047;
}

static void write_gba_diagnostics(void)
{
    char log[2048];
    char cwd[256];
    int used = 0;
    SceUID file;

    memset(log, 0, sizeof(log));
    used += snprintf(log + used, sizeof(log) - used,
                     "PSP GameShelf GBA diagnostics v0.45\n\n");
    memset(cwd, 0, sizeof(cwd));
    if (getcwd(cwd, sizeof(cwd) - 1) != NULL) {
        used += snprintf(log + used, sizeof(log) - used,
                         "GameShelf cwd: %s\n\n", cwd);
    } else {
        used += snprintf(log + used, sizeof(log) - used,
                         "GameShelf cwd: UNAVAILABLE\n\n");
    }
    add_diagnostic_line(log, &used, "TempGBA ROM directory",
                        "ms0:/PSP/GAME/TempGBA4PSP-Mod/roms", 1);
    add_diagnostic_line(log, &used, "TempGBA dir.ini",
                        "ms0:/PSP/GAME/TempGBA4PSP-Mod/dir.ini", 0);
    add_diagnostic_line(log, &used, "TempGBA BIOS",
                        "ms0:/PSP/GAME/TempGBA4PSP-Mod/gba_bios.bin", 0);
    add_diagnostic_line(log, &used, "GameShelf dir.ini",
                        "ms0:/PSP/GAME/GAMESHELF/dir.ini", 0);
    add_diagnostic_line(log, &used, "GameShelf BIOS",
                        "ms0:/PSP/GAME/GAMESHELF/gba_bios.bin", 0);
    add_diagnostic_line(log, &used, "GameShelf runtime",
                        "ms0:/PSP/GAME/GAMESHELF/TempGBA", 1);
    if (gba_count > 0 && cursor < gba_count) {
        char rom_path[ENTRY_LENGTH];
        snprintf(rom_path, sizeof(rom_path),
                 "ms0:/PSP/GAME/TempGBA4PSP-Mod/roms/%s", gba_roms[cursor]);
        add_diagnostic_line(log, &used, "Selected ROM", rom_path, 0);
    }
    file = sceIoOpen("ms0:/PSP/GAME/GAMESHELF/GBA_DIAG.TXT",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file >= 0) {
        sceIoWrite(file, log, used < 2048 ? used : 2047);
        sceIoClose(file);
        set_status(gsh_text("Diagnostics saved to GBA_DIAG.TXT"));
    } else {
        set_status(gsh_text("Cannot save GBA_DIAG.TXT"));
    }
}

static void launch_selected(void)
{
    SceKernelLoadExecVSHParam param;
    char path[ENTRY_LENGTH];
    char argument[512];
    int has_argument = 0;
    unsigned int argument_size = 0;
    int launch_apitype = PSP_INIT_APITYPE_MS2;
    int launch_ps1 = 0;
    int result;

    if (view_mode == 3) {
        if (cursor >= gba_count) {
            set_status(gsh_text("No GBA ROM selected."));
            return;
        }
        if (prepare_gba_runtime(gba_roms[cursor]) < 0) {
            set_status(gsh_text("GBA setup failed: check GBA_DIAG.TXT"));
            write_gba_diagnostics();
            return;
        }
        snprintf(path, sizeof(path),
                 "ms0:/PSP/GAME/GAMESHELF/TempGBA/EBOOT.PBP");
        argument[0] = '\0';
        sceIoChdir("ms0:/PSP/GAME/GAMESHELF");
        goto launch_application;
    }

    if (view_mode == 4 || view_mode == 5) {
        const char *rom_name;
        if ((view_mode == 4 && cursor >= gb_count) ||
            (view_mode == 5 && cursor >= gbc_count)) {
            set_status(gsh_text("No Game Boy ROM selected.")); return;
        }
        rom_name = view_mode == 4 ? gb_roms[cursor] : gbc_roms[cursor];
        snprintf(path, sizeof(path), "ms0:/PSP/GAME/GAMESHELF/GBAdhoc/EBOOT.PBP");
        /* PSP argv requires NUL-separated executable, option and ROM path. */
        {
            unsigned int offset = strlen(path)+1;
            memcpy(argument,path,offset);
            memcpy(argument+offset,"--gameshelf",12); offset+=12;
            snprintf(argument+offset,sizeof(argument)-offset,
                     "ms0:/PSP/GAME/TempGBA4PSP-Mod/roms/%s",rom_name);
            argument_size=offset+strlen(argument+offset)+1;
        }
        has_argument=1;
        sceIoChdir("ms0:/PSP/GAME/GAMESHELF/GBAdhoc");
        goto launch_application;
    }

    if (view_mode >= 6 && view_mode <= 8) {
        if(cursor<0||cursor>=(view_mode==6?atari_count:view_mode==7?megadrive_count:snes_count)){set_status(gsh_text("No title selected."));return;}
        const char *rom_name=view_mode==6?atari_roms[cursor]:
                             view_mode==7?megadrive_roms[cursor]:snes_roms[cursor];
        const char *runtime=view_mode==6?"ms0:/PSP/GAME/psp2600/EBOOT.PBP":
                            view_mode==7?"ms0:/PSP/GAME/PicoDrive/EBOOT.PBP":
                                         "ms0:/PSP/GAME/s9xTYLme_mod/EBOOT.PBP";
        unsigned int offset;
        snprintf(path,sizeof(path),"%s",runtime);
        offset=strlen(path)+1;memcpy(argument,path,offset);
        if(gsh_extra_path(argument+offset,sizeof(argument)-offset,view_mode,rom_name)<0) {
            set_status(gsh_text("Invalid game entry."));return;
        }
        argument_size=offset+strlen(argument+offset)+1;has_argument=1;
        sceIoChdir(view_mode==6?"ms0:/PSP/GAME/psp2600":
                   view_mode==7?"ms0:/PSP/GAME/PicoDrive":"ms0:/PSP/GAME/s9xTYLme_mod");
        goto launch_application;
    }

    if (view_mode == 10) {
        unsigned int offset;
        if(cursor<0 || cursor>=n64_count) {set_status(gsh_text("No title selected."));return;}
        snprintf(path,sizeof(path),"ms0:/PSP/GAME/DaedalusX64/EBOOT.PBP");
        offset=strlen(path)+1;memcpy(argument,path,offset);
        if(gsh_extra_path(argument+offset,sizeof(argument)-offset,10,n64_roms[cursor])<0 ||
           !diagnostic_exists(argument+offset,0) || !diagnostic_exists(path,0)) {
            set_status(gsh_text("Invalid game entry."));return;
        }
        argument_size=offset+strlen(argument+offset)+1;has_argument=1;
        sceIoChdir("ms0:/PSP/GAME/DaedalusX64");
        goto launch_application;
    }

    if (view_mode == 9) {
        snprintf(path, sizeof(path),
                 "ms0:/PSP/GAME/Nester_AoEX_R3/EBOOT.PBP");
        snprintf(argument, sizeof(argument),
                 "ms0:/PSP/GAME/Nester_AoEX_R3/roms/%s", nes_roms[cursor]);
        has_argument = 1;
        sceIoChdir("ms0:/PSP/GAME/Nester_AoEX_R3");
        goto launch_application;
    }

    if (view_mode == 0) {
        if (cursor >= iso_count) {
            set_status(gsh_text("No ISO selected."));
            return;
        }
        snprintf(path, sizeof(path), "ms0:/ISO/%s", iso_games[cursor]);
        if (!diagnostic_exists(path, 0)) {
            set_status(gsh_text("ISO not found on the memory card."));
            write_iso_status(path, "file missing");
            return;
        }
        if (write_launch_request(path) == 0) {
            set_status(gsh_text("ISO launch request saved for the CFW loader."));
            write_iso_status(path, "request written");
        } else {
            set_status(gsh_text("Cannot write LAUNCH.TXT."));
            write_iso_status(path, "request write failed");
        }
        return;
    }

    if (view_mode == 1) {
        if (cursor >= ps1_count) { set_status(gsh_text("No PS1 game selected.")); return; }
        snprintf(path, sizeof(path), "ms0:/PSP/GAME/%s/EBOOT.PBP", ps1_games[cursor]);
        if (!diagnostic_exists(path, 0)) {
            set_status(gsh_text("PS1 EBOOT not found."));
            write_ps1_status(ps1_games[cursor], path);
            return;
        }
        write_ps1_status(ps1_games[cursor], path);
        /* The POPS loadexec protocol uses the PS1 apitype, the "pops" key,
         * and the EBOOT path as argp. This is the sequence used by PSP shell
         * launchers; the generic "game" key leaves POPS at a black screen. */
        strcpy(argument, path);
        has_argument = 1;
        launch_apitype = PSP_INIT_APITYPE_MS4;
        launch_ps1 = 1;
        goto launch_application;
    }
    if (cursor >= homebrew_count) {
        set_status(gsh_text("No title selected."));
        return;
    }
    snprintf(path, sizeof(path), "ms0:/PSP/GAME/%s/EBOOT.PBP",
             homebrew[cursor]);
    strcpy(argument, path);
    has_argument = 1;
launch_application:
    if(gsh_launch_active){
        const char *name=view_mode==3?gba_roms[cursor]:view_mode==4?gb_roms[cursor]:view_mode==5?gbc_roms[cursor]:view_mode==6?atari_roms[cursor]:view_mode==7?megadrive_roms[cursor]:view_mode==8?snes_roms[cursor]:view_mode==10?n64_roms[cursor]:nes_roms[cursor];
        gsh_launch_stage("Starting emulator...");gsh_launch_log(name);
    }
    memset(&param, 0, sizeof(param));
    param.size = sizeof(param);
    param.args = has_argument ? (argument_size ? argument_size : strlen(argument) + 1) : 0;
    param.argp = has_argument ? argument : NULL;
    param.key = launch_ps1 ? "pops" : "game";
    result = sctrlKernelLoadExecVSHWithApitype(launch_apitype, path, &param);
    if (result < 0) {
        snprintf(status_message, STATUS_LENGTH, gsh_text("Loader error: 0x%08X"),
                 (unsigned int)result);
    }
}

// Generator shares the existing emulator launch adapters.
#include "xmb.h"

static void draw_screen(void)
{
    int index;
    int last_index;
    int total = view_mode == 0 ? iso_count :
                (view_mode == 1 ? ps1_count :
                 (view_mode == 2 ? homebrew_count :
                  (view_mode == 3 ? gba_count :
                   (view_mode == 4 ? gb_count :
                    (view_mode == 5 ? gbc_count :
                     (view_mode == 6 ? atari_count :
                      (view_mode == 7 ? megadrive_count :
                       (view_mode == 8 ? snes_count : view_mode == 10 ? n64_count : nes_count))))))));

    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("%s", gsh_text("PSP GameShelf 1.10 - hardware test\n"));
    pspDebugScreenPrintf("PSP-3004 / TA-095v2 / FW 6.61\n\n");
    if (view_mode == 0) {
        pspDebugScreenPrintf(gsh_text("%s  View: PSP ISO / CSO (%d)\n"), platform_badge(), iso_count);
        for (index = list_offset; index < iso_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, iso_games[index]);
        }
    } else if (view_mode == 1) {
        pspDebugScreenPrintf(gsh_text("%s  View: PlayStation 1 (%d)\n"), platform_badge(), ps1_count);
        for (index = list_offset; index < ps1_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, ps1_games[index]);
        }
    } else if (view_mode == 2) {
        pspDebugScreenPrintf(gsh_text("%s  View: Applications / Homebrew (%d)\n"), platform_badge(), homebrew_count);
        for (index = list_offset; index < homebrew_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, homebrew[index]);
        }
    } else if (view_mode == 3) {
        pspDebugScreenPrintf(gsh_text("%s  View: Game Boy Advance (%d)\n"), platform_badge(), gba_count);
        for (index = list_offset; index < gba_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, gba_roms[index]);
        }
    } else if (view_mode == 4) {
        pspDebugScreenPrintf(gsh_text("%s  View: Game Boy (%d)\n"), platform_badge(), gb_count);
        for (index = list_offset; index < gb_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, gb_roms[index]);
        }
    } else if (view_mode == 5) {
        pspDebugScreenPrintf(gsh_text("%s  View: Game Boy Color (%d)\n"), platform_badge(), gbc_count);
        for (index = list_offset; index < gbc_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, gbc_roms[index]);
        }
    } else if (view_mode == 6) {
        pspDebugScreenPrintf(gsh_text("%s  View: Atari 2600 (%d)\n"), platform_badge(), atari_count);
        for (index = list_offset; index < atari_count && index < list_offset + 18; index++)
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, atari_roms[index]);
    } else if (view_mode == 7) {
        pspDebugScreenPrintf(gsh_text("%s  View: Mega Drive (%d)\n"), platform_badge(), megadrive_count);
        for (index = list_offset; index < megadrive_count && index < list_offset + 18; index++)
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, megadrive_roms[index]);
    } else if (view_mode == 8) {
        pspDebugScreenPrintf(gsh_text("%s  View: Super Nintendo (%d)\n"), platform_badge(), snes_count);
        for (index = list_offset; index < snes_count && index < list_offset + 18; index++)
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, snes_roms[index]);
    } else if (view_mode == 10) {
        pspDebugScreenPrintf(gsh_text("%s  View: Nintendo 64 (%d)\n"), platform_badge(), n64_count);
        for(index=list_offset;index<n64_count && index<list_offset+18;index++)
            pspDebugScreenPrintf("%c %02d. %s\n",cursor==index?'>':' ',index+1,n64_roms[index]);
    } else {
        pspDebugScreenPrintf(gsh_text("%s  View: Nintendo Entertainment System (%d)\n"), platform_badge(), nes_count);
        for (index = list_offset; index < nes_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, nes_roms[index]);
        }
    }
    last_index = total < list_offset + 18 ? total : list_offset + 18;
    if (total > 18) {
        pspDebugScreenPrintf(gsh_text("Entries %d-%d of %d\n"), list_offset + 1,
                             last_index, total);
    }
    pspDebugScreenPrintf("%s", gsh_text("\nLeft/Right: change view\n"));
    pspDebugScreenPrintf("%s", gsh_text("Up/Down: choose   X: select/diagnose\n"));
    pspDebugScreenPrintf("%s", gsh_text("Triangle: refresh   HOME: exit\n"));
    pspDebugScreenPrintf(gsh_text("Covers: PSP/GAME/GAMESHELF/boxart/%s/\n"),
                         view_mode == 0 ? "psp" :
                         (view_mode == 1 ? "ps1" :
                          (view_mode == 2 ? "homebrew" :
                           (view_mode == 3 ? "gba" :
                            (view_mode == 4 ? "gb" :
                             (view_mode == 5 ? "gbc" :
                              (view_mode == 6 ? "atari" :
                               (view_mode == 7 ? "megadrive" :
                                (view_mode == 8 ? "snes" : view_mode == 10 ? "n64" : "nes")))))))));
    {
        char cover_path[ENTRY_LENGTH + 96];
        if (selected_cover_exists(cover_path, sizeof(cover_path)))
            pspDebugScreenPrintf("%s", gsh_text("Selected cover: OK\n"));
        else
            pspDebugScreenPrintf("%s", gsh_text("Selected cover: missing\n"));
    }
    pspDebugScreenPrintf(gsh_text("Status: %s\n"), status_message);
    if (selected >= 0 && selected < total) pspDebugScreenPrintf("%s", gsh_text("Selected.\n"));
    draw_selected_cover();
}

int main(void)
{
    SceCtrlData pad;

    pspDebugScreenInit();
    pspDebugScreenClear();
    sceDisplayWaitVblankStart();

    {
        int language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
        if (sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &language) < 0)
            language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
        gsh_set_language(language);
        set_status(gsh_text("Ready."));
    }
    if (xmb_target.mode != 0) {
        char name[256];
        unsigned int mode = xmb_target.mode;
        memcpy(name, (const void *)xmb_target.rom, sizeof(name));
        name[255] = '\0';
        setup_callbacks();
        if ((mode >= 3 && mode <= 10) && name[0] &&
            !strchr(name, '/') && !strchr(name, ':') && !strchr(name, '\\')) {
            view_mode = mode;
            cursor = 0;
            if (mode == 3) { strcpy(gba_roms[0], name); gba_count = 1; }
            else if(mode==4) { strcpy(gb_roms[0],name); gb_count=1; }
            else if(mode==5) { strcpy(gbc_roms[0],name); gbc_count=1; }
            else if(mode==6) { strcpy(atari_roms[0],name); atari_count=1; }
            else if(mode==7) { strcpy(megadrive_roms[0],name); megadrive_count=1; }
            else if(mode==8) { strcpy(snes_roms[0],name); snes_count=1; }
            else if(mode==10) { strcpy(n64_roms[0],name); n64_count=1; }
            else { strcpy(nes_roms[0], name); nes_count = 1; }
            gsh_launch_begin(name);
            launch_selected();
        } else set_status(gsh_text("Invalid game entry."));
        pspDebugScreenPrintf(gsh_text("%s\nPress HOME to exit.\n"), status_message);
        while (!done) sceKernelDelayThread(16000);
        sceKernelExitGame();
        return 0;
    }
    pspDebugScreenPrintf("%s", gsh_text("GameShelf 1.10\nX: create/update game entries (8 systems)\nCircle: legacy catalog   HOME: exit\n"));
    setup_callbacks();
    for (;;) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (done) { sceKernelExitGame(); return 0; }
        if (pad.Buttons & PSP_CTRL_CROSS) {
            generate_xmb();
            pspDebugScreenPrintf("%s", gsh_text("\nPress X to return to the home screen.\n"));
            do { sceCtrlReadBufferPositive(&pad, 1); sceKernelDelayThread(16000); }
            while (pad.Buttons & PSP_CTRL_CROSS);
            do { sceCtrlReadBufferPositive(&pad, 1); sceKernelDelayThread(16000); }
            while (!(pad.Buttons & PSP_CTRL_CROSS) && !done);
            sceKernelExitGame(); return 0;
        }
        if (pad.Buttons & PSP_CTRL_CIRCLE) break;
        sceKernelDelayThread(16000);
    }
    prepare_boxart_dirs();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    while (!done) {
        static unsigned int previous_buttons = 0;
        unsigned int pressed;
        int total;

        sceCtrlReadBufferPositive(&pad, 1);
        pressed = pad.Buttons & ~previous_buttons;
        previous_buttons = pad.Buttons;
        total = view_mode == 0 ? iso_count :
                (view_mode == 1 ? ps1_count :
                 (view_mode == 2 ? homebrew_count :
                  (view_mode == 3 ? gba_count :
                   (view_mode == 4 ? gb_count :
                    (view_mode == 5 ? gbc_count :
                     (view_mode == 6 ? atari_count :
                      (view_mode == 7 ? megadrive_count :
                       (view_mode == 8 ? snes_count : view_mode == 10 ? n64_count : nes_count))))))));

        if (pressed & PSP_CTRL_LEFT) {
            view_mode--;
            if (view_mode < 0) view_mode = 10;
            cursor = 0;
            list_offset = 0;
            selected = -1;
            if (view_mode == 3 && !gba_scanned) scan_gba_roms();
            if (view_mode == 4 && !gb_scanned) scan_gb_roms();
            if (view_mode == 5 && !gbc_scanned) scan_gbc_roms();
            if (view_mode == 6 && !atari_scanned) scan_extra_roms(atari_roms, &atari_count, &atari_scanned, "ms0:/PSP/GAME/psp2600/roms", 6);
            if (view_mode == 7 && !megadrive_scanned) scan_extra_roms(megadrive_roms, &megadrive_count, &megadrive_scanned, "ms0:/PSP/GAME/PicoDrive/roms", 7);
            if (view_mode == 8 && !snes_scanned) scan_extra_roms(snes_roms, &snes_count, &snes_scanned, "ms0:/PSP/GAME/s9xTYLme_mod/roms", 8);
            if (view_mode == 9 && !nes_scanned) scan_nes_roms();
            if (view_mode == 10 && !n64_scanned) scan_extra_roms(n64_roms,&n64_count,&n64_scanned,"ms0:/PSP/GAME/DaedalusX64/Roms",10);
            draw_screen();
        }
        if (pressed & PSP_CTRL_RIGHT) {
            view_mode++;
            if (view_mode > 10) view_mode = 0;
            cursor = 0;
            list_offset = 0;
            selected = -1;
            if (view_mode == 3 && !gba_scanned) scan_gba_roms();
            if (view_mode == 4 && !gb_scanned) scan_gb_roms();
            if (view_mode == 5 && !gbc_scanned) scan_gbc_roms();
            if (view_mode == 6 && !atari_scanned) scan_extra_roms(atari_roms, &atari_count, &atari_scanned, "ms0:/PSP/GAME/psp2600/roms", 6);
            if (view_mode == 7 && !megadrive_scanned) scan_extra_roms(megadrive_roms, &megadrive_count, &megadrive_scanned, "ms0:/PSP/GAME/PicoDrive/roms", 7);
            if (view_mode == 8 && !snes_scanned) scan_extra_roms(snes_roms, &snes_count, &snes_scanned, "ms0:/PSP/GAME/s9xTYLme_mod/roms", 8);
            if (view_mode == 9 && !nes_scanned) scan_nes_roms();
            if (view_mode == 10 && !n64_scanned) scan_extra_roms(n64_roms,&n64_count,&n64_scanned,"ms0:/PSP/GAME/DaedalusX64/Roms",10);
            draw_screen();
        }

        if ((pressed & PSP_CTRL_UP) && cursor > 0) {
            cursor--;
            if (cursor < list_offset) list_offset = cursor;
        }
        if ((pressed & PSP_CTRL_DOWN) && cursor + 1 < total) {
            cursor++;
            if (cursor >= list_offset + 18) list_offset = cursor - 17;
        }
        if ((pressed & PSP_CTRL_CROSS) && total > 0) {
            selected = cursor;
            launch_selected();
        }
        if (pressed & PSP_CTRL_TRIANGLE) {
            set_status(gsh_text("The library refreshes at the next startup."));
        }
        if (refresh) {
            scan_library();
            total = view_mode == 0 ? iso_count :
                    (view_mode == 1 ? ps1_count :
                     (view_mode == 2 ? homebrew_count :
                      (view_mode == 3 ? gba_count :
                       (view_mode == 4 ? gb_count :
                        (view_mode == 5 ? gbc_count :
                         (view_mode == 6 ? atari_count :
                          (view_mode == 7 ? megadrive_count :
                           (view_mode == 8 ? snes_count : view_mode == 10 ? n64_count : nes_count))))))));
            if (cursor >= total) cursor = total > 0 ? total - 1 : 0;
            if (list_offset > cursor) list_offset = cursor;
            draw_screen();
            refresh = 0;
        } else if (pressed & (PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_CROSS)) {
            draw_screen();
        }
        sceKernelDelayThread(16000);
    }

    sceKernelExitGame();
    return 0;
}
