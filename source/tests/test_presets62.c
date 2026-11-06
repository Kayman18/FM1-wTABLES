#include "../fm1/app/pw_presets.h"
#include "../fm1/app/pw_table_layout.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 const char *removed[]={"Organo FM1","Vocalish FM1","Muted Prism","AV - Glass Pluck","AV - Drive Bass","AV - Bell Pluck",
 "80'sStab FM1","DX9 FM1","SpookBass FM1","Reed Choir","Vowel Pad","Pulse Keys","Dust Bells","Copper Bass","Crystal Steps","Breath Choir","Frozen Dust","Organ Bloom"};
 assert(PW_PRESET_COUNT==34);
 for(unsigned i=0;i<PW_PRESET_COUNT;i++){
  const char *name=pw_preset_name(i);assert(name&&*name);
  for(unsigned j=0;j<sizeof(removed)/sizeof(removed[0]);j++)assert(strcmp(removed[j],name));
  core_settings c;assert(pw_preset_get(i,&c));assert(c.bank<PW_TABLE_BANKS);
  assert(c.voice_limit<=CORE_MAX_VOICES);assert(pw_preset_category(i)[0]);
  if(i>=25)assert(c.bank>=16&&c.bank<=27);
 }
 assert(!strcmp(pw_preset_name(0),"INIT"));
 assert(!strcmp(pw_preset_name(33),"AV - Bright Pluck"));
 assert(!pw_preset_get(PW_PRESET_COUNT,0));
 /* Transport and Bloom tempo/gate survive switching presets. */
 core_settings c;assert(pw_preset_get(1,&c));c.sequencer_bpm=175;c.bloom_gate_percent=83;c.bloom_mode=1;
 assert(pw_preset_apply(26,&c));assert(c.sequencer_bpm==175&&c.bloom_gate_percent==83&&c.bloom_mode);
 puts("PASS TEST62: 34 presets, six newly removed, 18 total retired; preserved sequencer controls");
}
