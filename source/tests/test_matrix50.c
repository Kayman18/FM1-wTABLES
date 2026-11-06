#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
static int32_t pcm[512],reference[512];
static void frames(unsigned n){while(n){unsigned k=n>256?256:n;core_render(pcm,k,4096);assert(pw_tables_ready()&&!core_worker_fault());n-=k;}}
static int position_for(unsigned dest,unsigned note,int amount){core_init();core_settings c;pw_preset_default(&c);c.position=50;c.matrix_source[0]=3;c.matrix_dest[0]=dest;c.matrix_amount[0]=amount;core_set(&c);core_midi(0,0x90,note,100);frames(22050);for(unsigned i=0;i<8;i++)if(live(&slots[i].main))return slots[i].main.osc.position_q8;abort();}
int main(void){assert(PW_PRESET_COUNT==40);core_settings c;
 for(unsigned p=0;p<PW_PRESET_COUNT;p++){assert(pw_preset_get(p,&c));assert(strcmp(pw_preset_name(p),"Simplemind FM1"));for(unsigned r=0;r<4;r++)assert(c.matrix_dest[r]==0&&c.matrix_amount[r]==0);}
 assert(!strcmp(pw_preset_name(1),"Koto"));
 int base=position_for(0,60,0),a=position_for(3,60,50),b=position_for(4,60,50);assert(a==b&&a!=base);assert(position_for(3,60,-50)!=a);
 assert(position_for(5,48,100)==position_for(0,48,0));assert(position_for(5,72,100)!=position_for(0,72,0));
 for(unsigned src=1;src<=11;src++)for(unsigned dest=1;dest<=5;dest++){
 core_init();pw_preset_get(1,&c);c.voice_limit=6;c.bloom_mode=1;c.matrix_source[0]=src;c.matrix_dest[0]=dest;c.matrix_amount[0]=100;core_set(&c);c.playing=1;core_set(&c);frames(22050);for(unsigned d=0;d<5;d++)assert(isfinite(matrix.value[d]));c.matrix_dest[0]=0;core_set(&c);frames(1024);for(unsigned d=0;d<5;d++)assert(matrix.value[d]==0);
 }
 for(unsigned mode=0;mode<2;mode++){
 core_init();core_parallel_set(mode);pw_preset_get(1,&c);c.voice_limit=6;c.bloom_mode=1;for(unsigned r=0;r<4;r++){c.matrix_source[r]=(unsigned[]){3,6,9,11}[r];c.matrix_dest[r]=(unsigned[]){3,4,5,1}[r];c.matrix_amount[r]=(int[]){100,-40,100,25}[r];}core_set(&c);c.playing=1;core_set(&c);
 for(unsigned block=0;block<800;block++){if(block%40==0){assert(pw_preset_apply((block/40)%40,&c));assert(c.playing&&c.bloom_mode&&c.matrix_dest[2]==5);core_set(&c);}frames(256);for(unsigned i=0;i<8;i++)if(live(&slots[i].main))assert(slots[i].main.osc.position_q8>=0&&slots[i].main.osc.position_q8<=1791);}
 if(!mode)memcpy(reference,pcm,sizeof(pcm));else assert(!memcmp(reference,pcm,sizeof(pcm)));
 }
 puts("PASS: 40 slots/NONE defaults/Simplemind removed; wave-position aliases, signed amounts, pitch-wave note tracking, all 55 source/destination pairs, disconnect settling, bounds and live preset preservation.");
}
