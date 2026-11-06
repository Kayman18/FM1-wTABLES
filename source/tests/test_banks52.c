#include "pw_tables.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
static void check(unsigned bank,FILE*ref){
 unsigned calls=0;do{pw_tables_service(bank,1);assert(pw_tables_ready());assert(++calls<40);}while(pw_tables_busy()||pw_tables_bank()!=bank);
 unsigned local=bank<PW_FACTORY_BANKS?bank:bank-PW_FACTORY_BANKS;
 assert(!fseek(ref,(long)local*32736,SEEK_SET));
 int16_t expected[8*1024];
 for(unsigned m=0;m<10;m++){
  unsigned count=8*(1024u>>m);assert(fread(expected,2,count,ref)==count);assert(!memcmp(expected,pw_table_mips[m][bank],count*2));
 }
}
int main(void){
 FILE*old=fopen("data/reference49.raw","rb"),*virus=fopen("data/virus-reference.raw","rb");assert(old&&virus);pw_tables_init();assert(pw_tables_ready());
 for(unsigned i=0;i<PW_TABLE_BANKS;i++)check(i,i<PW_FACTORY_BANKS?old:virus);
 for(unsigned i=0;i<100;i++){
  unsigned first=i%PW_TABLE_BANKS,second=(i*7+13)%PW_TABLE_BANKS;
  pw_tables_service(first,1);check(second,second<PW_FACTORY_BANKS?old:virus);
 }
 fclose(old);fclose(virus);printf("PASS: all %u banks, all 10 mip levels, exact decoded samples and 100 interrupted cross-container loads.\n",PW_TABLE_BANKS);
}
