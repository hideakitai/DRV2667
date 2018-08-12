#include "DRV2667.h"

DRV2667 drv;

const uint8_t dac_pin = 25;
uint8_t vol = 0;
uint32_t prev_ms = 0;

void dacSquare(int pin, uint8_t vol, float hz = 150.f)
{
    static uint32_t prev_us = micros();
    static bool is_dac_high = false;

    uint32_t interval_us = (uint32_t)(1.f / hz * 1000000.f);
    if ((micros() - prev_us) > interval_us)
    {
        is_dac_high = !is_dac_high;
        prev_us = micros();
    }
    dacWrite(pin, (is_dac_high ? vol : 0));
}

void setup()
{
    delay(1000);

    drv.begin();
    drv.gain(DRV2667::Gain::D100V_A407dB);
    drv.setToAnalogInput();
}

void loop()
{
    if (millis() - prev_ms > 25)
    {
        ++vol;
        prev_ms = millis();
        Serial.println(vol);
    }
    dacSquare(25, vol);
}
