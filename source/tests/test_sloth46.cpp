#include "sloth_reference/SlothCircuitFloat.hpp"
#include "../reference/nonlinearcircuits/SlothCircuit.hpp"
extern "C" {
#include "../fm1/app/pw_sloth.h"
}
#include <cstdio>
#include <cassert>
#include <cmath>
int main(){AnalogFloat::TripleSlothCircuit ref;pw_sloth native[3];ref.torpor.setKnobPosition(.5f);ref.apathy.setKnobPosition(.5f);for(unsigned i=0;i<3;i++)pw_sloth_init(&native[i],i);
float largest=0,peak=0;for(unsigned n=0;n<720000;n++){
 ref.update(200);for(unsigned i=0;i<3;i++){
 pw_sloth_update(&native[i],200);float x=i==0?ref.torpor.xVoltage():i==1?ref.apathy.xVoltage():ref.inertia.xVoltage();float y=i==0?ref.torpor.yVoltage():i==1?ref.apathy.yVoltage():ref.inertia.yVoltage();float z=i==0?ref.torpor.zVoltage():i==1?ref.apathy.zVoltage():ref.inertia.zVoltage();
 float error=fmaxf(fabsf(x-native[i].x),fmaxf(fabsf(y-native[i].y),fabsf(z-native[i].z)));largest=fmaxf(largest,error);peak=fmaxf(peak,fabsf(native[i].x));assert(std::isfinite(error)&&error<.0001f);assert(std::isfinite(native[i].x)&&fabsf(native[i].x)<100);}}
// Short trajectory against original double precision at the same control rate.
Analog::TripleSlothCircuit precise;precise.torpor.setKnobPosition(.5);precise.apathy.setKnobPosition(.5);for(unsigned i=0;i<3;i++)pw_sloth_init(&native[i],i);double maxDouble=0;
for(unsigned n=0;n<2000;n++){precise.update(200);for(unsigned i=0;i<3;i++){pw_sloth_update(&native[i],200);double y=i==0?precise.torpor.yVoltage():i==1?precise.apathy.yVoltage():precise.inertia.yVoltage();maxDouble=fmax(maxDouble,fabs(y-native[i].y));}}assert(maxDouble<.001);
Analog::TripleSlothCircuit audio;audio.torpor.setKnobPosition(.5);audio.apathy.setKnobPosition(.5);for(unsigned i=0;i<3;i++)pw_sloth_init(&native[i],i);unsigned samples=0;double rateError=0;
for(unsigned n=0;n<2000;n++){unsigned end=(n+1)*44100/200;while(samples<end){audio.update(44100);++samples;}for(unsigned i=0;i<3;i++){pw_sloth_update(&native[i],200);double y=i==0?audio.torpor.yVoltage():i==1?audio.apathy.yVoltage():audio.inertia.yVoltage();rateError=fmax(rateError,fabs(y-native[i].y));}}assert(rateError<.01);
printf("PASS: first ten seconds versus original 44.1 kHz double solver: max Y error %.9g V.\n",rateError);
printf("PASS: one hour at 200 Hz matches float upstream solver (max error %.9g V, peak X %.4g V); first ten seconds vs double max Y error %.9g V.\n",largest,peak,maxDouble);
}
