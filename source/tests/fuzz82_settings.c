/* Randomized settings/notes/controller fuzz over the wavetable engine.
   Exercises the branch combinations in the per-sample voice path that the
   factory presets alone do not reach. */
#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static uint32_t rs;
static unsigned rnd(void){rs=rs*1664525u+1013904223u;return rs>>8;}
static int8_t sgn(unsigned range){return (int8_t)((int)(rnd()%(2u*range+1u))-(int)range);}
int main(int argc,char**argv){unsigned seeds=atoi(argv[1]);int32_t pcm[512];core_settings s;
 uint64_t all=1469598103934665603ull;
 for(unsigned sd=0;sd<seeds;sd++){rs=sd*2654435761u+12345;uint64_t h=1469598103934665603ull;
  core_init();core_parallel_set(rnd()&1);
  pw_preset_default(&s);pw_preset_get(rnd()%PW_PRESET_COUNT,&s);
  s.playing=0;s.bloom_mode=0;
  s.voice_limit=1+rnd()%8;s.unison_count=rnd()%4;s.unison_mode=rnd()%3;
  s.unison_detune=rnd()%101;s.unison_wave_spread=sgn(100);
  s.wave_quality=rnd()&1;s.filter.mode=rnd()%4;s.filter.cutoff=rnd()%128;
  s.filter.resonance=rnd()%101;s.filter.mix=rnd()%101;
  s.filter_env_depth=sgn(127);s.filter_env2_depth=sgn(127);
  s.wave_env_depth=sgn(100);s.wave_env2_depth=sgn(100);
  s.lfo.rate=sgn(100);s.lfo.wave=sgn(100);s.lfo.cutoff=sgn(100);s.lfo.pitch=(uint8_t)(rnd()%128);s.lfo_phase_mode=rnd()%4;s.lfo_spread=sgn(100);
  s.lfo_pitch_cents=(int16_t)sgn(120);s.env2_pitch=sgn(120);
  s.tracking.velocity_wave=sgn(100);s.tracking.velocity_cutoff=sgn(100);
  s.tracking.pitch_wave=sgn(100);s.tracking.pitch_cutoff=sgn(100);
  s.velocity_mode=rnd()&1;s.velocity_amount=rnd()%101;s.velocity_curve=rnd()%4;
  s.trigger_mode=rnd()%3;s.sustain_mode=rnd()%3;
  s.env=(pw_env_params){(uint8_t)(rnd()%128),(uint8_t)(rnd()%128),(int8_t)(rnd()%128),(uint8_t)(rnd()%128)};
  s.env2=(pw_env_params){(uint8_t)(rnd()%128),(uint8_t)(rnd()%128),(int8_t)(rnd()%128),(uint8_t)(rnd()%128)};
  s.amp_attack_shape=sgn(63);s.amp_decay_shape=sgn(63);
  s.env2_attack_shape=sgn(63);s.env2_decay_shape=sgn(63);
  s.coarse_tune=sgn(60);s.fine_tune=sgn(60);s.bend_range_offset=(int8_t)(rnd()%48)-2;
  s.position=rnd()%4096;s.bank=rnd()%4;
  for(unsigned r=0;r<4;r++){s.matrix_source[r]=(uint8_t)(rnd()%12);s.matrix_dest[r]=(uint8_t)(rnd()%7);s.matrix_amount[r]=sgn(100);}
  core_set(&s);
  unsigned held[8],nh=0;
  for(unsigned blk=0;blk<40;blk++){
   if(rnd()%3==0){unsigned n=24+rnd()%72;core_midi(0,0x90,(uint8_t)n,(uint8_t)(1+rnd()%127));if(nh<8)held[nh++]=n;}
   if(rnd()%5==0&&nh){unsigned k=rnd()%nh;core_midi(0,0x80,(uint8_t)held[k],0);held[k]=held[--nh];}
   if(rnd()%7==0)core_midi(0,0xB0,(rnd()&1)?74:71,(uint8_t)(rnd()%128));
   if(rnd()%9==0)core_midi(0,0xB0,64,(rnd()&1)?127:0);
   if(rnd()%6==0)core_midi(0,0xE0,(uint8_t)(rnd()%128),(uint8_t)(rnd()%128));
   if(rnd()%11==0){s.position=rnd()%4096;s.filter.cutoff=rnd()%128;core_set(&s);}
   core_render(pcm,256,4096);
   for(unsigned i=0;i<512;i++){uint32_t x=(uint32_t)pcm[i];h^=x;h*=1099511628211ull;}
  }
  if(core_worker_fault())printf("worker fault seed %u\n",sd);
  all^=h;all*=1099511628211ull;}
 printf("WTFUZZ %016llx\n",(unsigned long long)all);return 0;}
