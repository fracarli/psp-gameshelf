# PSP GameShelf

I giochi NES e GBA direttamente in **Gioco → Memory Stick** sulla PSP:
scegli un titolo e parte il relativo emulatore con il gioco già caricato,
senza passare dal catalogo di GameShelf o dal menu dell’emulatore.

**Avvio diretto NES e GBA confermato su PSP reale dall’utente.**
Target del progetto: PSP-3004, firmware 6.61 con PRO-C attivo.
Dopo uno spegnimento completo può essere necessario eseguire FastRecovery.

## Versioni

| Versione | Stato | Contenuto |
| --- | --- | --- |
| [0](releases/v0/00/) | Base stabile | Avvio dal catalogo, precedentemente v0.51.0 |
| [1.00](releases/v1/00/) | Stabile: NES e GBA provati su PSP | Generazione delle singole voci nella Memory Stick |
| [1.01](releases/v1/01/) | Copertine confermate su PSP | Copertina del gioco nell’icona XMB; aggiornamento delle voci 1.00 |
| [1.02](releases/v1/02/) | Confermata su PSP | Icone quadrate senza cornice rettangolare |
| [1.03](releases/v1/03/) | Sfondi confermati su PSP | Sfondo a schermo intero alla selezione del gioco |
| [1.04](candidates/1.04/) | Da confermare su PSP | Salvataggi, stati e configurazioni GBA con il nome del gioco |

`releases/` contiene soltanto le versioni stabili e l’adattatore NesterJ
funzionante. Le build in prova sono separate in `candidates/`.
Le vecchie build sono state rimosse dalla struttura corrente; quelle tracciate
restano nella cronologia Git. La numerazione prosegue con 1.04, 1.05, ecc.

## Utilizzo

1. Installa la [1.00 seguendo le istruzioni](releases/v1/00/INSTALL.txt).
2. Per provare le copertine, sostituisci l’EBOOT principale con quello della
   [1.03 stabile](releases/v1/03/INSTALL.txt). Gli emulatori rimangono condivisi.
   La [1.04](candidates/1.04/INSTALL.txt) richiede invece di aggiornare anche il runtime TempGBA.
3. Apri GameShelf e premi **X** per creare o aggiornare le voci dei giochi.
4. Al termine premi **X** per tornare alla home e apri **Gioco → Memory Stick**.
5. Seleziona direttamente il gioco. Le voci persistono dopo il riavvio.

Con **Cerchio** dalla schermata iniziale puoi ancora aprire il vecchio catalogo.

| Sistema | ROM | Emulatore richiesto |
| --- | --- | --- |
| NES | `PSP/GAME/Nester_AoEX_R3/roms/*.nes` | NesterJ con adattamento autoload |
| GBA | `PSP/GAME/TempGBA4PSP-Mod/roms/*.gba` | TempGBA integrato in `PSP/GAME/GAMESHELF/TempGBA/` |

Non sostituire gli emulatori adattati con gli eseguibili standard:
la selezione automatica dipende dagli adattamenti inclusi nel progetto.

## Copertine e sfondi nella 1.03

Sul Mac, con la scheda montata:

```sh
python3 tools/fetch_boxart.py --sd-root /Volumes/SD-FAT32 --platform nes --platform gba
```

Per aggiornare solo l’EBOOT dalla scheda collegata, con backup automatico:

```sh
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.03 --backup-root ../PSP_BACKUP
```

Il programma delle copertine recupera le immagini disponibili e crea le texture `.gsh` in
`PSP/GAME/GAMESHELF/boxart/{nes,gba}/`. Il nome deve coincidere con quello
della ROM senza estensione. Poi avvia GameShelf 1.03 sulla PSP e premi X:
aggiorna anche i collegamenti creati con la 1.00.

La copertina appare come icona quadrata PNG 80×80 senza cornice rettangolare.
Le immagini quadrate riempiono l’icona; quelle rettangolari vengono ritagliate
al centro senza deformazioni. Le texture dedicate sono i file `.xmb.gsh`.
Per i titoli senza un’immagine corrispondente rimane l’icona GameShelf;
gli hack possono richiedere una copertina fornita manualmente.
La 1.03 inserisce anche un’immagine **PIC1 a 480×272** nel collegamento:
quando resti sul gioco selezionato, la XMB può mostrarla a schermo intero.
È un ingrandimento della copertina, ritagliato senza deformazioni e scurito
per la leggibilità. Il file preparato dal helper è `NOME.pic1.png`.
Senza un’immagine valida il collegamento conserva l’icona e rimane avviabile.

Le immagini scaricate rimangono sulla scheda e non fanno parte del repository.

## Salvataggi GBA nella 1.04 (in prova)

La 1.04 aggiorna insieme launcher e runtime TempGBA. L’installer crea un backup
completo e archivia le vecchie voci GBA prima di rimuoverle; aprire GameShelf
sulla PSP e premere X per rigenerarle con il nuovo protocollo.

```sh
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.04 --backup-root ../PSP_BACKUP
```

La ROM temporanea rimane `game.gba`, ma l’emulatore riceve il nome originale
attraverso `SAVE_ID.TXT` e lo verifica contro dimensione e header della ROM.
Ogni gioco usa il proprio `.sav`, stato, configurazione e screenshot nelle
cartelle esistenti di `GAMESHELF/TempGBA/`. Il nome della ROM è limitato a
180 byte; rinominarla cambia anche il nome del salvataggio associato.

I vecchi `game.sav` e stati condivisi restano conservati, senza assegnarli
automaticamente a un titolo. Per ripristinare la 1.03 bisogna usare il backup
completo di launcher, runtime e voci GBA: il downgrade del solo EBOOT è rifiutato.
La prova richiesta è **A → salva → B → salva → A**, poi di nuovo B, verificando
anche gli stati. Finché non viene confermata su console, la 1.03 resta stabile.

## Limiti attuali

- Massimo 32 ROM per sistema, nella cartella prevista e senza sottocartelle.
- Le voci usano una piccola copia del launcher; gli emulatori restano condivisi.
- Le voci di ROM eliminate vanno rimosse manualmente dalle cartelle
  `PSP/GAME/GSH_GBA_*` o `GSH_NES_*`.
- Per GBA usa il menu interno **Quit/Exit**: permane la limitazione del popup HOME.
- Nelle versioni fino alla 1.03 i salvataggi GBA possono condividere il nome
  `game.sav`; la 1.04 introduce nomi separati, ancora da verificare su console.
- ISO e PS1 si avviano normalmente dalla XMB, ma il loro avvio attraverso
  GameShelf rimane irrisolto sul PRO-C provato.
- GB/GBC, Atari, Mega Drive e SNES restano nel vecchio catalogo: per questi
  sistemi manca ancora la selezione automatica della ROM nell’emulatore.

## Sorgenti e verifica

```sh
git clone --recurse-submodules https://github.com/fracarli/psp-gameshelf.git
cd psp-gameshelf
# Con PSPDEV/PSPSDK installato e i tool nel PATH:
make
python3 tools/verify_xmb.py
python3 tools/verify_gba_saves.py
python3 tools/verify_install.py
```

Il Makefile usa `~/pspdev` per impostazione predefinita; puoi impostare `PSPDEV`
per un’altra installazione. Il sorgente corrente genera la versione 1.04.

I test eseguono il generatore C sul computer e controllano: aggiornamento delle
voci 1.00, metadati PBP/SFO, integrità dell’eseguibile, PNG e colori delle
copertine, fallback senza immagini e rifiuto di collisioni e input invalidi.
Gli sfondi della 1.03 sono stati confermati sulla console dall’utente.

Gli emulatori sono submodule dei progetti originali; gli adattamenti del codice
sono conservati in [patches/](patches/README.md).
[Storia tecnica della versione 0](docs/history-v0.md).

## Contenuti pubblicati

Questo repository contiene esclusivamente codice, documentazione e pacchetti
PSP GameShelf. Non contiene ROM, ISO, BIOS, salvataggi, copertine scaricate,
backup della scheda o altri file del Desktop.

Codice GameShelf: [MIT](LICENSE). Gli emulatori conservano le licenze dei
rispettivi progetti originali.
