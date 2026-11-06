#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
int main(void){int32_t pcm[512];for(unsigned p=40;p<PW_PRESET_COUNT;p++)for(unsigned voices=1;voices<=6;voices+=5)for(unsigned base=36;base<=48;base+=12)for(unsigned vel=100;vel<=127;vel+=27){
 core_init();core_settings c;assert(pw_preset_get(p,&c));assert(c.bank>=16&&c.bank<PW_TABLE_BANKS);assert(!strncmp(pw_preset_name(p),"AV - ",5));c.voice_limit=6;core_set(&c);for(unsigned b=0;b<40;b++)core_render(pcm,256,4096);
 for(unsigned n=0;n<voices;n++)core_midi(0,0x90,base+(unsigned[]){0,7,12,16,19,24}[n],vel);
 double peak=0,power=0;unsigned clips=0;for(unsigned b=0;b<520;b++){core_render(pcm,256,4096);for(unsigned j=0;j<512;j++){double x=(double)pcm[j]/8388608.;if(fabs(x)>peak)peak=fabs(x);power+=x*x;if(abs(pcm[j])>=8388607)clips++;}}
 assert(peak>0&&clips==0);printf("%u,%s,%u,%u,%u,%.8f,%.8f,%u\n",p,pw_preset_name(p),voices,base,vel,peak,sqrt(power/(520*512)),clips);
}}
