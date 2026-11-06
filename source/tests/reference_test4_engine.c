#include "test_engine.h"
#include "pw_core.h"
#include "akwf_tables.h"
#define VOICES 4u
#define STEAL_FADE 128u
#define PANEL_PORT 254u
#define AUDITION_PORT 255u
static volatile core_settings st;
static volatile uint32_t panel_keys;
static volatile uint8_t panic_requested;
static uint32_t processed_keys,serial;
static int32_t gain,position;
static uint8_t active_bank,audition_on,audition_note;
typedef struct {
 pw_core_voice osc;
 pw_env env;
 uint32_t age;
 uint8_t port,channel,note,velocity,held;
} poly_voice;
typedef struct {poly_voice main,tail;uint32_t fade;float tail_weight;} voice_slot;
static voice_slot slots[VOICES];
static const uint32_t INC[60]={3185015,3374406,3575058,3787642,4012867,4251485,4504291,4772130,5055896,5356535,5675051,6012507,6370030,6748811,7150117,7575285,8025735,8502970,9008582,9544261,10111792,10713070,11350103,12025015,12740059,13497623,14300233,15150569,16051469,17005939,18017165,19088521,20223584,21426141,22700205,24050030,25480119,26995246,28600467,30301139,32102938,34011878,36034330,38177043,40447168,42852281,45400411,48100060,50960238,53990491,57200933,60602278,64205876,68023757,72068660,76354085,80894335,85704563,90800821,96200119};

static int live(const poly_voice *v){return v->env.stage!=PW_ENV_IDLE;}
static void defaults(poly_voice *v){*v=(poly_voice){0};pw_env_init(&v->env);v->osc.pitch_scale_q12=4096;}
void core_init(void){
 st=(core_settings){0,48,0,0,75,{50,60,80,60}};
 panel_keys=processed_keys=serial=panic_requested=0;gain=position=active_bank=audition_on=0;audition_note=48;
 for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
}
void core_set(const core_settings *s){core_settings c=*s;if(c.note<24)c.note=24;if(c.note>83)c.note=83;if(c.position>100)c.position=100;if(c.bank>7)c.bank=7;if(c.level>100)c.level=100;st=c;}
void core_keys(uint32_t keys){panel_keys=keys&0x7ffffffu;}
static int matches(const poly_voice *v,unsigned port,unsigned channel,unsigned note){return live(v)&&v->port==port&&v->channel==channel&&v->note==note;}
static void note_off(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(matches(v,port,channel,note)&&v->held){v->held=0;pw_env_off(&v->env);}
 }
}
static void note_on(unsigned port,unsigned channel,unsigned note,unsigned velocity){
 if(note<24||note>83||!velocity)return;
 unsigned chosen=VOICES;
 /* Repeated note-on retriggers its own slot, preserving phase and level. */
 for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note)){chosen=i;break;}
 if(chosen==VOICES){
  for(unsigned i=0;i<VOICES;i++)if(!live(&slots[i].main)&&!slots[i].fade){chosen=i;break;}
  if(chosen==VOICES){
   /* Prefer the quietest released voice, then the oldest held voice. */
   float quiet=2.0f;
   for(unsigned i=0;i<VOICES;i++)if(!slots[i].main.held&&slots[i].main.env.level<quiet){chosen=i;quiet=slots[i].main.env.level;}
   if(chosen==VOICES){chosen=0;for(unsigned i=1;i<VOICES;i++)if((uint32_t)(serial-slots[i].main.age)>(uint32_t)(serial-slots[chosen].main.age))chosen=i;}
  }
  voice_slot *slot=&slots[chosen];
  if(live(&slot->main)&&(!slot->fade||slot->main.env.level>0.0f)){
   /* Preserve an interrupted crossfade's approximate current amplitude. */
   slot->tail_weight=1.0f-(float)slot->fade/(float)STEAL_FADE;
   slot->tail=slot->main;slot->fade=STEAL_FADE;
  }
  defaults(&slot->main);
 }
 poly_voice *v=&slots[chosen].main;
 v->port=(uint8_t)port;v->channel=(uint8_t)channel;v->note=(uint8_t)note;v->velocity=(uint8_t)velocity;v->held=1;v->age=++serial;
 v->osc.frequency_increment=(int32_t)INC[note-24];v->osc.position_q8=position;
 pw_env_set(&v->env,st.env);pw_env_on(&v->env);
}
void core_midi(unsigned port,uint8_t status,uint8_t note,uint8_t velocity){
 if(port>2)return;unsigned channel=status&15,command=status&240;
 if(command==0x90&&(velocity&127))note_on(port,channel,note&127,velocity&127);
 else if(command==0x80||command==0x90)note_off(port,channel,note&127);
 else if(command==0xb0&&(note==120||note==123)){
  for(unsigned i=0;i<VOICES;i++){
   voice_slot *s=&slots[i];
   if(s->main.port==port&&s->main.channel==channel){
    s->main.held=0;
    if(note==120)defaults(&s->main);else pw_env_off(&s->main.env);
   }
   if(note==120&&s->tail.port==port&&s->tail.channel==channel){s->fade=0;defaults(&s->tail);}
  }
 }
}
void core_panic(void){st.playing=0;panic_requested=1;}
static const poly_voice *latest(unsigned held_only){
 const poly_voice *v=0;
 for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)&&(!held_only||slots[i].main.held))
  if(!v||(uint32_t)(serial-slots[i].main.age)<(uint32_t)(serial-v->age))v=&slots[i].main;
 return v;
}
unsigned core_active_note(void){const poly_voice *v=latest(1);return v?v->note:0;}
unsigned core_env_stage(void){const poly_voice *v=latest(0);return v?v->env.stage:0;}
float core_env_level(void){float level=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].main.env.level>level)level=slots[i].main.env.level;return level;}
unsigned core_voice_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)||slots[i].fade)n++;return n;}
unsigned core_held_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].main.held&&live(&slots[i].main))n++;return n;}
unsigned core_note_held(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(slots[i].main.held&&matches(&slots[i].main,port,channel,note))return 1;return 0;}
static float voice_sample(poly_voice *v){
 if(!live(v))return 0.0f;
 v->osc.position_q8=position;
 int32_t sample=pw_core_tick(&v->osc,&drone_wt[active_bank][0][0],8,3)/256;
 return (float)sample*(pw_env_tick(&v->env)*(float)v->velocity/127.0f);
}
void core_render(int32_t *lr,uint32_t n,uint32_t master){
 if(panic_requested){
  for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
  gain=0;processed_keys=panel_keys;audition_on=0;panic_requested=0;
 }
 uint32_t keys=panel_keys,changed=keys^processed_keys;
 for(unsigned k=0;k<27;k++)if((changed&(1u<<k))&&!(keys&(1u<<k)))note_off(PANEL_PORT,0,53+k);
 for(unsigned k=0;k<27;k++)if((changed&keys)&(1u<<k))note_on(PANEL_PORT,0,53+k,127);
 processed_keys=keys;
 const core_settings settings=st;
 if(audition_on&&(!settings.playing||settings.note!=audition_note))note_off(AUDITION_PORT,0,audition_note);
 if(settings.playing&&(!audition_on||settings.note!=audition_note))note_on(AUDITION_PORT,0,settings.note,127);
 audition_on=settings.playing;audition_note=settings.note;
 for(unsigned i=0;i<VOICES;i++)pw_env_set(&slots[i].main.env,settings.env);
 if(master>4096)master=4096;
 const int32_t target_gain=settings.bank==active_bank?(int32_t)(master*(uint32_t)settings.level/100u):0;
 const int32_t target_pos=(int32_t)settings.position*1791/100;
 for(uint32_t j=0;j<n;j++){
  if(position<target_pos)position++;else if(position>target_pos)position--;
  int32_t delta=target_gain-gain,step=delta/256;if(!step&&delta)step=delta>0?1:-1;gain+=step;
  if(!gain)active_bank=settings.bank;
  float mix=0.0f;
  for(unsigned i=0;i<VOICES;i++){
   voice_slot *s=&slots[i];float sample=voice_sample(&s->main);
   if(s->fade){float f=(float)s->fade/(float)STEAL_FADE;sample=sample*(1.0f-f)+voice_sample(&s->tail)*(f*s->tail_weight);s->fade--;}
   mix+=sample;
  }
  /* Four-voice fixed headroom: no gain pumping as release tails finish.
   * Slot crossfades are convex, keeping their summed peak bounded. */
  int32_t out=(int32_t)(mix*((float)gain*(256.0f/(4096.0f*4.0f))));
  lr[2*j]=lr[2*j+1]=out;
 }
}

int32_t core_wave_sample(uint32_t index){
    if(index>255)index=255;
    int32_t pos=(int32_t)st.position*1791/100;
    uint32_t frame=(uint32_t)pos>>8;int32_t mix=pos&255;
    int32_t a=drone_wt[st.bank][frame][index],b=drone_wt[st.bank][frame+1][index];
    return a+(((b-a)*mix)>>8);
}
