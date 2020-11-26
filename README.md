# DRV2667
Arduino library for [DRV2667](http://www.ti.com/product/DRV2667) Piezo Haptic Driver with Boost, Digital Front End, and Internal Waveform Memory

This library is strongly inspired by great works of [Fyber Labs](https://github.com/yurikleb/DRV2667)


## Usage

```C++
#include "DRV2667.h"
DRV2667 drv;

struct EmbeddedDevices::DRV2667::Synthesizer synth {
    {255, 0x15, 50, 0x09},
    {255, 0x17, 50, 0x09},
};

void setup() {
    Wire.begin(21, 22);
    drv.attatch(Wire);
    drv.addSynthesizer(synth);
    drv.setWaveformOrderAndID(0, 1); // play waveform id 1
    drv.gain(DRV2667::Gain::D25V_A288dB);
}

void loop() {
    drv.play();
    delay(3000);
}
```


## License

MIT

