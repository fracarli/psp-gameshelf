/* GameShelf save identity protocol, shared by launcher and TempGBA. MIT. */
#ifndef GSH_GBA_IDENTITY_H
#define GSH_GBA_IDENTITY_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define GSH_IDENTITY_PATH "ms0:/PSP/GAME/GAMESHELF/TempGBA/SAVE_ID.TXT"
#define GSH_IDENTITY_LIMIT 180
static inline int gsh_valid_rom_name(const char *name)
{
    size_t i,n=strlen(name);
    if(n<5 || n>GSH_IDENTITY_LIMIT || name[0]=='.' || name[0]==' ')return 0;
    for(i=0;i<n;i++)if((unsigned char)name[i]<32 || name[i]=='/' || name[i]=='\\' || name[i]==':')return 0;
    return name[n-4]=='.' && (name[n-3]=='g'||name[n-3]=='G') &&
        (name[n-2]=='b'||name[n-2]=='B') && (name[n-1]=='a'||name[n-1]=='A');
}
static inline unsigned int gsh_header_id(const unsigned char *header)
{
    unsigned int i,h=2166136261u;
    for(i=0;i<256;i++){h^=header[i];h*=16777619u;}
    return h;
}
static inline int gsh_identity_encode(char *out,size_t capacity,const char *name,
                               unsigned int rom_size,const unsigned char *header)
{
    int length;
    if(!gsh_valid_rom_name(name) || rom_size<256)return -1;
    length=snprintf(out,capacity,"GS104\n%u %08X\n%s\n",rom_size,gsh_header_id(header),name);
    return length>0 && (size_t)length<capacity?length:-1;
}
static inline int gsh_identity_decode(const char *text,size_t length,unsigned int rom_size,
                               const unsigned char *header,char *name,size_t capacity)
{
    unsigned long recorded_size,recorded_hash;
    const char *cursor;char *end;
    size_t n;
    if(length<10 || strlen(text)!=length || strncmp(text,"GS104\n",6)!=0)return -1;
    cursor=text+6;
    if(*cursor<'0'||*cursor>'9')return -1;
    recorded_size=strtoul(cursor,&end,10);
    if(end==cursor || *end!=' ')return -1;
    cursor=end+1;
    if(strlen(cursor)<9)return -1;
    {size_t i;for(i=0;i<8;i++)if(!((cursor[i]>='0'&&cursor[i]<='9')||(cursor[i]>='A'&&cursor[i]<='F')))return -1;}
    recorded_hash=strtoul(cursor,&end,16);
    if(end!=cursor+8 || *end!='\n' || recorded_size!=rom_size || recorded_hash!=gsh_header_id(header))return -1;
    cursor=end+1;n=strlen(cursor);
    if(n<2 || cursor[n-1]!='\n' || n>capacity)return -1;
    memcpy(name,cursor,n-1);name[n-1]=0;
    return gsh_valid_rom_name(name)?0:-1;
}
static inline int gsh_identity_load(const char *path,unsigned int rom_size,
                                    const unsigned char *header,char *name,size_t capacity)
{
    char record[256];size_t length;
    FILE *identity=fopen(path,"rb");
    if(!identity)return -1;
    length=fread(record,1,sizeof(record)-1,identity);
    if(ferror(identity)||!feof(identity)){fclose(identity);return -1;}
    fclose(identity);record[length]=0;
    return gsh_identity_decode(record,length,rom_size,header,name,capacity);
}
#endif
