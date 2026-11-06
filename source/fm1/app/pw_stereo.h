#pragma once
/* Mode0 retains FM1 dual-mono gain. Modes1/2/3 recover Voice/Pairs/Pitch. */
void pw_stereo_gains(unsigned mode,int spread,unsigned slot,unsigned count,int note,float *left,float *right);
