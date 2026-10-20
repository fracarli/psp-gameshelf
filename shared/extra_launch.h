/* Original-path launch contract for the additional shared PSP runtimes. */
#ifndef GSH_EXTRA_LAUNCH_H
#define GSH_EXTRA_LAUNCH_H
#include <stdio.h>
#include <string.h>
#include <strings.h>
static inline const char *gsh_extra_root(int mode)
{
 return mode==6?"ms0:/PSP/GAME/psp2600/roms/":
        mode==7?"ms0:/PSP/GAME/PicoDrive/roms/":
        mode==8?"ms0:/PSP/GAME/s9xTYLme_mod/roms/":NULL;
}
static inline int gsh_extra_extension(int mode,const char *name)
{
 const char *dot=strrchr(name,'.');if(!dot)return 0;
 return mode==6?(!strcasecmp(dot,".a26")||!strcasecmp(dot,".bin")||!strcasecmp(dot,".rom")):
        mode==7?(!strcasecmp(dot,".bin")||!strcasecmp(dot,".md")||!strcasecmp(dot,".gen")||!strcasecmp(dot,".smd")):
        mode==8?(!strcasecmp(dot,".smc")||!strcasecmp(dot,".sfc")):0;
}
/* Leave room for legacy runtime save/state suffixes in 256-byte paths. */
static inline int gsh_extra_valid(int mode,const char *path)
{
 const char *root=gsh_extra_root(mode),*name;unsigned int i;
 if(!root||!path||strncmp(root,path,strlen(root)))return 0;
 name=path+strlen(root);
 if(!name[0]||name[0]=='.'||strlen(path)>=240||!gsh_extra_extension(mode,name))return 0;
 for(i=0;name[i];i++)if((unsigned char)name[i]<32||name[i]=='/'||name[i]=='\\'||name[i]==':')return 0;
 return 1;
}
static inline int gsh_extra_path(char *out,size_t capacity,int mode,const char *name)
{
 const char *root=gsh_extra_root(mode);int length;
 if(!root||!name)return -1;
 length=snprintf(out,capacity,"%s%s",root,name);
 return length<0||(size_t)length>=capacity||!gsh_extra_valid(mode,out)?-1:0;
}
#endif
