#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
static int32_t pcm[512];
static core_settings config(void){core_settings c={0};c.note=48;c.level=10;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.filter.cutoff=96;c.wave_quality=1;c.delay_time_ms=50;c.delay_return_db=-3;c.delay_feedback=50;c.delay_mix=35;return c;}
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;for(unsigned i=0;i<m*2;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
int main(void){
 assert(sizeof(core_settings)==96);
 // Independent integer-delay recurrence: first echo, recursive decays and ping-pong lanes.
 unsigned cases=0;float mem[32][2],ref[32][2];pw_delay d;
 for(unsigned mode=1;mode<=2;mode++)for(unsigned fb=0;fb<3;fb++)for(int db=-40;db<=0;db++){
  pw_delay_init(&d,mem,32);pw_delay_reset(&d,7);memset(ref,0,sizeof ref);float feedback=fb==0?0:fb==1?.5f:.9f,level=pw_gain_db(db);
  for(unsigned t=0;t<400;t++){
   float in[2]={t==0?1.0f:0,t==3?-.5f:0},out[2],expected[2];unsigned rd=(t+32-7)%32;
   for(unsigned k=0;k<2;k++){float h=ref[rd][k^(mode==2)];expected[k]=in[k]+h*level;float wr=in[k]+h*feedback;if(fabsf(wr)<1e-6f)wr=0;ref[t%32][k]=wr;}
   pw_delay_tick_source(&d,in,out,mode,7,feedback,level);for(unsigned k=0;k<2;k++)assert(out[k]==expected[k]);
  }cases++;
 }
 // Source fractional interpolation, longest supported tap, wrap and stale-memory reset.
 static float longmem[PW_DELAY_FRAMES][2];
 pw_delay_init(&d,longmem,PW_DELAY_FRAMES);pw_delay_reset(&d,10.5f);
 for(unsigned t=0;t<60;t++){float in2[2]={t==0?1:0,0},out2[2];pw_delay_tick_source(&d,in2,out2,1,10.5f,0,1);assert(out2[0]==(t==0?1:t==10||t==11?.5f:0)&&!out2[1]);}
 pw_delay_reset(&d,24256);
 for(unsigned t=0;t<50000;t++){float in2[2]={t==0?1:0,0},out2[2];pw_delay_tick_source(&d,in2,out2,2,24256,0,1);assert(out2[0]==(t==0?1:0)&&out2[1]==(t==24256?1:0));}
 pw_delay_reset(&d,1);
 for(unsigned t=0;t<30000;t++){float in2[2]={0,0},out2[2];pw_delay_tick_source(&d,in2,out2,2,1,.9f,1);assert(!out2[0]&&!out2[1]);}
 // Invalid modes and short buffers bypass without writes.
 pw_delay_init(&d,mem,3);float in[2]={.3f,-.7f},out[2];pw_delay_tick_source(&d,in,out,1,1,.5f,1);assert(!memcmp(in,out,sizeof in));
 // Full engine: gain endpoints settle exactly, mute stops new excitation, FX tails remain.
 core_init();core_settings c=config();c.playing=1;core_set(&c);frames(6000);assert(synth_gain==1);
 for(int db=-40;db<=24;db++){c.gain_db=db;core_set(&c);frames(6000);assert(synth_gain==pw_gain_db(db));}
 c.gain_db=0;core_set(&c);frames(6000);assert(synth_gain==1);
 c.delay_mode=2;c.delay_style=1;core_set(&c);frames(12000);assert(delay_style_active==1&&delay_mix==pw_gain_db(-3)&&delay.valid);
 c.delay_return_db=-40;core_set(&c);frames(300);assert(delay_mix==0&&delay.valid); // return mute does not erase buffer
 c.delay_return_db=0;c.delay_style=0;core_set(&c);frames(300);assert(!delay_style_active&&delay_mix==.35f);
 c.delay_style=1;c.delay_return_db=-3;core_set(&c);frames(600);assert(delay_style_active==1&&delay_mix==pw_gain_db(-3));
 c.delay_mode=0;core_set(&c);frames(600);assert(!delay_active&&!delay_mix&&!delay.valid);
 c.gain_db=-40;core_set(&c);frames(7000);assert(!synth_gain);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 // Sustained source at maximum feedback remains finite; repeated time/style/mode changes and panic.
 c.gain_db=24;c.delay_feedback=90;c.delay_mode=2;c.delay_style=1;c.delay_return_db=0;core_set(&c);frames(44100*3);
 for(unsigned i=0;i<5000;i++){
  c.gain_db=(int)(i%65)-40;c.delay_return_db=-(int)(i%41);c.delay_style=(i/9)%2;c.delay_mode=(i/13)%3;c.delay_time_ms=1+i%550;c.delay_feedback=i%91;c.delay_mix=i%101;c.chorus_mode=(i/17)%3;c.chorus_mix=i%101;core_set(&c);frames(256);
  assert(isfinite(synth_gain)&&isfinite(delay_mix));for(unsigned j=0;j<delay.length;j+=199)assert(isfinite(delay.ring[j][0])&&isfinite(delay.ring[j][1]));
 }
 c.delay_mode=2;c.delay_style=1;c.delay_return_db=0;c.gain_db=0;core_set(&c);frames(12000);assert(delay.valid);
 c.playing=0;core_set(&c);core_panic();frames(1);assert(delay.valid<=1);for(unsigned i=0;i<2;i++)assert(!pcm[i]);frames(44100);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: TEST36 gain/delay: 246 independent stereo/ping-pong impulse recurrences, fractional/max taps, wrap/reset; all gain endpoints, mute, return-only mute, smooth mode/style changes, 90% feedback, panic and 5000 sanitized stress blocks.");
}
