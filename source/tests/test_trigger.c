#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../fm1/app/test_engine.h"
static int32_t pcm[512],reference[160][512];
static core_settings settings(void){core_settings c={0};c.note=48;c.position=30;c.level=75;c.env=(pw_env_params){20,30,64,30};c.env2=(pw_env_params){10,20,-64,40};c.filter=(pw_filter_params){0,96,0,100};c.wave_quality=1;c.velocity_amount=100;c.trigger_mode=1;return c;}
static void render(unsigned n){for(unsigned j=0;j<n;j++){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
int main(void){
 assert(sizeof(core_settings)==76);assert(sizeof(pw_env)==32);
 for(unsigned shape=0;shape<128;shape++)for(int sustain=-127;sustain<=127;sustain+=127){
  pw_env e;pw_env_init(&e);pw_env_set(&e,(pw_env_params){20,30,(int8_t)sustain,40});pw_env_shapes(&e,shape,127-shape);pw_env_trigger_mode(&e,1);pw_env_on(&e);
  unsigned i=0;while(e.stage&&i++<44100){float v=pw_env_tick(&e);assert(isfinite(v)&&v>=-1.00001f&&v<=1.00001f);assert(e.stage!=PW_ENV_SUSTAIN);}
  assert(e.stage==PW_ENV_IDLE&&e.level==0&&i<44100);
 }
 for(unsigned port=0;port<3;port++)for(unsigned unison=0;unison<3;unison++){
  for(unsigned pass=0;pass<2;pass++){
   core_init();core_settings c=settings();c.unison_mode=unison;c.env2_pitch=30;c.filter_env2_depth=20;c.filter.mode=1;core_set(&c);render(5);core_midi(port,0x93,48,100);
   for(unsigned b=0;b<160;b++){
    if(pass&&b==1){core_midi(port,0xb3,64,127);core_midi(port,0x93,48,0);assert(!core_note_held(port,3,48));assert(!core_note_sustained(port,3,48));}
    if(pass&&b==8)core_midi(port,0xb3,64,0);
    render(1);if(!pass)memcpy(reference[b],pcm,sizeof(pcm));else assert(!memcmp(reference[b],pcm,sizeof(pcm)));
   }
   assert(core_voice_count()==0);for(unsigned i=0;i<512;i++)assert(pcm[i]==0);
  }
 }
 /* Existing gated note remains gated after selection changes. */
 core_init();core_settings c=settings();c.trigger_mode=0;core_set(&c);core_midi(0,0x90,48,127);render(40);assert(core_env_stage()==PW_ENV_SUSTAIN);
 c.trigger_mode=1;core_set(&c);render(40);assert(core_env_stage()==PW_ENV_SUSTAIN);core_midi(0,0x80,48,0);render(100);assert(!core_voice_count());
 /* Existing triggered note completes after switching back; repeated trigger restarts from current level. */
 core_init();c=settings();core_set(&c);core_midi(0,0x90,48,127);render(2);float level=core_env_level();core_midi(0,0x90,48,127);assert(core_env_stage()==PW_ENV_ATTACK);assert(core_env_level()==level);
 c.trigger_mode=0;core_set(&c);render(160);assert(!core_voice_count());
 /* Forced All Notes Off and All Sound Off retain channel isolation. */
 core_init();c=settings();c.env.attack=c.env.decay=127;core_set(&c);core_midi(0,0x90,48,127);core_midi(2,0x91,60,127);render(4);core_midi(0,0xb0,123,0);assert(!core_note_held(0,0,48));assert(core_note_held(2,1,60));render(80);assert(core_note_held(2,1,60));core_midi(2,0xb1,120,0);render(1);assert(!core_voice_count());
 for(unsigned i=0;i<5000;i++){c=settings();c.trigger_mode=i%2;c.unison_mode=i%3;c.env.sustain=i%128;c.env2.sustain=(int)(i%255)-127;c.env.attack=i%128;c.filter.mode=i%4;core_set(&c);core_midi(i%3,0x90|(i%16),24+i%60,1+i%127);if(i%3==0)core_midi(i%3,0x80|(i%16),24+i%60,0);render(1);}
 core_panic();render(1);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: triggered envelopes: 384 shape/polarity sweeps, exact early-note-off/pedal PCM equivalence across all ports/unison, automatic silence while held, per-note mode capture, retrigger continuity, channel-scoped CC123/120, 5000 mixed-mode stress blocks and panic.");
}
