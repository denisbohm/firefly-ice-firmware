#ifndef FDI_CURRENT_H
#define FDI_CURRENT_H

// rs = Current Sense Resistor
// ri = LTC2433 Current Sense Amplifier Rin Resistor
// ro = LTC2433 Current Sense Amplifier Rout Resistor
// rf = TLV272 Op Amp Rf Feedback Resistor
// rg = TLV272 Op Amp Rg Gain Resistor
float fdi_current_sense_gain(float rs, float ri, float ro, float rf, float rg);

#endif