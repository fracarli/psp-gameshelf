/* First-install transaction. Existing files are never replaced. */
#ifndef GSH_INSTALL_H
#define GSH_INSTALL_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "payload.h"

static char install_error[256];
static unsigned char install_buffer[65536];
static uint32_t install_crc(uint32_t crc, const unsigned char *data, size_t n)
{
    size_t i; int bit;
    for(i=0;i<n;i++) {
        crc^=data[i];
        for(bit=0;bit<8;bit++) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return crc;
}
static int install_path(char out[512], const char *root, const char *rel, int stage)
{
    int n;
    if(strncmp(rel,"PSP/",4) || strstr(rel,"..") || strchr(rel,':') || strchr(rel,'\\')) return -1;
    n=snprintf(out,512,"%s/%s%s",root,rel,stage?".gshnew":"");
    return n<0 || n>=512 ? -1:0;
}
static int install_parents(const char *path)
{
    char part[512]; size_t i;
    if(strlen(path)>=sizeof(part)) return -1;
    strcpy(part,path);
    for(i=1;part[i];i++) if(part[i]=='/') {
        if(part[i-1]==':')continue; /* Device roots already exist on PSP. */
        part[i]=0;
        if(mkdir(part,0777)<0 && errno!=EEXIST) return -1;
        part[i]='/';
    }
    return 0;
}
static int install_file_matches(const char *path, uint32_t size, uint32_t expected)
{
    FILE *f=fopen(path,"rb"); uint32_t crc=~0u,total=0; size_t n;
    if(!f) return 0;
    while((n=fread(install_buffer,1,sizeof(install_buffer),f))>0) {
        if(n>size-total) { fclose(f); return 0; }
        total+=(uint32_t)n;crc=install_crc(crc,install_buffer,n);
    }
    { int ok=!ferror(f)&&total==size&&(crc^~0u)==expected;fclose(f);return ok; }
}
/* progress is preparation/extraction progress, not emulator launch time. */
static int install_payload(const char *pbp, const char *root, void (*progress)(unsigned int))
{
    FILE *source=NULL,*out=NULL; unsigned char h[40];
    unsigned char owned[GSH_PAYLOAD_COUNT],committed[GSH_PAYLOAD_COUNT];
    uint32_t psar,previous=40; unsigned int i,j; long length;
    char target[512],stage[512]; const char *reason="Invalid installer payload";
    memset(owned,0,sizeof(owned));memset(committed,0,sizeof(committed));install_error[0]=0;
    source=fopen(pbp,"rb");
    if(!source || fread(h,1,40,source)!=40 || memcmp(h,"\0PBP",4)) goto fail;
    for(i=0;i<8;i++) {
        uint32_t offset=(uint32_t)h[8+4*i]|((uint32_t)h[9+4*i]<<8)|((uint32_t)h[10+4*i]<<16)|((uint32_t)h[11+4*i]<<24);
        if(offset<previous)goto fail;
        previous=offset;
    }
    psar=previous;
    if(fseek(source,0,SEEK_END) || (length=ftell(source))<0 || (uint32_t)length<psar || (uint32_t)length-psar!=GSH_PAYLOAD_SIZE)goto fail;
    /* Inspect every destination before any installation writes. */
    for(i=0;i<GSH_PAYLOAD_COUNT;i++) {
        struct stat st; const GshPayloadFile *p=&gsh_payload[i];
        if(p->offset>GSH_PAYLOAD_SIZE || p->size>GSH_PAYLOAD_SIZE-p->offset ||
           install_path(target,root,p->path,0) || install_path(stage,root,p->path,1))goto fail;
        reason="Existing file differs; use the backup-preserving updater";
        if(stat(target,&st)==0) {
            if(!S_ISREG(st.st_mode) || !install_file_matches(target,p->size,p->crc))goto file_fail;
        } else {
            if(errno!=ENOENT) { reason="Cannot inspect destination";goto file_fail; }
            reason="An unfinished staging file needs review";
            if(stat(stage,&st)==0 || errno!=ENOENT)goto file_fail;
            owned[i]=1;
        }
    }
    for(i=0;i<GSH_PAYLOAD_COUNT;i++) if(owned[i]) {
        const GshPayloadFile *p=&gsh_payload[i];uint32_t left=p->size,crc=~0u;
        install_path(stage,root,p->path,1);
        reason="Cannot prepare files; check free space and card access";
        if(install_parents(stage) || fseek(source,psar+p->offset,SEEK_SET))goto file_fail;
        out=fopen(stage,"wb");if(!out)goto file_fail;
        while(left) {
            size_t n=left>sizeof(install_buffer)?sizeof(install_buffer):left;
            if(fread(install_buffer,1,n,source)!=n || fwrite(install_buffer,1,n,out)!=n)goto file_fail;
            crc=install_crc(crc,install_buffer,n);left-=n;
        }
        if(fclose(out)) {out=NULL;goto file_fail;}out=NULL;
        reason="Installer checksum verification failed";
        if((crc^~0u)!=p->crc || !install_file_matches(stage,p->size,p->crc))goto file_fail;
        if(progress)progress((i+1)*90/GSH_PAYLOAD_COUNT);
    }
    for(i=0;i<GSH_DIRECTORY_COUNT;i++) {
        if(install_path(target,root,gsh_directories[i],0) || install_parents(target))goto fail;
    }
    for(i=0;i<GSH_PAYLOAD_COUNT;i++) if(owned[i]) {
        install_path(target,root,gsh_payload[i].path,0);install_path(stage,root,gsh_payload[i].path,1);
        reason="Cannot complete installation";
        if(rename(stage,target))goto file_fail;
        committed[i]=1;
    }
    fclose(source);if(progress)progress(100);return 0;
file_fail:
    snprintf(install_error,sizeof(install_error),"%s:\n%.150s",reason,gsh_payload[i].path);
fail:
    if(out)fclose(out);
    if(source)fclose(source);
    if(!install_error[0])snprintf(install_error,sizeof(install_error),"%s",reason);
    for(j=0;j<GSH_PAYLOAD_COUNT;j++) if(owned[j]) {
        if(!install_path(stage,root,gsh_payload[j].path,1))remove(stage);
        if(committed[j] && !install_path(target,root,gsh_payload[j].path,0))remove(target);
    }
    return -1;
}
#endif
