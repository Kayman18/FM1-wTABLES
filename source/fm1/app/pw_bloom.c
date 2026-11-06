#include "pw_bloom.h"
/* Firmware 0x24038208: exact pitch classes, including source scale 3. */
static const uint8_t scales[PW_BLOOM_SCALES][13]={
 {0,1,2,3,4,5,6,7,8,9,10,11,12},{0,2,4,5,7,9,11,0,0,0,0,0,7},
 {0,2,3,5,7,8,10,0,0,0,0,0,7},{0,2,5,7,9,0,0,0,0,0,0,0,5},
 {0,3,5,7,10,0,0,0,0,0,0,0,5},{0,2,3,5,7,8,11,0,0,0,0,0,7},
 {0,2,4,6,8,10,0,0,0,0,0,0,6},
 {0,2,3,5,7,9,10,0,0,0,0,0,7}, /* Dorian */
 {0,1,3,5,7,8,10,0,0,0,0,0,7}, /* Phrygian */
 {0,2,4,6,7,9,11,0,0,0,0,0,7}, /* Lydian */
 {0,2,4,5,7,9,10,0,0,0,0,0,7}, /* Mixolydian */
 {0,1,3,5,6,8,10,0,0,0,0,0,7}, /* Locrian */
 {0,2,4,7,9,0,0,0,0,0,0,0,5}, /* Major pentatonic */
 {0,2,3,5,7,9,11,0,0,0,0,0,7}, /* Melodic minor ascending */
 {0,3,5,6,7,10,0,0,0,0,0,0,6}, /* Minor blues */
 {0,2,3,4,7,9,0,0,0,0,0,0,6}, /* Major blues */
 {0,2,3,5,6,8,9,11,0,0,0,0,8}}; /* Whole-half diminished */
uint32_t pw_bloom_random(uint32_t *rng){uint32_t x=*rng;if(!x)x=1;x^=x<<13;x^=x>>17;x^=x<<5;return *rng=x;}
/* TEST68: table is the exact output of the original scale quantizer for
   every one of 17 scales and all 61 possible input notes. No rounding changes. */
static const uint8_t quant_table[PW_BLOOM_SCALES][61] = {
 {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60},
 {0,0,2,2,4,5,5,7,7,9,9,11,12,12,14,14,16,17,17,19,19,21,21,23,24,24,26,26,28,29,29,31,31,33,33,35,36,36,38,38,40,41,41,43,43,45,45,47,48,48,50,50,52,53,53,55,55,57,57,59,60},
 {0,0,2,3,3,5,5,7,8,8,10,10,12,12,14,15,15,17,17,19,20,20,22,22,24,24,26,27,27,29,29,31,32,32,34,34,36,36,38,39,39,41,41,43,44,44,46,46,48,48,50,51,51,53,53,55,56,56,58,58,60},
 {0,0,2,2,5,5,5,7,7,9,9,9,12,12,14,14,17,17,17,19,19,21,21,21,24,24,26,26,29,29,29,31,31,33,33,33,36,36,38,38,41,41,41,43,43,45,45,45,48,48,50,50,53,53,53,55,55,57,57,57,60},
 {0,0,3,3,3,5,5,7,7,10,10,10,12,12,15,15,15,17,17,19,19,22,22,22,24,24,27,27,27,29,29,31,31,34,34,34,36,36,39,39,39,41,41,43,43,46,46,46,48,48,51,51,51,53,53,55,55,58,58,58,60},
 {0,0,2,3,3,5,5,7,8,8,11,11,12,12,14,15,15,17,17,19,20,20,23,23,24,24,26,27,27,29,29,31,32,32,35,35,36,36,38,39,39,41,41,43,44,44,47,47,48,48,50,51,51,53,53,55,56,56,59,59,60},
 {0,0,2,2,4,4,6,6,8,8,10,10,12,12,14,14,16,16,18,18,20,20,22,22,24,24,26,26,28,28,30,30,32,32,34,34,36,36,38,38,40,40,42,42,44,44,46,46,48,48,50,50,52,52,54,54,56,56,58,58,60},
 {0,0,2,3,3,5,5,7,7,9,10,10,12,12,14,15,15,17,17,19,19,21,22,22,24,24,26,27,27,29,29,31,31,33,34,34,36,36,38,39,39,41,41,43,43,45,46,46,48,48,50,51,51,53,53,55,55,57,58,58,60},
 {0,1,1,3,3,5,5,7,8,8,10,10,12,13,13,15,15,17,17,19,20,20,22,22,24,25,25,27,27,29,29,31,32,32,34,34,36,37,37,39,39,41,41,43,44,44,46,46,48,49,49,51,51,53,53,55,56,56,58,58,60},
 {0,0,2,2,4,4,6,7,7,9,9,11,12,12,14,14,16,16,18,19,19,21,21,23,24,24,26,26,28,28,30,31,31,33,33,35,36,36,38,38,40,40,42,43,43,45,45,47,48,48,50,50,52,52,54,55,55,57,57,59,60},
 {0,0,2,2,4,5,5,7,7,9,10,10,12,12,14,14,16,17,17,19,19,21,22,22,24,24,26,26,28,29,29,31,31,33,34,34,36,36,38,38,40,41,41,43,43,45,46,46,48,48,50,50,52,53,53,55,55,57,58,58,60},
 {0,1,1,3,3,5,6,6,8,8,10,10,12,13,13,15,15,17,18,18,20,20,22,22,24,25,25,27,27,29,30,30,32,32,34,34,36,37,37,39,39,41,42,42,44,44,46,46,48,49,49,51,51,53,54,54,56,56,58,58,60},
 {0,0,2,2,4,4,7,7,7,9,9,9,12,12,14,14,16,16,19,19,19,21,21,21,24,24,26,26,28,28,31,31,31,33,33,33,36,36,38,38,40,40,43,43,43,45,45,45,48,48,50,50,52,52,55,55,55,57,57,57,60},
 {0,0,2,3,3,5,5,7,7,9,9,11,12,12,14,15,15,17,17,19,19,21,21,23,24,24,26,27,27,29,29,31,31,33,33,35,36,36,38,39,39,41,41,43,43,45,45,47,48,48,50,51,51,53,53,55,55,57,57,59,60},
 {0,0,3,3,3,5,6,7,7,10,10,10,12,12,15,15,15,17,18,19,19,22,22,22,24,24,27,27,27,29,30,31,31,34,34,34,36,36,39,39,39,41,42,43,43,46,46,46,48,48,51,51,51,53,54,55,55,58,58,58,60},
 {0,0,2,3,4,4,7,7,7,9,9,9,12,12,14,15,16,16,19,19,19,21,21,21,24,24,26,27,28,28,31,31,31,33,33,33,36,36,38,39,40,40,43,43,43,45,45,45,48,48,50,51,52,52,55,55,55,57,57,57,60},
 {0,0,2,3,3,5,6,6,8,9,9,11,12,12,14,15,15,17,18,18,20,21,21,23,24,24,26,27,27,29,30,30,32,33,33,35,36,36,38,39,39,41,42,42,44,45,45,47,48,48,50,51,51,53,54,54,56,57,57,59,60}
};
unsigned pw_bloom_quantize(unsigned note,unsigned scale){
 if(note>60)note=60;
 if(scale>=PW_BLOOM_SCALES)scale=0;
 return quant_table[scale][note];
}
unsigned pw_bloom_mutate(uint32_t *rng,unsigned scale,unsigned raw,unsigned reference){
 static const int8_t offsets[14]={1,-1,2,-2,3,-3,4,-4,5,-5,7,-7,12,-12};
 static const float weights[14]={7,7,10,10,8,8,8,8,5,15,15,7,3,3};
 if(raw>60)raw=60;if(reference>60)reference=60;
 int start=(int)pw_bloom_quantize(raw,scale),previous=(int)pw_bloom_quantize(reference,scale),diff=start-previous;
 float cumulative[14],sum=0;
 for(unsigned i=0;i<14;i++){float w=weights[i];if((diff>12&&offsets[i]<0)||(diff< -12&&offsets[i]>0)){float factor=(float)(diff<0?-diff:diff)/12.f;factor=factor>3.f?4.f:factor+1.f;w*=factor;}sum+=w;cumulative[i]=sum;}
 float unit=(float)pw_bloom_random(rng)*0x1p-32f,threshold=sum*unit;int delta=0;
 for(unsigned i=0;i<14;i++)if(cumulative[i]>=threshold){delta=offsets[i];break;}
 int target=start+delta,movement=delta;
 if(target>60||target<0){int opposite=start-delta;if(opposite>=0&&opposite<=60)movement=-delta;else if(target>60)movement=-(target-60);else movement=-target;}
 int magnitude=movement<0?-movement:movement;if(magnitude<1)magnitude=1;
 int shifted=(int)raw+(movement<0?-magnitude:magnitude);
 if(shifted<0||shifted>=72)return raw;return shifted>60?60:(unsigned)shifted;
}
void pw_bloom_restore(pw_bloom *b){b->build=b->ready=b->valid=b->node=b->depth=0;b->length=0;for(unsigned i=0;i<PW_BLOOM_STEPS;i++)b->notes[i]=b->reference[i]=b->seed[i];}
void pw_bloom_init(pw_bloom *b){*b=(pw_bloom){0};b->rng=1;for(unsigned i=0;i<PW_BLOOM_STEPS;i++)b->seed[i]=(uint8_t)((i*7)%25);pw_bloom_restore(b);}
void pw_bloom_reseed(pw_bloom *b){for(unsigned i=0;i<PW_BLOOM_STEPS;i++)b->seed[i]=(uint8_t)(((uint64_t)pw_bloom_random(&b->rng)*25u)>>32);pw_bloom_restore(b);}
unsigned pw_bloom_step(pw_bloom *b,unsigned index,unsigned scale,unsigned amount){
 index&=PW_BLOOM_STEPS-1;if(amount>100)amount=100;
 if(amount&&((uint64_t)pw_bloom_random(&b->rng)*100u>>32)<amount){unsigned old=b->notes[index];b->notes[index]=(uint8_t)pw_bloom_mutate(&b->rng,scale,old,b->reference[index]);b->reference[index]=(uint8_t)old;}
 return pw_bloom_quantize(b->notes[index],scale);
}

/* Source 0x240143d0, integer pitches; output is quantized at playback. */
/* noinline for the same reason as mul64 in test_engine.c: the 64-bit multiply
   stays clear of the loads around pw_bloom_branch (felucca-bsp docs/CPU.md). */
static __attribute__((noinline)) unsigned pick(uint32_t *rng,unsigned n){return (unsigned)((uint64_t)pw_bloom_random(rng)*n>>32);}
/* TEST68: compute only the data required by the chosen branch transform.
   RNG calls, generated nodes, and the complete 255-node tree are unchanged.
   In TEST67 the full-scale quantize/range/mean scan ran even for a simple
   reversal (choice 0) or per-note mutation (choice 3). */
void pw_bloom_branch(uint8_t *dst,const uint8_t *src,const uint8_t *ancestor,unsigned length,unsigned scale,uint32_t *rng){
 if(length<1||length>PW_BLOOM_STEPS||scale>=PW_BLOOM_SCALES)return;
 unsigned choice=pick(rng,5);
 if(choice==0){
  for(unsigned i=0;i<length;i++)dst[i]=src[length-1-i];
  for(unsigned i=length;i<PW_BLOOM_STEPS;i++)dst[i]=src[i];
  return;
 }
 if(choice==3){
  for(unsigned i=0;i<length;i++)dst[i]=(uint8_t)pw_bloom_mutate(rng,scale,src[i],ancestor?ancestor[i]:src[i]);
  for(unsigned i=length;i<PW_BLOOM_STEPS;i++)dst[i]=src[i];
  return;
 }
 unsigned q[PW_BLOOM_STEPS],lo=60,hi=0,sum=0,low=12;
 for(unsigned i=0;i<length;i++){
  q[i]=pw_bloom_quantize(src[i],scale);
  if(choice!=1){if(q[i]<lo)lo=q[i];if(q[i]>hi)hi=q[i];}
  if(choice==1){if(q[i]<low)low=q[i];}else if(choice==4)sum+=q[i];
 }
 unsigned movement=0;
 if(choice==1){
  unsigned high=scales[scale][12]*2;high=high>low?high-low:0;
  if(high)movement=pick(rng,high+1);
  for(unsigned t=0;!movement&&t<8;t++)if(high)movement=pick(rng,high+1);
  if(!movement)movement=1;
 }
 float mean=0,half=0;
 unsigned lower=0,upper=0;
 if(choice==4){
  mean=(float)sum/(float)length;half=(float)(hi-lo)*.5f;
  lower=mean>half?(unsigned)(mean-half):0;
  upper=(unsigned)(mean+half);
 }
 for(unsigned i=0;i<length;i++){
  unsigned n=src[i];
  if(choice==1){unsigned shifted=n+movement;if(shifted<72)n=shifted>60?60:shifted;}
  if(choice==2)n=pw_bloom_quantize(hi+lo-q[i],scale);
  if(choice==4)n=lower+(lower<upper?pick(rng,upper+1-lower):0);
  dst[i]=(uint8_t)n;
 }
 for(unsigned i=length;i<PW_BLOOM_STEPS;i++)dst[i]=src[i];
}
/* Build at most one node per audio block (every second block above 16 steps); publish at trunk boundary. */
void pw_bloom_service(pw_bloom *b,unsigned length,unsigned scale){
 if(length<1||length>PW_BLOOM_STEPS||scale>=PW_BLOOM_SCALES)return;
 if(b->length!=length||b->scale!=scale){
  b->length=length;b->scale=scale;b->ready=0;b->build=1;
  b->tree_rng=b->rng?b->rng:1;
  for(unsigned i=0;i<PW_BLOOM_STEPS;i++)b->tree[b->active^1][0][i]=b->notes[i];
 }
 if(!b->build)return;
 if(length>16){b->service_phase^=1;if(b->service_phase)return;}
 unsigned n=b->build,p=(n-1)/2;uint8_t (*tree)[PW_BLOOM_STEPS]=b->tree[b->active^1];
 pw_bloom_branch(tree[n],tree[p],p?tree[(p-1)/2]:0,length,scale,&b->tree_rng);
 if(++b->build==255){b->build=0;b->ready=1;}
}
void pw_bloom_boundary(pw_bloom *b,unsigned branches,unsigned path){
 if(branches>7)branches=7;if(path>127)path=127;
 if(b->depth>=b->branches||branches!=b->branches||path!=b->path||!b->valid){
  b->depth=b->node=0;b->branches=branches;b->path=path;
  if(b->ready){b->active^=1;b->ready=0;b->valid=1;}
 }else{unsigned bit=(path>>(6-b->depth))&1;b->node=b->node*2+1+bit;++b->depth;}
}
unsigned pw_bloom_tree_step(pw_bloom *b,unsigned index,unsigned scale,unsigned amount){
 if(!b->valid||!b->node)return pw_bloom_step(b,index,scale,amount);
 index&=PW_BLOOM_STEPS-1;unsigned node=b->node;uint8_t *notes=b->tree[b->active][node];
 if(amount>100)amount=100;
 if(amount&&pick(&b->rng,100)<amount)notes[index]=pw_bloom_mutate(&b->rng,scale,notes[index],b->tree[b->active][(node-1)/2][index]);
 return pw_bloom_quantize(notes[index],scale);
}

/* Edit the seed, not the currently visited branch. A new tree publishes at a trunk boundary. */
unsigned pw_bloom_edit(pw_bloom *b,unsigned index,int delta,unsigned scale){
 if(index>=PW_BLOOM_STEPS)return 0;
 unsigned note=pw_bloom_quantize(b->seed[index],scale);
 if(delta>60)delta=60;if(delta< -60)delta= -60;
 while(delta){int direction=delta>0?1:-1;int n=(int)note+direction;
  while(n>=0&&n<=60&&pw_bloom_quantize((unsigned)n,scale)!=(unsigned)n)n+=direction;
  if(n<0||n>60)break;note=(unsigned)n;delta-=direction;
 }
 b->seed[index]=b->notes[index]=b->reference[index]=(uint8_t)note;
 b->length=0;b->ready=b->build=0;
 return note;
}

/* Diatonic performance transpose; leaves the seed and branch tree untouched. */
int pw_bloom_offset(unsigned note,unsigned scale,int steps){
 if(scale>=PW_BLOOM_SCALES)scale=0;
 unsigned q=pw_bloom_quantize(note,scale),n=scales[scale][12],degree=0;
 while(degree+1<n&&scales[scale][degree]!=q%12)++degree;
 int index=(int)(q/12*n+degree)+steps;
 int octave=index/(int)n,pc=index%(int)n;
 if(pc<0){pc+=(int)n;--octave;}
 return octave*12+scales[scale][pc];
}

uint32_t pw_bloom_eighth_interval(unsigned bpm,uint32_t *remainder){
 if(bpm<40)bpm=40;if(bpm>240)bpm=240;
 unsigned divisor=bpm*2;uint32_t sum=2646000u+*remainder;
 *remainder=sum%divisor;return sum/divisor;
}
