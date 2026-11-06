#include "pw_chord.h"
/* Recovered from 1.26.1: scales 9d0dd654, shapes 9d082d40. */
static const uint8_t scales[9][13]={
 {7,0,2,4,5,7,9,11},{7,0,2,3,5,7,8,10},{7,0,2,4,5,7,9,10},
 {8,0,2,3,5,6,8,9,11},{8,0,1,3,4,6,7,9,10},{6,0,3,4,7,8,11},
 {6,0,2,4,6,8,10},{12,0,1,2,3,4,5,6,7,8,9,10,11},{7,0,2,3,5,7,8,11}};
static const uint8_t quant[9][12]={
 {0,0,1,1,2,3,3,4,4,5,5,6},{0,0,1,2,2,3,3,4,5,5,6,6},{0,0,1,1,2,3,3,4,4,5,5,6},
 {0,0,1,2,2,3,4,4,5,6,6,7},{0,1,1,2,3,3,4,5,5,6,7,7},{0,0,0,1,2,2,2,3,4,4,4,5},
 {0,0,1,1,2,2,3,3,4,4,5,5},{0,1,2,3,4,5,6,7,8,9,10,11},{0,0,1,2,2,3,3,4,5,5,6,6}};
static const uint16_t shapes[14][6]={
 {1,0},{2,0,256},{3,0,256,512},{2,0,4},{3,0,4,256},{3,0,2,4},
 {4,0,2,4,256},{3,0,3,4},{4,0,3,4,256},{4,0,2,4,5},
 {5,0,2,4,5,256},{4,0,2,4,6},{5,0,2,4,6,256},{5,0,2,4,6,257}};
unsigned pw_chord_shape(unsigned s){static const uint8_t map[15]={0,0,1,2,3,4,5,6,7,8,9,11,10,12,13};return map[s<15?s:0];}
unsigned pw_chord_notes(unsigned note,int key,unsigned scale,unsigned shape,unsigned inversion,uint8_t out[5]){
 if(note>127||key< -12||key>12||scale>8||shape>13||inversion>3)return 0;
 int tonic=(key+60)%12,relative=(int)note-tonic;
 /* Valid FM1 notes are >=24. For the source's negative modulo read, reject
    below-tonic inputs instead of indexing before its quantization table. */
 if(relative<0)return 0;
 unsigned degree=quant[scale][relative%12],size=scales[scale][0],count=shapes[shape][0],used=0;
 int base=relative/12*12+tonic;
 for(unsigned i=0;i<count;i++){
  unsigned index=i+inversion,code=shapes[shape][1+index%count]+256*(index/count);
  unsigned d=degree+(code&255)+(code>>8)*size;
  int pitch=base+12*(d/size)+scales[scale][1+d%size];
  if(pitch>=0&&pitch<=127)out[used++]=(uint8_t)pitch;
 }
 return used;
}

unsigned pw_chord_omit_octave(unsigned shape){
 /* Bit 8 of the original descriptor at 9d082d40, separate from its low-byte count. */
 return shape==4||shape==6||shape==8||shape==10||shape==12;
}

unsigned pw_chord_fit(uint8_t pitches[5],unsigned count,unsigned budget){
 if(count>5||!budget)return 0;if(budget>4)budget=4;
 if(count<=4)return count<budget?count:budget;
 /* FM1 reduction of five-note source shapes: retain upper extensions.
    Existing <=4-note shapes retain their original prefix selection. */
 static const uint8_t members[4][4]={{0},{0,4},{0,3,4},{0,1,3,4}};
 for(unsigned i=0;i<budget;i++)pitches[i]=pitches[members[budget-1][i]];
 return budget;
}
