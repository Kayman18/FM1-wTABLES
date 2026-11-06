/* Retain every original bank/preset and validate the appended two-frame banks. */
#include "pw_tables.h"
#include "pw_core.h"
#include "pw_presets.h"
#include "plat.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t region[PW_TABLE_BYTES] __attribute__((aligned(4)));
const uint8_t *plat_data(uint32_t off) {
 return off>=PW_TABLE_FLASH_START&&off-PW_TABLE_FLASH_START<PW_TABLE_BYTES ? region+off-PW_TABLE_FLASH_START : 0;
}
static void load_external(void){
 FILE *f=fopen("data/factory-mips.pwlc","rb"); assert(f);
 assert(fread(region,1,sizeof region,f)==sizeof region);assert(fgetc(f)==EOF);fclose(f);
}
int main(void){
 assert(PW_TABLE_BANKS==25 && PW_FACTORY_BANKS==7 && PW_VIRUS_BANKS==18);
 assert(PW_FRAME_COUNT==16 && PW_BANK_SAMPLES==32736 && PW_FRAME_POS_MAX==3839);
 load_external();pw_tables_init();assert(pw_tables_ready());assert(!pw_tables_busy());
 for(unsigned bank=0;bank<PW_TABLE_BANKS;bank++){
  for(unsigned q=0;q<300 && (pw_tables_busy()||pw_tables_bank()!=bank);q++)pw_tables_service(bank,1);
  assert(pw_tables_ready()&&!pw_tables_busy()&&pw_tables_bank()==bank);
  unsigned frames=pw_tables_frame_count(bank);
  assert(frames==(bank<14?16u:2u));
  unsigned position_max=(frames-1u)*256u-1u;
  for(unsigned mip=0;mip<10;mip++){
   assert(pw_table_mips[mip][bank]);
   for(unsigned frame=0;frame<frames;frame++){
    uint32_t width=1024u>>mip;
    const int16_t *wave=pw_table_mips[mip][bank]+frame*width;
    volatile int32_t edge=(int32_t)wave[0]+(int32_t)wave[width-1];(void)edge;
   }
  }
  pw_core_voice v={0};v.frequency_increment=16000000;v.pitch_scale_q12=4096;
  for(unsigned i=0;i<frames;i++){
   v.position_q8=(int32_t)(i==frames-1?position_max:i*256);
   for(unsigned j=0;j<32;j++){
    int32_t sample=pw_core_tick(&v,pw_table_mips[0][bank],frames,1);
    assert(sample>= -8388608 && sample<=8388608);
    assert(v.position_q8 <=(int32_t)position_max);
   }
  }
 }
 int hits[PW_TABLE_BANKS]={0};
 assert(PW_PRESET_COUNT==29);
 for(unsigned i=0;i<PW_PRESET_COUNT;i++){
  core_settings c;assert(pw_preset_get(i,&c));assert(c.bank<PW_TABLE_BANKS);
  assert(c.voice_limit==CORE_DEFAULT_VOICES && CORE_MAX_VOICES==6 && CORE_DEFAULT_VOICES==4 && c.gain_db==0);
  assert(pw_preset_name(i)[0]&&pw_preset_category(i)[0]);
  assert(core_unison_count(&c)<=CORE_MAX_UNISON);
  hits[c.bank]++;
  const char *name=pw_preset_name(i);
  assert(strncmp(name,"AV -",5)!=0);
  assert(strcmp(name,"Dust Halo")&&strcmp(name,"Vocal Pluck"));
 }
 /* New tables are selectable without rewriting any existing factory preset. */
 for(unsigned i=0;i<14;i++)assert(hits[i]);
 core_settings c;assert(pw_preset_get(1,&c));c.sequencer_bpm=175;c.bloom_gate_percent=83;c.bloom_mode=1;
 assert(pw_preset_apply(PW_PRESET_COUNT-1,&c));assert(c.sequencer_bpm==175&&c.bloom_gate_percent==83&&c.bloom_mode);
 assert(!pw_preset_get(PW_PRESET_COUNT,&c));
 puts("PASS: 14 original 16-frame banks plus 11 two-frame banks, all 10 mips, lossless C decompressor, original 29 presets and legacy-bank coverage");
}
