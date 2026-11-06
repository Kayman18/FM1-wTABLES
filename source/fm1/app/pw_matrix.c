#include "pw_matrix.h"
static float q_table[101];
void pw_matrix_init(pw_matrix*m){*m=(pw_matrix){0};for(unsigned i=0;i<3;i++)pw_sloth_init(&m->sloth[i],i);for(unsigned i=0;i<101;i++)q_table[i]=pw_filter_q(i);}
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
float pw_matrix_resonance(float x){if(x<=0)return q_table[0];if(x>=100)return q_table[100];unsigned i=(unsigned)x;return q_table[i]+(q_table[i+1]-q_table[i])*(x-(float)i);}
static void update(pw_matrix*m,const core_settings*c){
 for(unsigned i=0;i<3;i++){pw_sloth_update(&m->sloth[i],200.0f);m->volts[1+i*3]=m->sloth[i].x;m->volts[2+i*3]=m->sloth[i].y;m->volts[3+i*3]=m->sloth[i].z;}
 float combined=.7f*(m->sloth[0].z-(m->sloth[1].z+m->sloth[2].z));m->volts[10]=combined<0?combined:0;m->volts[11]=combined>0?combined:0;
 for(unsigned d=0;d<6;d++)m->target[d]=0;
 for(unsigned r=0;r<4;r++){unsigned source=c->matrix_source[r],dest=c->matrix_dest[r];if(source&&source<12&&dest&&dest<7){float unit=clamp(m->volts[source]*.1f,-1,1),amount=(float)c->matrix_amount[r]*.01f;m->target[dest-1]+=unit*amount*(dest==1?64.0f:dest==2?50.0f:100.0f);}}
 m->remainder+=44100;m->left=m->remainder/200;m->remainder%=200;
 for(unsigned d=0;d<6;d++)m->step[d]=(m->target[d]-m->value[d])/(float)m->left;
}
void pw_matrix_tick(pw_matrix*m,const core_settings*c){if(!m->left)update(m,c);for(unsigned d=0;d<6;d++)m->value[d]+=m->step[d];if(!--m->left){for(unsigned d=0;d<6;d++)m->value[d]=m->target[d];}}
void pw_matrix_advance(pw_matrix*m,const core_settings*c,unsigned frames){while(frames){if(!m->left)update(m,c);unsigned n=frames<m->left?frames:m->left;m->left-=n;frames-=n;/* Called only with silent routing; preserve generator time without audio-rate work. */if(!m->left){for(unsigned d=0;d<6;d++)m->value[d]=m->target[d];}}}
unsigned pw_matrix_wave_depth_assigned(const core_settings *c){for(unsigned r=0;r<4;r++)if(c->matrix_source[r]&&c->matrix_dest[r]==6)return 1;return 0;}
unsigned pw_matrix_active(const pw_matrix*m,const core_settings*c){for(unsigned r=0;r<4;r++)if(c->matrix_dest[r]&&c->matrix_source[r]&&(c->matrix_amount[r]||c->matrix_dest[r]==6))return 1;for(unsigned d=0;d<6;d++)if(m->value[d]!=0||m->target[d]!=0)return 1;return 0;}
const char *pw_matrix_source_name(unsigned s){static const char *n[]={"OFF","TORPOR X","TORPOR Y","TORPOR Z","APATHY X","APATHY Y","APATHY Z","INERTIA X","INERTIA Y","INERTIA Z","SLOTH Z-","SLOTH Z+"};return n[s<12?s:0];}
const char *pw_matrix_source_rate(unsigned s){return !s?"NO OUTPUT":s<4?"15-30 SEC":s<7?"60-90 SEC":s<10?"30-40 MIN":"MIXED RATES";}

const char *pw_matrix_dest_name(unsigned d){static const char *n[]={"NONE","FILTER CUTOFF","FILTER RESONANCE","WAVE POS %","BASE WAV %","PITCH WAVE %","WAVE DEPTH %"};return n[d<7?d:0];}
