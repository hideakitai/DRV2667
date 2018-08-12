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
    {255, 0x15, 10, 0},
    {255, 0x17, 50, 0}
};
#endif

void setup()
{
    Serial.begin(115200);

    Serial.println("DRV2667 test setup");
    drv.begin();
    Serial.println("DRV2667 add synth");

#ifdef __AVR__
    drv.addSynthesizer(synth, 2);
#else
    drv.addSynthesizer(synth);
#endif

    Serial.println("set waveform");
    drv.setWaveformOrderAndID(0, 1); // in 0th order, play waveform id 1 (waveform id 0 means stop)
    Serial.println("set gain");
    drv.gain(DRV2667::Gain::D25V_A288dB);

    Serial.println("DRV2667 test start");
}

uint8_t amp = 0;
uint32_t prev_ms = 0;

void loop()
{
    // 40fps is
    if (millis() - prev_ms > 25)
    {
        ++amp;
        drv.amp(0, 0, amp);
        prev_ms = millis();
        Serial.println(amp);
    }
    drv.play();
}
