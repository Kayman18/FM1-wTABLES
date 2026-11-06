#pragma once
#include <stdint.h>
/* Don Cross / mhetrick NonlinearCircuits, CC0-1.0. Float C adaptation. */
typedef struct {float x,w,y,z,dx,dw,dy,K,U,dilation;} pw_sloth;
void pw_sloth_init(pw_sloth*,unsigned variant);
void pw_sloth_balance(pw_sloth*,float fraction);
unsigned pw_sloth_update(pw_sloth*,float sample_rate);
