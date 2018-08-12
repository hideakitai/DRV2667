#include "DRV2667.h"

DRV2667 drv;

struct EmbeddedDevices::DRV2667::SynthChunk synth[2]
{
    {255, 0x15, 50, 0x09},
    {255, 0x17, 50, 0x09}
};

void setup()
{
    Serial.begin(115200);

    drv.begin();
    drv.setSynthsizer(synth, 2);
    drv.setWaveformOrderAndID(0, 1); // in 0th order, play waveform id 1 (waveform id 0 means stop)
    drv.gain(DRV2667::Gain::D25V_A288dB);

    Serial.println("DRV2667 test start");
}

void loop()
{
      drv.play();
      delay(3000); //Wait for the wave to play;
}
