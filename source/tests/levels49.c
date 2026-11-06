#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
int main(void){int32_t pcm[512];for(unsigned p=15;p<PW_PRESET_COUNT;p++)for(unsigned voices=1;voices<=6;voices+=5){core_init();core_settings c;pw_preset_get(p,&c);c.voice_limit=6;core_set(&c);for(unsigned b=0;b<40;b++)core_render(pcm,256,4096);for(unsigned n=0;n<voices;n++)core_midi(0,0x90,(unsigned[]){48,55,60,64,67,72}[n],100);double peak=0,power=0;unsigned clips=0;for(unsigned b=0;b<520;b++){core_render(pcm,256,4096);for(unsigned j=0;j<512;j++){double x=(double)pcm[j]/8388608.;if(fabs(x)>peak)peak=fabs(x);power+=x*x;if(abs(pcm[j])>=8388607)clips++;}}printf("%u,%s,%u,%.8f,%.8f,%u\n",p,pw_preset_name(p),voices,peak,sqrt(power/(520*512)),clips);}}
