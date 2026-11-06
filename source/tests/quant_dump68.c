#include <stdio.h>
#include "pw_bloom.h"
int main(void){printf("static const uint8_t quant_table[PW_BLOOM_SCALES][61] = {\n");for(unsigned scale=0;scale<PW_BLOOM_SCALES;scale++){printf(" {");for(unsigned note=0;note<=60;note++)printf("%u%s",pw_bloom_quantize(note,scale),note==60?"":",");printf("}%s\n",scale+1==PW_BLOOM_SCALES?"":",");}printf("};\n");}
