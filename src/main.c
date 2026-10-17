#include <pspuser.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <systemctrl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("CONTROLTEST", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define MAX_ENTRIES 32
#define MAX_ROM_ENTRIES 32
#define ENTRY_LENGTH 256
#define STATUS_LENGTH 96

static int done = 0;
static int refresh = 1;
static int cursor = 0;
static int list_offset = 0;
static int selected = -1;
static int view_mode = 0;
static char iso_games[MAX_ENTRIES][ENTRY_LENGTH];
static char homebrew[MAX_ENTRIES][ENTRY_LENGTH];
static char gba_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static char nes_roms[MAX_ROM_ENTRIES][ENTRY_LENGTH];
static int iso_count;
static int homebrew_count;
static int gba_count;
static int nes_count;
static int gba_scanned = 0;
static int nes_scanned = 0;
static char status_message[STATUS_LENGTH] = "Pronto.";

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

static void scan_library(void)
{
    SceUID directory;
    SceIoDirent entry;

    iso_count = 0;
    homebrew_count = 0;
    memset(iso_games, 0, sizeof(iso_games));
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
                copy_entry(homebrew, &homebrew_count, entry.d_name);
            }
            memset(&entry, 0, sizeof(entry));
        }
        sceIoDclose(directory);
    }
    sort_entries(homebrew, homebrew_count);
}

static void set_status(const char *message)
{
    strncpy(status_message, message, STATUS_LENGTH - 1);
    status_message[STATUS_LENGTH - 1] = '\0';
}

static int copy_file(const char *source_path, const char *destination_path)
{
    SceUID source;
    SceUID destination;
    unsigned char buffer[16 * 1024];
    int read_count;

    source = sceIoOpen(source_path, PSP_O_RDONLY, 0);
    if (source < 0) return -1;
    destination = sceIoOpen(destination_path,
                             PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (destination < 0) {
        sceIoClose(source);
        return -1;
    }
    while ((read_count = sceIoRead(source, buffer, sizeof(buffer))) > 0) {
        if (sceIoWrite(destination, buffer, read_count) != read_count) {
            sceIoClose(destination);
            sceIoClose(source);
            return -1;
        }
    }
    sceIoClose(destination);
    sceIoClose(source);
    return read_count < 0 ? -1 : 0;
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
                  "ms0:/PSP/GAME/GAMESHELF/roms/game.gba") < 0) return -1;
    if (copy_file(source_rom,
                  "ms0:/PSP/GAME/GAMESHELF/TempGBA/roms/game.gba") < 0) return -1;
    /*
     * Keep the source-built GameShelf TempGBA EBOOT already installed in the
     * runtime folder.  Copying the standalone EBOOT here would replace the
     * integration build and reintroduce the cwd/dir.ini problem.
     */
    if (copy_file("ms0:/PSP/GAME/TempGBA4PSP-Mod/gba_bios.bin",
                  "ms0:/PSP/GAME/GAMESHELF/gba_bios.bin") < 0) return -1;
    if (copy_file("ms0:/PSP/GAME/GAMESHELF/gba_bios.bin",
                  "ms0:/PSP/GAME/GAMESHELF/TempGBA/gba_bios.bin") < 0) return -1;
    if (copy_file("ms0:/PSP/GAME/TempGBA4PSP-Mod/tempgba_me.prx",
                  "ms0:/PSP/GAME/GAMESHELF/tempgba_me.prx") < 0) return -1;
    if (copy_file("ms0:/PSP/GAME/GAMESHELF/tempgba_me.prx",
                  "ms0:/PSP/GAME/GAMESHELF/TempGBA/tempgba_me.prx") < 0) return -1;
    if (copy_file("ms0:/PSP/GAME/TempGBA4PSP-Mod/cp_fix_gbk.prx",
                  "ms0:/PSP/GAME/GAMESHELF/cp_fix_gbk.prx") < 0) return -1;
    if (copy_file("ms0:/PSP/GAME/GAMESHELF/cp_fix_gbk.prx",
                  "ms0:/PSP/GAME/GAMESHELF/TempGBA/cp_fix_gbk.prx") < 0) return -1;
    if (copy_file("ms0:/PSP/GAME/TempGBA4PSP-Mod/exception.prx",
                  "ms0:/PSP/GAME/GAMESHELF/exception.prx") < 0) return -1;
    if (copy_file("ms0:/PSP/GAME/GAMESHELF/exception.prx",
                  "ms0:/PSP/GAME/GAMESHELF/TempGBA/exception.prx") < 0) return -1;
    return 0;
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
                           label, present ? "OK" : "MANCANTE", path);
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
        set_status("Diagnostica salvata in GBA_DIAG.TXT");
    } else {
        set_status("Impossibile salvare GBA_DIAG.TXT");
    }
}

static void launch_selected(void)
{
    SceKernelLoadExecVSHParam param;
    char path[ENTRY_LENGTH];
    char argument[ENTRY_LENGTH];
    int has_argument = 0;
    int result;

    if (view_mode == 2) {
        if (cursor >= gba_count) {
            set_status("Nessuna ROM GBA selezionata.");
            return;
        }
        if (prepare_gba_runtime(gba_roms[cursor]) < 0) {
            set_status("Preparazione GBA fallita: usa GBA_DIAG.TXT");
            write_gba_diagnostics();
            return;
        }
        snprintf(path, sizeof(path),
                 "ms0:/PSP/GAME/GAMESHELF/TempGBA/EBOOT.PBP");
        argument[0] = '\0';
        sceIoChdir("ms0:/PSP/GAME/GAMESHELF");
        goto launch_application;
    }

    if (view_mode == 3) {
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
            set_status("Nessun ISO selezionato.");
            return;
        }
        snprintf(path, sizeof(path), "ms0:/ISO/%s", iso_games[cursor]);
        if (write_launch_request(path) == 0) {
            set_status("ISO selezionato. Avvialo dal menu Memory Stick.");
        } else {
            set_status("Impossibile scrivere LAUNCH.TXT.");
        }
        return;
    }

    if (cursor >= homebrew_count) {
        set_status("Nessun titolo selezionato.");
        return;
    }
    snprintf(path, sizeof(path), "ms0:/PSP/GAME/%s/EBOOT.PBP",
             homebrew[cursor]);
    strcpy(argument, path);
    has_argument = 1;
launch_application:
    memset(&param, 0, sizeof(param));
    param.size = sizeof(param);
    param.args = has_argument ? strlen(argument) + 1 : 0;
    param.argp = has_argument ? argument : NULL;
    param.key = "game";
    result = sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_MS2, path, &param);
    if (result < 0) {
        snprintf(status_message, STATUS_LENGTH, "Errore loader: 0x%08X",
                 (unsigned int)result);
    }
}

static void draw_screen(void)
{
    int index;
    int last_index;
    int total = view_mode == 0 ? iso_count :
                (view_mode == 1 ? homebrew_count :
                 (view_mode == 2 ? gba_count : nes_count));

    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("PSP GameShelf v0.51.0\n");
    pspDebugScreenPrintf("PSP-3004 / TA-095v2 / FW 6.61\n\n");
    if (view_mode == 0) {
        pspDebugScreenPrintf("Vista: PSP ISO / CSO (%d)\n", iso_count);
        for (index = list_offset; index < iso_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, iso_games[index]);
        }
    } else if (view_mode == 1) {
        pspDebugScreenPrintf("Vista: Applicazioni / Homebrew (%d)\n", homebrew_count);
        for (index = list_offset; index < homebrew_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, homebrew[index]);
        }
    } else if (view_mode == 2) {
        pspDebugScreenPrintf("Vista: Game Boy Advance (%d)\n", gba_count);
        for (index = list_offset; index < gba_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, gba_roms[index]);
        }
    } else {
        pspDebugScreenPrintf("Vista: Nintendo Entertainment System (%d)\n", nes_count);
        for (index = list_offset; index < nes_count && index < list_offset + 18; index++) {
            pspDebugScreenPrintf("%c %02d. %s\n", cursor == index ? '>' : ' ', index + 1, nes_roms[index]);
        }
    }
    last_index = total < list_offset + 18 ? total : list_offset + 18;
    if (total > 18) {
        pspDebugScreenPrintf("Elementi %d-%d di %d\n", list_offset + 1,
                             last_index, total);
    }
    pspDebugScreenPrintf("\nSinistra/Destra: cambia vista\n");
    pspDebugScreenPrintf("Su/Giu: scegli   X: seleziona/diagnostica\n");
    pspDebugScreenPrintf("Triangolo: aggiorna   HOME: esci\n");
    pspDebugScreenPrintf("Stato: %s\n", status_message);
    if (selected >= 0 && selected < total) pspDebugScreenPrintf("Selezionato.\n");
}

int main(void)
{
    SceCtrlData pad;

    pspDebugScreenInit();
    setup_callbacks();
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
                (view_mode == 1 ? homebrew_count :
                 (view_mode == 2 ? gba_count : nes_count));

        if (pressed & PSP_CTRL_LEFT) {
            view_mode--;
            if (view_mode < 0) view_mode = 3;
            cursor = 0;
            list_offset = 0;
            selected = -1;
            if (view_mode == 2 && !gba_scanned) scan_gba_roms();
            if (view_mode == 3 && !nes_scanned) scan_nes_roms();
            draw_screen();
        }
        if (pressed & PSP_CTRL_RIGHT) {
            view_mode++;
            if (view_mode > 3) view_mode = 0;
            cursor = 0;
            list_offset = 0;
            selected = -1;
            if (view_mode == 2 && !gba_scanned) scan_gba_roms();
            if (view_mode == 3 && !nes_scanned) scan_nes_roms();
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
            set_status("Aggiornamento disponibile al prossimo avvio.");
        }
        if (refresh) {
            scan_library();
            total = view_mode == 0 ? iso_count :
                    (view_mode == 1 ? homebrew_count :
                     (view_mode == 2 ? gba_count : nes_count));
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
