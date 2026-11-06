#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
static void render(FILE *f,unsigned frames){int32_t out[512];while(frames){unsigned n=frames>256?256:frames;core_render(out,n,4096);assert(!core_worker_fault());if(f)assert(fwrite(out,8,n,f)==n);frames-=n;}}
int main(int argc,char**argv){assert(argc==2);for(unsigned i=1;i<PW_PRESET_COUNT;i++){
 core_settings c;assert(pw_preset_get(i,&c));core_init();core_set(&c);char path[512];snprintf(path,sizeof(path),"%s/preset-%02u.raw",argv[1],i);FILE *f=fopen(path,"wb");assert(f);
 render(f,4410);core_midi(0,0x90,48,100);render(f,44100*7);core_midi(0,0x80,48,0);render(f,44100*3);
 core_panic();render(f,4410);
 for(unsigned n=0;n<4;n++)core_midi(0,0x90,48+(unsigned[]){0,4,7,12}[n],127);
 render(f,44100*7);for(unsigned n=0;n<4;n++)core_midi(0,0x80,48+(unsigned[]){0,4,7,12}[n],0);render(f,44100*4);
 core_panic();render(f,4410);fclose(f);printf("rendered %u %s\n",i,pw_preset_name(i));fflush(stdout);
 }return 0;}
