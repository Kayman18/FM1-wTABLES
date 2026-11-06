#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
static int32_t pcm[512];
static core_settings cfg(void){core_settings c={0};c.note=48;c.level=60;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.wave_quality=1;c.filter.cutoff=96;c.delay_time_ms=500;c.sustain_hold=1;return c;}
static void render(unsigned blocks){while(blocks--){core_render(pcm,256,4096);assert(core_primary_count()<=voice_budget());for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
static poly_voice *root(unsigned p,unsigned ch,unsigned n){for(unsigned i=0;i<4;i++)if(matches(&slots[i].main,p,ch,n))return &slots[i].main;return 0;}
int main(void){
 assert(sizeof(core_settings)==76);
 for(unsigned port=0;port<3;port++)for(unsigned ch=0;ch<16;ch++)for(unsigned mode=0;mode<3;mode++){
  core_init();core_settings c=cfg();c.sustain_mode=mode;core_set(&c);
  core_midi(port,0x90|ch,48,100);render(8);core_midi(port,0x80|ch,48,0);render(30);
  poly_voice *v=root(port,ch,48);assert(v&&v->sustained&&!v->held&&v->env.stage==PW_ENV_SUSTAIN);
  uint32_t phase=v->osc.phase;float level=v->env.level;
  core_midi(port,0x90|ch,48,80);v=root(port,ch,48);assert(v&&v->held&&!v->sustained);
  if(mode==1){assert(v->osc.phase==phase&&v->env.level==level&&v->velocity==100);} // Synth retains strike
  if(mode==2){assert(v->env.level==0&&v->velocity==80);} // Piano fresh attack
  render(8);core_midi(port,0x80|ch,48,0);c.sustain_hold=0;core_set(&c);render(50);assert(!root(port,ch,48));
 }
 // Hold and pedal are independent OR inputs; releases are scoped.
 core_init();core_settings c=cfg();core_set(&c);
 core_midi(0,0xb0,64,127);core_midi(0,0x90,48,100);core_midi(1,0x91,55,100);render(8);
 core_midi(0,0x80,48,0);core_midi(1,0x81,55,0);core_midi(0,0xb0,64,0);render(8);assert(root(0,0,48)&&root(1,1,55));
 core_midi(0,0xb0,64,127);c.sustain_hold=0;core_set(&c);render(50);assert(root(0,0,48)&&!root(1,1,55));core_midi(0,0xb0,64,0);render(50);assert(!core_primary_count());
 // Hold-off does not release physical keys, and enabling Hold cannot resurrect a release.
 core_init();c=cfg();core_set(&c);core_midi(0,0x90,48,100);render(8);c.sustain_hold=0;core_set(&c);render(8);assert(root(0,0,48)->held);core_midi(0,0x80,48,0);render(1);c.sustain_hold=1;core_set(&c);render(50);assert(!core_primary_count());
 // Panel keys participate; audition PLAY keeps its stop behavior.
 core_init();c=cfg();core_set(&c);core_keys(1);render(8);core_keys(0);render(30);assert(root(PANEL_PORT,0,53)->sustained);c.sustain_hold=0;core_set(&c);render(50);assert(!core_primary_count());
 core_init();c=cfg();c.playing=1;core_set(&c);render(8);c.playing=0;core_set(&c);render(50);assert(!core_primary_count());
 // Triggered one-shots ignore Hold. Pending strum members still cancel on key-up.
 core_init();c=cfg();c.trigger_mode=1;core_set(&c);core_midi(0,0x90,48,100);core_midi(0,0x80,48,0);render(50);assert(!core_primary_count());
 core_init();c=cfg();c.chord_shape=7;c.break_time_ms=200;core_set(&c);core_midi(0,0x90,48,100);render(8);assert(core_pending_count()==3);core_midi(0,0x80,48,0);render(150);assert(!core_pending_count()&&core_primary_count()==1&&root(0,0,48)->sustained);c.sustain_hold=0;core_set(&c);render(50);assert(!core_primary_count());
 // All-notes-off respects sustain like pedal; All-sound-off clears only its channel.
 core_init();c=cfg();core_set(&c);core_midi(0,0x90,48,100);core_midi(1,0x90,55,100);render(8);core_midi(0,0xb0,123,0);assert(root(0,0,48)->sustained);core_midi(0,0xb0,121,0);render(8);assert(root(0,0,48)->sustained);core_midi(0,0xb0,120,0);assert(!root(0,0,48)&&root(1,0,55));
 for(unsigned i=0;i<4000;i++){
  c=cfg();c.sustain_hold=i%2;c.sustain_mode=i%3;c.voice_limit=1+i%4;c.unison_mode=i%3;c.chord_shape=i%12;c.break_time_ms=i%1001;c.trigger_mode=(i/3)%2;core_set(&c);
  unsigned p=i%3,ch=i%16,n=24+i%60;core_midi(p,0xb0|ch,64,i%4?127:0);core_midi(p,0x90|ch,n,100);render(1);core_midi(p,0x80|ch,n,0);core_keys(i%7?0:1u<<(i%27));if(i%13==0)core_midi(p,0xb0|ch,120,0);
 }
 core_panic();render(1);assert(!core_primary_count());render(100);assert(!core_primary_count());
 puts("PASS: Hold: 144 port/channel/retrigger scenarios, pedal OR and isolation, physical keys, no resurrection, panel latch, PLAY stop, Triggered exclusion, pending strum cancellation, CC120/121/123 and 4000 sanitized stress blocks.");
}
