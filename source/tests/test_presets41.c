#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int32_t pcm[512];
static unsigned frames(unsigned n){unsigned peak=0;while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(!core_worker_fault());assert(core_primary_count()<=8);for(unsigned i=0;i<2*m;i++){unsigned a=pcm[i]<0?-pcm[i]:pcm[i];assert(a<8388607);if(a>peak)peak=a;}}return peak;}
int main(void){core_settings c,copy;memset(&c,0x55,sizeof(c));copy=c;assert(!pw_preset_get(17,&c)&&!memcmp(&c,&copy,sizeof(c)));assert(!pw_preset_get(0,0));
for(unsigned i=0;i<PW_PRESET_COUNT;i++){
 assert(pw_preset_get(i,&c));assert(!c.playing&&!c.bloom_mode&&!c.arp_mode&&!c.chord_shape&&!c.sustain_hold);assert(c.bank<8&&c.position<=100&&c.level<=100);assert(c.delay_time_ms<=550&&c.delay_feedback<=90);assert(c.gain_db>=-40&&c.gain_db<=24);assert(c.voice_limit<=8);
 for(unsigned key=0;key<3;key++){core_init();core_set(&c);for(unsigned n=0;n<8;n++)core_midi(0,0x90,24+key*24+n*3,127);assert(frames(44100*2)>100);for(unsigned n=0;n<8;n++)core_midi(0,0x80,24+key*24+n*3,0);frames(44100);core_panic();frames(512);for(unsigned n=0;n<512;n++)assert(!pcm[n]);}
 // Loading via the same panic/set pair clears old effects and held ownership.
 core_midi(0,0x90,60,127);frames(200);core_panic();core_set(&c);frames(256);assert(!core_primary_count());for(unsigned n=0;n<512;n++)assert(!pcm[n]);
}
printf("PASS: 17 preset records, 51 eight-note low/mid/high playback cases, output headroom, ownership reset, effect clearing and panic silence.\n");return 0;}
