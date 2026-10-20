/* Recoverably archive owned entries only when the source ROM is missing.
 * Close directory enumeration before inspecting entries (PRO-C compatibility). */
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <strings.h>
static int gsh_owned_entry(const char *folder,char *rom,int *mode)
{
    char path[384],expected[64];unsigned char *data;
    FILE *file;long size;unsigned int i,hits=0,target=0;
    if(strlen(folder)!=16)return 0;
    snprintf(path,sizeof(path),"ms0:/PSP/GAME/%s/EBOOT.PBP",folder);
    file=fopen(path,"rb");if(!file)return 0;
    if(fseek(file,0,SEEK_END)!=0){fclose(file);return 0;}
    size=ftell(file);
    if(size<40||size>2097152){fclose(file);return 0;}
    rewind(file);data=malloc(size);
    if(!data){fclose(file);return 0;}
    if(fread(data,1,size,file)!=(size_t)size){free(data);fclose(file);return 0;}
    fclose(file);
    if(read32(data)!=0x50425000){free(data);return 0;}
    for(i=40;i+sizeof(xmb_target)<=(unsigned long)size;i++)
        if(memcmp(data+i,(const void *)xmb_target.magic,16)==0){hits++;target=i;}
    if(hits!=1){free(data);return 0;}
    *mode=read32(data+target+16);
    if((*mode!=3&&*mode!=9)||!memchr(data+target+20,0,256)){free(data);return 0;}
    strcpy(rom,(char *)data+target+20);free(data);
    if(!rom[0]||rom[0]=='.'||strchr(rom,'/')||strchr(rom,'\\')||strchr(rom,':'))return 0;
    for(i=0;rom[i];i++)if((unsigned char)rom[i]<32)return 0;
    {const char *dot=strrchr(rom,'.');if(!dot||strcasecmp(dot,*mode==3?".gba":".nes"))return 0;}
    snprintf(expected,sizeof(expected),"GSH_%s_%08X",*mode==3?"GBA":"NES",rom_id(rom,*mode));
    return strcmp(folder,expected)==0;
}
static int gsh_clean_entry_folder(const char *path)
{
    DIR *dir=opendir(path);struct dirent *entry;int clean=1;
    if(!dir)return 0;
    errno=0;
    while((entry=readdir(dir))!=NULL){
        const char *n=entry->d_name;
        if(!strcmp(n,".")||!strcmp(n,"..")||!strcmp(n,"EBOOT.PBP"))continue;
        if(strcmp(n,".DS_Store")&&strncmp(n,"._",2)){clean=0;break;}
        {char p[768];struct stat st;snprintf(p,sizeof(p),"%s/%s",path,n);
         if(stat(p,&st)<0||!S_ISREG(st.st_mode)){clean=0;break;}}
    }
    if(errno)clean=0;
    closedir(dir);return clean;
}
static int gsh_archive_entry(const char *folder)
{
    char from[384],to[384];unsigned int index;struct stat st;
    snprintf(from,sizeof(from),"ms0:/PSP/GAME/%s",folder);
    if(!gsh_clean_entry_folder(from))return 0;
    if(mkdir("ms0:/PSP/GAMESHELF",0777)<0&&errno!=EEXIST)return 0;
    if(mkdir("ms0:/PSP/GAMESHELF/REMOVED_ENTRIES",0777)<0&&errno!=EEXIST)return 0;
    for(index=0;index<1000;index++){
        snprintf(to,sizeof(to),"ms0:/PSP/GAMESHELF/REMOVED_ENTRIES/%s_%03u",folder,index);
        if(stat(to,&st)==0)continue;
        if(errno!=ENOENT)return 0;
        /* Move the complete verified folder, including metadata, without
         * deleting files or mutating an open directory enumeration. */
        return rename(from,to)==0?1:0;
    }
    return 0;
}
static int gsh_cleanup_entries(void)
{
    static char folders[256][17];unsigned int count=0,i;int archived=0;
    DIR *dir=opendir("ms0:/PSP/GAME");struct dirent *entry;
    if(!dir)return 0;
    while((entry=readdir(dir))!=NULL && count<256){
        const char *n=entry->d_name;
        if(strlen(n)==16&&(!strncmp(n,"GSH_GBA_",8)||!strncmp(n,"GSH_NES_",8))){
            strcpy(folders[count++],n);
        }
    }
    closedir(dir);
    for(i=0;i<count;i++){
        char rom[256],source_dir[128],source[384];int mode;struct stat st;
        if(!gsh_owned_entry(folders[i],rom,&mode))continue;
        snprintf(source_dir,sizeof(source_dir),"ms0:/PSP/GAME/%s/roms",mode==3?"TempGBA4PSP-Mod":"Nester_AoEX_R3");
        /* A missing/unreadable library never causes a mass cleanup. */
        if(stat(source_dir,&st)<0||!S_ISDIR(st.st_mode))continue;
        dir=opendir(source_dir);if(!dir)continue;closedir(dir);
        snprintf(source,sizeof(source),"%s/%s",source_dir,rom);
        if(stat(source,&st)==0||errno!=ENOENT)continue;
        archived+=gsh_archive_entry(folders[i]);
    }
    return archived;
}
