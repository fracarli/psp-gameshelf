/* English is the source language. Italian is selected only by the PSP setting. */
#ifndef GSH_LANGUAGE_H
#define GSH_LANGUAGE_H
#include <string.h>
static int gsh_language = 1;
static inline void gsh_set_language(int language) { gsh_language = language == 5 ? 5 : 1; }
static inline const char *gsh_text(const char *english)
{
    static const struct { const char *english; const char *italian; } messages[] = {
        { "Ready.", "Pronto." },
        { "Diagnostics saved to GBA_DIAG.TXT", "Diagnostica salvata in GBA_DIAG.TXT" },
        { "Cannot save GBA_DIAG.TXT", "Impossibile salvare GBA_DIAG.TXT" },
        { "No GBA ROM selected.", "Nessuna ROM GBA selezionata." },
        { "GBA setup failed: check GBA_DIAG.TXT", "Preparazione GBA fallita: usa GBA_DIAG.TXT" },
        { "No Game Boy ROM selected.", "Nessuna ROM Game Boy selezionata." },
        { "Failed to copy the GBAdhoc ROM.", "Copia ROM GBAdhoc fallita." },
        { "No Atari game selected.", "Nessun gioco Atari selezionato." },
        { "No Mega Drive game selected.", "Nessun gioco Mega Drive selezionato." },
        { "No SNES game selected.", "Nessun gioco SNES selezionato." },
        { "Failed to copy the emulator ROM.", "Copia ROM emulatore fallita." },
        { "No ISO selected.", "Nessun ISO selezionato." },
        { "ISO not found on the memory card.", "ISO non trovato sulla scheda SD." },
        { "ISO launch request saved for the CFW loader.", "Richiesta ISO salvata. Loader CFW pronto." },
        { "Cannot write LAUNCH.TXT.", "Impossibile scrivere LAUNCH.TXT." },
        { "No PS1 game selected.", "Nessun gioco PS1 selezionato." },
        { "PS1 EBOOT not found.", "EBOOT PS1 non trovato." },
        { "No title selected.", "Nessun titolo selezionato." },
        { "Loader error: 0x%08X", "Errore loader: 0x%08X" },
        { "PSP GameShelf 1.06 - hardware test\n", "PSP GameShelf 1.06 - test PSP\n" },
        { "Entries %d-%d of %d\n", "Elementi %d-%d di %d\n" },
        { "\nLeft/Right: change view\n", "\nSinistra/Destra: cambia vista\n" },
        { "Up/Down: choose   X: select/diagnose\n", "Su/Giu: scegli   X: seleziona/diagnostica\n" },
        { "Triangle: refresh   HOME: exit\n", "Triangolo: aggiorna   HOME: esci\n" },
        { "Covers: PSP/GAME/GAMESHELF/boxart/%s/\n", "Copertine: PSP/GAME/GAMESHELF/boxart/%s/\n" },
        { "Selected cover: OK\n", "Copertina selezionata: OK\n" },
        { "Selected cover: missing\n", "Copertina selezionata: assente\n" },
        { "Status: %s\n", "Stato: %s\n" },
        { "Selected.\n", "Selezionato.\n" },
        { "Invalid game entry.", "Collegamento non valido." },
        { "%s\nPress HOME to exit.\n", "%s\nPremi HOME per uscire.\n" },
        { "GameShelf 1.06\nX: create/update NES/GBA Memory Stick entries\nCircle: legacy catalog   HOME: exit\n", "GameShelf 1.06\nX: crea giochi NES/GBA nella Memory Stick\nCerchio: catalogo precedente   HOME: esci\n" },
        { "\nPress X to return to the home screen.\n", "\nPremi X per tornare alla home.\n" },
        { "The library refreshes at the next startup.", "Aggiornamento disponibile al prossimo avvio." },
        { "ERROR", "ERRORE" },
        { "UPDATED", "AGGIORNATO" },
        { "CREATED", "CREATO" },
        { "\nCreated %d, updated %d, errors %d.\n", "\nCreati %d, aggiornati %d, errori %d.\n" },
        { "Up to 32 ROMs per system in this build.\n", "Massimo 32 ROM per sistema in questa prova.\n" },
        { "%s  View: PSP ISO / CSO (%d)\n", "%s  Vista: PSP ISO / CSO (%d)\n" },
        { "%s  View: PlayStation 1 (%d)\n", "%s  Vista: PlayStation 1 (%d)\n" },
        { "%s  View: Applications / Homebrew (%d)\n", "%s  Vista: Applicazioni / Homebrew (%d)\n" },
        { "%s  View: Game Boy Advance (%d)\n", "%s  Vista: Game Boy Advance (%d)\n" },
        { "%s  View: Game Boy (%d)\n", "%s  Vista: Game Boy (%d)\n" },
        { "%s  View: Game Boy Color (%d)\n", "%s  Vista: Game Boy Color (%d)\n" },
        { "%s  View: Atari 2600 (%d)\n", "%s  Vista: Atari 2600 (%d)\n" },
        { "%s  View: Mega Drive (%d)\n", "%s  Vista: Mega Drive (%d)\n" },
        { "%s  View: Super Nintendo (%d)\n", "%s  Vista: Super Nintendo (%d)\n" },
        { "%s  View: Nintendo Entertainment System (%d)\n", "%s  Vista: Nintendo Entertainment System (%d)\n" },
        { "Preparing game...", "Preparazione del gioco..." },
        { "Checking emulator files...", "Controllo dei file emulatore..." },
        { "Starting emulator...", "Avvio emulatore..." },
        { "Copying ROM: %3u%%                         \n", "Copia ROM: %3u%%                         \n" },
        { "Archived missing-ROM entries: %d (recoverable).\n", "Voci senza ROM archiviate: %d (recuperabili).\n" },
    };
    unsigned int i;
    if(gsh_language != 5)return english;
    for(i=0;i<sizeof(messages)/sizeof(messages[0]);i++)
        if(strcmp(messages[i].english,english)==0)return messages[i].italian;
    return english;
}
#endif
