#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 const char *ban[]={"80'sStab FM1","DX9 FM1","SpookBass FM1","Reed Choir","Vowel Pad","Pulse Keys","Dust Bells","Copper Bass","Crystal Steps","Breath Choir","Frozen Dust","Organ Bloom"};
 assert(PW_PRESET_COUNT==40);
 for(unsigned i=0;i<PW_PRESET_COUNT;i++){
  const char *n=pw_preset_name(i);assert(n&&n[0]);
  for(unsigned j=0;j<12;j++)assert(strcmp(n,ban[j])!=0);
  core_settings c;assert(pw_preset_get(i,&c));assert(c.voice_limit<=CORE_MAX_VOICES);
  assert(pw_preset_category(i)[0]);
  if(i>=28){assert(c.bank>=16&&c.bank<=27);}
 }
 assert(!pw_preset_get(PW_PRESET_COUNT,0));
 assert(!strcmp(pw_preset_name(39),"AV - Bright Pluck"));
 puts("PASS TEST61: 40 compacted presets; 12 names removed and all AV presets retained");
}
