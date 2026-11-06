/* Port of SlothCircuit.hpp by Don Cross, CC0-1.0.
 * Same circuit equations, predictor/corrector and comparator-crossing treatment.
 * FM1 uses float arithmetic and a 200 Hz control update, interpolated to audio. */
#include "pw_sloth.h"
static float Q(float z){return z<0.0f?11.38f:-10.64f;}
void pw_sloth_balance(pw_sloth*s,float f){if(f<0)f=0;if(f>1)f=1;s->K=100000.0f+f*10000.0f;}
void pw_sloth_init(pw_sloth*s,unsigned variant){*s=(pw_sloth){0};s->dilation=variant==0?1.0f:variant==1?0.27391343022607395f:0.009697118406631193f;s->w=variant==1?.017f:variant==2?-.023f:0.0f;pw_sloth_balance(s,variant==2?0.0f:.5f);}
unsigned pw_sloth_update(pw_sloth*s,float rate){
 float dt=s->dilation/rate,xm=s->x+s->dx/2.0f,wm=s->w+s->dw/2.0f,ym=s->y+s->dy/2.0f;
 float zm=-100000.0f*(ym/100000.0f+s->U/470000.0f),qm=Q(zm);
 for(unsigned iter=1;;iter++){
  float ex=s->dx,ew=s->dw,ey=s->dy;
  s->dx=-dt/2.0e-6f*(zm/1.0e6f+qm/4.7e6f+wm/s->K);
  s->dw=dt/50.0e-6f*(xm/100000.0f-(1.0f/100000.0f+1.0f/s->K+1.0f/100000.0f)*wm);
  s->dy=(-dt/(100000.0f*1.42e-6f))*wm;
  float x2=s->x+s->dx,w2=s->w+s->dw,y2=s->y+s->dy,z2=-100000.0f*(y2/100000.0f+s->U/470000.0f);
  float ddx=s->dx-ex,ddw=s->dw-ew,ddy=s->dy-ey;
  if(ddx*ddx+ddw*ddw+ddy*ddy<1.0e-24f||iter>=5){s->x=x2;s->w=w2;s->y=y2;s->z=z2;return iter;}
  xm=s->x+s->dx/2.0f;wm=s->w+s->dw/2.0f;zm=(s->z+z2)/2.0f;
  if(s->z*z2>=0.0f)qm=Q(zm);else{float a=s->z/(s->z-z2);qm=a*Q(s->z)+(1.0f-a)*Q(z2);}
 }
}
