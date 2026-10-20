#include "language.h"
#include "../shared/extra_launch.h"
/* GameShelf 1.10: generate per-ROM XMB EBOOTs, no emulator duplication. */
static unsigned int read32(const unsigned char *p)
{
    return p[0] | ((unsigned int)p[1]<<8) | ((unsigned int)p[2]<<16) | ((unsigned int)p[3]<<24);
}
static void put32(unsigned char *p, unsigned int v)
{
    p[0]=v; p[1]=v>>8; p[2]=v>>16; p[3]=v>>24;
}
static unsigned int rom_id(const char *s, int mode)
{
    unsigned int h = 2166136261u ^ mode;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}
static const char *gsh_system(int mode)
{ return mode==3?"GBA":mode==4?"GB":mode==5?"GBC":mode==6?"ATARI":mode==7?"MD":mode==8?"SNES":mode==10?"N64":"NES"; }
#include "xmb_icon.h"
#include "xmb_cleanup.h"

static int xmb_replace_section(unsigned char **buffer, unsigned int *size,
                               unsigned int section, const unsigned char *data, unsigned int length)
{
    unsigned char *b=*buffer, *expanded;
    unsigned int start=read32(b+8+section*4), end=read32(b+12+section*4), i;
    unsigned int new_size;
    if(section>=7 || start<40 || end<start || end>*size) return -1;
    new_size=*size-(end-start)+length;
    expanded=realloc(b,new_size>*size?new_size:*size);
    if(!expanded)return -1;
    b=expanded;*buffer=b;
    memmove(b+start+length,b+end,*size-end);
    memcpy(b+start,data,length);
    for(i=section+1;i<8;i++)put32(b+8+i*4,read32(b+8+i*4)-(end-start)+length);
    *size=new_size;return 0;
}

static int make_xmb_entry(const char *rom, int mode)
{
    unsigned char *b;
    int updating=0;
    SceUID f;
    SceIoStat st;
    unsigned int size, i, target=0, hits=0, sfo, end, keys, data, count;
    char dir[128], path[160], temporary[164], backup[164], title[128];
    unsigned int title_entry=0, oldmax=0, value=0, growth;
    const char *base="ms0:/PSP/GAME/GAMESHELF/EBOOT.PBP";
    if (!rom[0] || rom[0]=='.' || strlen(rom) >= 256 || strchr(rom, '/') || strchr(rom, ':') || strchr(rom, '\\') || (mode<3 || mode>10)) return -1;
    memset(&st, 0, sizeof(st));
    if (sceIoGetstat(base, &st)<0 || st.st_size<40 || st.st_size>2097152) return -1;
    size=(unsigned int)st.st_size;
    b=malloc(size+128);
    if (!b) return -1;
    f=sceIoOpen(base, PSP_O_RDONLY, 0);
    if (f<0) { free(b); return -1; }
    i=sceIoRead(f,b,size); sceIoClose(f);
    if (i!=size || read32(b)!=0x50425000) goto fail;
    for (i=40;i+sizeof(xmb_target)<=size;i++) {
        if (memcmp(b+i,(const void *)xmb_target.magic,16)==0) { target=i; hits++; }
    }
    if (hits!=1 || read32(b+target+16)!=0) goto fail;
    put32(b+target+16,mode);
    memset(b+target+20,0,256); strcpy((char *)b+target+20,rom);
    /* Expand the final SFO TITLE value, keeping executable bytes unchanged. */
    sfo=read32(b+8); end=read32(b+12);
    if (sfo>size-20 || end>size || end<=sfo || read32(b+sfo)!=0x46535000) goto fail;
    keys=read32(b+sfo+8); data=read32(b+sfo+12); count=read32(b+sfo+16);
    if (count>64 || sfo+20+count*16>end || keys>=end-sfo || data>=end-sfo) goto fail;
    for(i=0;i<count;i++) {
        unsigned int e=sfo+20+i*16, k=b[e]|(b[e+1]<<8);
        if (keys+k+6<=end-sfo && memcmp(b+sfo+keys+k,"TITLE",6)==0) {
            title_entry=e; oldmax=read32(b+e+8); value=sfo+data+read32(b+e+12); break;
        }
    }
    if (!title_entry || !oldmax || oldmax>128 || value>end || oldmax!=end-value) goto fail;
    growth=128-oldmax;
    memmove(b+end+growth,b+end,size-end);
    size+=growth;
    for (i=1;i<8;i++) put32(b+8+i*4,read32(b+8+i*4)+growth);
    /* Display aliases never alter the launch path, entry hash or save identity. */
    snprintf(title,sizeof(title),"[%s] %.115s",gsh_system(mode),
             mode==6 && !strcmp(rom,"Moonptrl.bin") ? "Moon Patrol (1983) (Atari)" : rom);
    { char *dot=strrchr(title,'.'); if(dot) *dot=0; }
    memset(b+value,0,128); strcpy((char *)b+value,title);
    put32(b+title_entry+4,strlen(title)+1); put32(b+title_entry+8,128);
    {
        unsigned int image_size=0;
        unsigned char *image=xmb_cover_png(rom,mode,&image_size);
        if(image) {
            int result=xmb_replace_section(&b,&size,1,image,image_size);
            free(image);if(result<0)goto fail;
        }
        image=xmb_background_png(rom,mode,&image_size);
        if(image) {
            int result=xmb_replace_section(&b,&size,4,image,image_size);
            free(image);if(result<0)goto fail;
        }
    }
    snprintf(dir,sizeof(dir),"ms0:/PSP/GAME/GSH_%s_%08X",gsh_system(mode),rom_id(rom,mode));
    sceIoMkdir(dir,0777);
    snprintf(path,sizeof(path),"%s/EBOOT.PBP",dir);
    snprintf(temporary,sizeof(temporary),"%s/NEW.PBP",dir);
    snprintf(backup,sizeof(backup),"%s/OLD.PBP",dir);
    /* Replace only our own entry for this exact ROM; refuse hash collisions. */
    if (sceIoGetstat(path,&st)>=0) {
        unsigned char *old;
        unsigned int old_size, old_target=0, old_hits=0;
        if(st.st_size<40 || st.st_size>2097152) goto fail;
        old_size=st.st_size;old=malloc(old_size);
        if(!old) goto fail;
        f=sceIoOpen(path,PSP_O_RDONLY,0);
        if(f<0) { free(old);goto fail; }
        i=sceIoRead(f,old,old_size);sceIoClose(f);
        if(i==old_size && read32(old)==0x50425000) {
            for(i=40;i+sizeof(xmb_target)<=old_size;i++) {
                if(memcmp(old+i,(const void *)xmb_target.magic,16)==0) { old_target=i;old_hits++; }
            }
        }
        if(old_hits==1) updating=read32(old+old_target+16)==(unsigned int)mode &&
            memchr(old+old_target+20,0,256)!=NULL &&
            strcmp((char *)old+old_target+20,rom)==0;
        free(old);
        if(!updating) goto fail;
    }
    if(sceIoGetstat(backup,&st)>=0) goto fail;
    f=sceIoOpen(temporary,PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC,0777);
    if(f<0) goto fail;
    i=sceIoWrite(f,b,size); sceIoClose(f);
    if(i!=size) { sceIoRemove(temporary); goto fail; }
    if(updating && sceIoRename(path,backup)<0) { sceIoRemove(temporary);goto fail; }
    if(sceIoRename(temporary,path)<0) {
        if(updating) sceIoRename(backup,path);
        sceIoRemove(temporary);goto fail;
    }
    if(updating) sceIoRemove(backup);
    free(b); return updating?1:0;
fail:
    free(b); return -1;
}
static void generate_xmb(void)
{
    int i, result, created=0, existing=0, failed=0;
    scan_gba_roms(); scan_nes_roms(); scan_gb_roms(); scan_gbc_roms();
    scan_extra_roms(atari_roms,&atari_count,&atari_scanned,"ms0:/PSP/GAME/psp2600/roms",6);
    scan_extra_roms(megadrive_roms,&megadrive_count,&megadrive_scanned,"ms0:/PSP/GAME/PicoDrive/roms",7);
    scan_extra_roms(snes_roms,&snes_count,&snes_scanned,"ms0:/PSP/GAME/s9xTYLme_mod/roms",8);
    scan_extra_roms(n64_roms,&n64_count,&n64_scanned,"ms0:/PSP/GAME/DaedalusX64/Roms",10);
    {
    struct { int mode,count;char (*roms)[256];const char *runtime; } systems[]={
        {3,gba_count,gba_roms,"ms0:/PSP/GAME/GAMESHELF/TempGBA/EBOOT.PBP"},
        {4,gb_count,gb_roms,"ms0:/PSP/GAME/GAMESHELF/GBAdhoc/EBOOT.PBP"},
        {5,gbc_count,gbc_roms,"ms0:/PSP/GAME/GAMESHELF/GBAdhoc/EBOOT.PBP"},
        {6,atari_count,atari_roms,"ms0:/PSP/GAME/psp2600/EBOOT.PBP"},
        {7,megadrive_count,megadrive_roms,"ms0:/PSP/GAME/PicoDrive/EBOOT.PBP"},
        {8,snes_count,snes_roms,"ms0:/PSP/GAME/s9xTYLme_mod/EBOOT.PBP"},
        {9,nes_count,nes_roms,"ms0:/PSP/GAME/Nester_AoEX_R3/EBOOT.PBP"},
        {10,n64_count,n64_roms,"ms0:/PSP/GAME/DaedalusX64/EBOOT.PBP"}
    };
    int system;
    for(system=0;system<8;system++)for(i=0;i<systems[system].count;i++) {
        int mode=systems[system].mode;
        const char *rom=systems[system].roms[i];
        char path[256];
        if(!diagnostic_exists(systems[system].runtime,0) ||
           (((mode>=6&&mode<=8)||mode==10)&&gsh_extra_path(path,sizeof(path),mode,rom)<0))result=-1;
        else result=make_xmb_entry(rom,mode);
        if(result==0) created++; else if(result==1) existing++; else failed++;
        pspDebugScreenPrintf("%s: %.45s\n",result<0?gsh_text("ERROR"):result?gsh_text("UPDATED"):gsh_text("CREATED"),rom);
    }
    }
    pspDebugScreenPrintf(gsh_text("Archived missing-ROM entries: %d (recoverable).\n"),gsh_cleanup_entries());
    pspDebugScreenPrintf(gsh_text("\nCreated %d, updated %d, errors %d.\n"),created,existing,failed);
    pspDebugScreenPrintf("%s", gsh_text("Up to 32 ROMs per system in this build.\n"));
}
