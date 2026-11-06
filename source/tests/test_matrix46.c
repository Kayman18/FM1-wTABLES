#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static int32_t out[512];
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(out,m,4096);n-=m;assert(!core_worker_fault());assert(core_primary_count()<=6);for(unsigned i=0;i<m*2;i++)assert(out[i]>=-8388608&&out[i]<=8388607);}}
int main(void){core_settings c;assert(sizeof(c)==108);assert(PW_PRESET_COUNT==15);assert(!strcmp(pw_preset_name(2),"Koto"));
for(unsigned p=0;p<PW_PRESET_COUNT;p++){pw_preset_get(p,&c);assert(c.voice_limit<=6);for(unsigned r=0;r<4;r++)assert(!c.matrix_dest[r]&&!c.matrix_amount[r]);}
core_init();pw_preset_default(&c);c.voice_limit=99;c.unison_count=99;core_set(&c);assert(st.voice_limit==6&&st.unison_count==6);
for(unsigned source=1;source<=11;source++)for(unsigned dest=1;dest<=2;dest++){
 core_init();pw_preset_get(2,&c);c.matrix_source[0]=source;c.matrix_dest[0]=dest;c.matrix_amount[0]=100;c.filter.cutoff=60;c.filter.resonance=40;c.filter_env_depth=c.filter_env2_depth=0;c.tracking.velocity_cutoff=0;core_set(&c);core_midi(0,0x90,60,100);frames(44100);
 assert(isfinite(matrix.value[dest-1]));if(source<10)assert(matrix.value[dest-1]!=0.0f);if(source==10)assert(matrix.value[dest-1]<=0);if(source==11)assert(matrix.value[dest-1]>=0);assert(matrix.value[2-dest]==0.0f);
 c.matrix_dest[0]=0;core_set(&c);frames(1024);assert(matrix.value[0]==0&&matrix.value[1]==0);
}
// All four routes, both destinations, signed amounts, preset and live control changes.
core_init();pw_preset_get(8,&c);c.voice_limit=6;c.bloom_mode=1;core_set(&c);c.playing=1;
for(unsigned i=0;i<4;i++){c.matrix_source[i]=1+i*3;c.matrix_dest[i]=1+i%2;c.matrix_amount[i]=i%2?-100:100;}core_set(&c);
for(unsigned b=0;b<3000;b++){if(b%77==0){unsigned p=b%15;assert(pw_preset_apply(p,&c));assert(c.matrix_dest[0]==1);}if(b%11==0)c.matrix_amount[b%4]=(int)(b%201)-100;core_set(&c);frames(256);assert(isfinite(matrix.value[0])&&isfinite(matrix.value[1]));}
puts("PASS: 15 presets/6-voice cap/NONE defaults; all 11 Sloth outputs to both destinations, disconnect silence, four concurrent routes, live preset preservation and 3000 stress blocks.");}
