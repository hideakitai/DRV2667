// #define __AVR__

#include "DRV2667.h"

DRV2667 drv;

#ifdef __AVR__
struct EmbeddedDevices::DRV2667::SynthChunk synth[2]
{
    {255, 0x15, 50, 0x09},
    {255, 0x17, 50, 0x09}
};
#else
struct EmbeddedDevices::DRV2667::Synthesizer synth
{
    {255, 0x15, 50, 0x09},
    {255, 0x17, 50, 0x09},
};
#endif

void setup()
{
    Serial.begin(115200);

    Serial.println("DRV2667 test setup");
    Wire.begin(21, 22);
    drv.attatch(Wire);
    Serial.print("status : ");
    Serial.print(drv.getStatus());
    Serial.println("DRV2667 add synth");

#ifdef __AVR__
    drv.addSynthesizer(synth, 2);
#else
    drv.addSynthesizer(synth);
#endif
    Serial.print("status : ");
    Serial.print(drv.getStatus());

    Serial.println("set waveform");
    drv.setWaveformOrderAndID(0, 1); // in 0th order, play waveform id 1 (waveform id 0 means stop)
    Serial.print("status : ");
    Serial.print(drv.getStatus());
    Serial.println("set gain");
    // drv.gain(DRV2667::Gain::D100V_A407dB);
    drv.gain(DRV2667::Gain::D25V_A288dB);
    Serial.print("status : ");
    Serial.println(drv.getStatus());

    Serial.println("DRV2667 test start");
}


void loop()
{

    drv.play();
    Serial.print("status : ");
    Serial.println(drv.getStatus());
    delay(3000); //Wait for the wavkVkke to play;

}
