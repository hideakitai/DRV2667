#pragma once
#ifndef DRV2667_H
#define DRV2667_H

#include <Wire.h>
#include "DRV2667/Effect.h"

namespace EmbeddedDevices
{
    namespace DRV2667
    {
        class DRV2667
        {
            static const uint8_t I2C_ADDR = 0x59;

        public:

            enum class Gain { D25V_A288dB, D50V_A348dB, D75V_A384dB, D100V_A407dB };
            enum class Input { DIGITAL_IN, ANALOG_IN };
            enum class Timeout { MS_5, MS_10, MS_15, MS_20 };

            virtual ~DRV2667() {}

            void attatch(TwoWire& w) { wire = &w; }

            void play()
            {
                standby(false);
                go(); // start waveform seq
            }

            void setToAnalogInput()
            {
                standby(false);
                selectInput(Input::ANALOG_IN);
                boost(true);
            }

            // 0 : success
            // 1 : data too long
            // 2 : NACK on transmit of address
            // 3 : NACK on transmit of data
            // 4 : other
            uint8_t getI2CStatus() const { return status_; }

            void printStatus() const
            {
                Serial.print("current I2C status : ");
                Serial.println(status_);
            }


            void gain(const Gain g)
            {
                reg_01 &= (0xFF & (uint8_t)g);
                write(0x01, reg_01);
            }


            void selectInput(const Input input)
            {
                if ((bool)input) reg_01 |= (1 << 2);
                else             reg_01 &= ~(1 << 2);
                write(0x01, reg_01);
            }

            void go()
            {
                reg_02 |= 0x01;
                write(0x02, reg_02);
            }

            void boost(const bool b)
            {
                if (b) reg_02 |= (1 << 1); // EN_OVERRIDE
                else   reg_02 &= ~(1 << 1);
                write(0x02, reg_02);
            }


            void timeout(const Timeout timeout)
            {
                reg_02 |= ((uint8_t)timeout << 2);
                write(0x02, reg_02);
            }

            void standby(bool b)
            {
                if (b) reg_02 |= (1 << 6);
                else   reg_02 &= ~(1 << 6);
                write(0x02, reg_02);
            }

            void reset()
            {
                reg_02 |= (1 << 7);
                write(0x02, reg_02);
            }

            void setWaveformOrderAndID(uint8_t order, uint8_t id)
            {
                // if id == 0, stop playing
                // else continue to play until the it reaches to id == 7

                if (order > 8) return;
                waveform_ids[order] = id;
                for (uint8_t i = 0; i < 8; ++i)
                    write(0x03 + i, waveform_ids[i]);
            }

            void setMemoryPage(uint8_t page)
            {
                write(0xFF, page);
            }

#ifndef __AVR__
            void setRepeat(uint8_t i, uint8_t r)
            {
                effects.setRepeat(i, r);
            }
#endif

#ifdef __AVR__

            void addSynthesizer(const SynthChunk* synthparams, const uint8_t size_of_synth)
            {
                const uint8_t SYNTH_PARAMS_BYTES = 4;
                const uint8_t SYNTH_HEADER_BYTES = 5;
                const uint8_t SYNTH_MODE_BITS = 0x80;
                setMemoryPage(0x01); // go to RAM

                Serial.println("write header size");

                // header size
                uint8_t header_size = 5; // for one sequence....
                write(0x00, header_size);

                Serial.println("write waveform");

                for (uint8_t i = 0; i < size_of_synth; ++i)
                {
                    uint16_t start_addr = (uint16_t)header_size + 1 + 4 * i;

                    if (i == 0)
                    {
                        setMemoryPage(0x01); // go to RAM

                Serial.println("write waveform header");

                        // header
                        uint8_t start_addr_h = (uint8_t)((start_addr >> 8) & 0x03);
                        uint8_t start_addr_l = (uint8_t)(start_addr & 0x00FF);
                        write(0x01 + SYNTH_HEADER_BYTES * i, start_addr_h + SYNTH_MODE_BITS);
                        write(0x02 + SYNTH_HEADER_BYTES * i, start_addr_l);
                        write(0x03 + SYNTH_HEADER_BYTES * i, start_addr_h); // TODO:
                        write(0x04 + SYNTH_HEADER_BYTES * i, start_addr_l + SYNTH_PARAMS_BYTES * size_of_synth - 1);
                        write(0x05 + SYNTH_HEADER_BYTES * i, 1); // 0x00 means repeat endlessly
                    }

                Serial.print("write waveform data ");
                Serial.println(i);

                    // synthesizer parameters
                    write(start_addr + 0, synthparams[i].amp);
                    write(start_addr + 1, synthparams[i].freq);
                    write(start_addr + 2, synthparams[i].cycle);
                    write(start_addr + 3, synthparams[i].envelop);
                }

                Serial.println("return to control space");

                setMemoryPage(0x00); // back to register control space
            }

#else

            // TODO: support multiple synth effects
            // TODO: vector support...

            void addWaveform(const uint8_t* const data, const uint16_t size)
            {
                effects.append(data, size);
                setEffects();
            }

            void addSynthesizer(const Synthesizer& synth)
            {
                effects.append(synth);
                setEffects();
            }

            void setEffects()
            {
                if (effects.isMaxSize()) return;

                setMemoryPage(0x01); // go to RAM

                // 0x000 : header size
                write(0x00, effects.getHeaderSize());

                // 0x001- : headers
                for (uint8_t i = 0; i < effects.size(); ++i)
                {
                    write(effects.getHeaderAddrStart(i) + 0x00, effects.getEffectAddrStartH(i));
                    write(effects.getHeaderAddrStart(i) + 0x01, effects.getEffectAddrStartL(i));
                    write(effects.getHeaderAddrStart(i) + 0x02, effects.getEffectAddrStopH(i));
                    write(effects.getHeaderAddrStart(i) + 0x03, effects.getEffectAddrStopL(i));
                    write(effects.getHeaderAddrStart(i) + 0x04, effects.getRepeatCount(i)); // 0x00 means repeat endlessly
                }

                // effects
                for (uint8_t i = 0; i < effects.size(); ++i)
                {
                    setMemoryPage(effects.getEffectStartPage(i));
                    switch (effects.getPlayMode(i))
                    {
                        case PlayMode::Direct:
                        {
                            for (size_t j = 0; j < effects.size(i); ++i)
                                write(effects.getEffectAddrStartL(i), effects.data(i, j));
                            break;
                        }
                        case PlayMode::Synthesis:
                        {
                            for (size_t j = 0; j < effects.size(i); ++j)
                            {
                                write(effects.getEffectAddrChunk(i, j) + 0x00, effects.amp(i, j));
                                write(effects.getEffectAddrChunk(i, j) + 0x01, effects.freq(i, j));
                                write(effects.getEffectAddrChunk(i, j) + 0x02, effects.cycle(i, j));
                                write(effects.getEffectAddrChunk(i, j) + 0x03, effects.envelop(i, j));
                            }
                            break;
                        }
                    }
                }
                setMemoryPage(0x00); // back to register control space
            }

            void amp(uint8_t i, uint8_t j, uint8_t v)
            {
                effects.amp(i, j, v);
                setMemoryPage(0x01); // go to RAM
                write(effects.getEffectAddrChunk(i, j) + 0x00, v);
                setMemoryPage(0x00); // back to register control space
                // setEffects();
            }
#endif

            void write(const uint8_t reg, const uint8_t data, bool stop = true)
            {
                wire->beginTransmission(I2C_ADDR);
                wire->write(reg);
                wire->write(data);
                status_ = wire->endTransmission(stop);

                if (status_ != 0)
                {
                    Serial.print("I2C error : ");
                    Serial.println(status_);
                }
            }

            uint16_t read(const uint8_t reg)
            {
                // write(reg, false);
                wire->beginTransmission(I2C_ADDR);
                wire->write(reg);
                status_ = wire->endTransmission(false);

                if (status_ != 0)
                {
                    Serial.print("I2C error : ");
                    Serial.println(status_);
                }

                if (wire->requestFrom((uint8_t)I2C_ADDR, (uint8_t)1))
                    return wire->read();

                return 0xFF;
            }

        private:

            TwoWire* wire;

#ifndef __AVR__
            Effects effects;
#endif

            uint8_t reg_01 {0x38};
            uint8_t reg_02 {0x40};

            uint8_t waveform_ids[8] {0};
            uint8_t status_;
        };
    }
}

using DRV2667 = EmbeddedDevices::DRV2667::DRV2667;
#ifdef __AVR__
using DRV2667Effects = EmbeddedDevices::DRV2667::SynthChunk;
#else
using DRV2667Synthesizer = EmbeddedDevices::DRV2667::Synthesizer;
using DRV2667Waveform = EmbeddedDevices::DRV2667::Waveform;
#endif

#endif // DRV2667_H
