#pragma once
#ifndef DRV2667_EFFECT_H
#define DRV2667_EFFECT_H

#ifndef __AVR__
#include <vector>
#include <memory>
#include <initializer_list>
#endif

        // The Direct Playback from RAM mode requires no special handling: the waveform starts at the start-address
        // location and plays each sub-sequent byte at the Nyquist-rate. The data is stored in twos complement, where
        // 0xFF is interpreted as full-scale, 0x00 is no signal, and 0x80 is negative full-scale. The waveform is played at an
        // 8-kHz data rate.
        // The Waveform Synthesis Playback Mode stores data in sinusoidal chunks, where each chunk consists of four
        // bytes as shown in Figure 28:
        // - Amplitude
        // - Frequency
        // - Number of Cycles (Duration)
        // - Envelope

        // The amplitude byte refers to the magnitude of the synthesized sinusoid. 0xFF produces a fullscale
        // sinusoid, 0x80 produces a half-scale sinusoid, and 0x00 does not produce any signal. An
        // amplitude of 0x00 can be useful for producing timed waits or delays within the effect.
        // To calculate the absolute peak voltage, use the following equation, where amplitude is a singlebyte
        // integer:
        // Peak voltage = amplitude / 255 x full-scale peak voltage

        // The frequency byte adjusts the frequency of the synthesized sinusoid. The minimum frequency is
        // 7.8125 Hz. A value of zero is not allowed. The sinusoidal frequency is determined with the
        // following equation, where frequency is a single-byte integer:
        // Sinusoid frequency (Hz) = 7.8125 x frequency

        // The number of sinusoidal cycles to be played by the synthesizer. A convenient way to specify the
        // duration of a coherent sinusoid is by inputting the number of cycles. This method ensures that
        // the waveform chunk will always begin and end at zero amplitude, thus avoiding discontinuities.
        // The actual duration in time given by this value may be calculated through the following equation,
        // where # of cycles and frequency are both single-byte integers.
        // Duration (ms) = 1000 x # of cycles / (7.8125 x frequency)

        // The envelope byte is divided into two nibbles. The upper nibble, bits [7:4], sets the ramp-up rate
        // at the beginning of the synthesized sinusoid, and the lower nibble, bits [3:0], sets the ramp-down
        // rate at the end of the synthesized sinusoid. The user must note that the ramp-up time is included
        // in the duration parameter of the waveform, and the ramp-down time is appended to the duration
        // parameter of the waveform. As such, if a ramp-up time is used, the ramp-up time must be less
        // than the duration time as programmed in byte 3. Also note that the Total Ramp Time is for a
        // ramp to full-scale amplitude (amplitude = 0xFF). Ramps to a fraction of full-scale have the same
        // fraction of the Total Ramp Time.

namespace EmbeddedDevices
{
    namespace DRV2667
    {
        enum class PlayMode { Direct, Synthesis };

#ifdef __AVR__

        struct Synthesizer
        {
            uint8_t amp;
            uint8_t freq;
            uint8_t cycle;
            uint8_t envelop;
        };

#else

        class ChunkBase
        {
        public:
            ChunkBase(std::initializer_list<std::vector<uint8_t>> list)
            : repeat_(1)
            , size_(list.size())
            , raw_(list.begin(), list.end())
            {}
            virtual ~ChunkBase() {}

            const uint8_t data(const uint8_t i) const { return raw_.front()[i]; }
            const uint8_t size() const { return size_; }
            const uint8_t repeat() const { return repeat_; }
            void setRepeat(uint8_t r) { repeat_ = r; }

            virtual const uint8_t amp(const uint8_t i) const = 0;
            virtual const uint8_t freq(const uint8_t i) const = 0;
            virtual const uint8_t cycle(const uint8_t i) const = 0;
            virtual const uint8_t envelop(const uint8_t i) const = 0;

            virtual void amp(const uint8_t i, const uint8_t v) { }
            virtual void freq(const uint8_t i, const uint8_t v) { }
            virtual void cycle(const uint8_t i, const uint8_t v) { }
            virtual void envelop(const uint8_t i, const uint8_t v) { }

            virtual const PlayMode getPlayMode() const = 0;

        // private:
            uint8_t repeat_;
            uint8_t size_;
            std::vector<std::vector<uint8_t>> raw_;
        };

        class Waveform : public ChunkBase
        {
        public:
            Waveform(const uint8_t* const data, const uint16_t size, const uint8_t repeat = 1)
            : ChunkBase({})
            {
                this->repeat_ = repeat;
                this->size_ = size;
                std::vector<uint8_t> v;
                for (uint16_t i = 0; i < size; ++i) v.push_back(data[i]);
                this->raw_.push_back(std::move(v));
            }
            virtual ~Waveform() {}

            virtual const uint8_t amp(const uint8_t i) const override { return 0; }
            virtual const uint8_t freq(const uint8_t i) const override { return 0; }
            virtual const uint8_t cycle(const uint8_t i) const override { return 0; }
            virtual const uint8_t envelop(const uint8_t i) const override { return 0; }

            virtual const PlayMode getPlayMode() const override { return PlayMode::Direct; }
        };

        class Synthesizer : public ChunkBase
        {
        public:
            // Synthesizer() {}
            Synthesizer(std::initializer_list<std::vector<uint8_t>> list)
            : ChunkBase(list)
            {
            }

            void appendChunk(uint8_t amp, uint8_t freq, uint8_t cycle, uint8_t envelop)
            {
                raw_.emplace_back(std::vector<uint8_t> {amp, freq, cycle, envelop});
            }

            const uint16_t size() const { return raw_.size(); }

            virtual const uint8_t amp(const uint8_t i) const override { return raw_[i][0]; }
            virtual const uint8_t freq(const uint8_t i) const override { return raw_[i][1]; }
            virtual const uint8_t cycle(const uint8_t i) const override { return raw_[i][2]; }
            virtual const uint8_t envelop(const uint8_t i) const override { return raw_[i][3]; }

            virtual void amp(const uint8_t i, const uint8_t v) override { raw_[i][0] = v; }
            virtual void freq(const uint8_t i, const uint8_t v) override { raw_[i][1] = v; }
            virtual void cycle(const uint8_t i, const uint8_t v) override { raw_[i][2] = v; }
            virtual void envelop(const uint8_t i, const uint8_t v) override { raw_[i][3] = v; }

            virtual const PlayMode getPlayMode() const override { return PlayMode::Synthesis; }
        };


        class Effects
        {
        public:
            void append(const uint8_t* const data, const uint16_t size, uint8_t repeat = 1)
            {
                effects.emplace_back(std::make_shared<Waveform>(data, size, repeat));
            }
            void append(const Synthesizer& synth, uint8_t repeat = 1)
            {
                effects.push_back(std::make_shared<Synthesizer>(synth));
            }

            static const uint8_t HEADER_BYTES = 0x05;
            static const uint8_t SYNTH_DATA_BYTES = 0x04;
            static const uint8_t MAX_WAVEFORM_SIZE = 50;
            static const uint8_t SYNTH_MODE_BITS = 0x80;


            const uint8_t size() const { return effects.size(); }
            const uint8_t size(uint8_t i) const { return effects[i]->size(); }
            // const uint16_t bytes() const { return ;}
            const uint16_t bytes(uint8_t i) const { return (uint16_t)effects[i]->size() * (uint16_t)SYNTH_DATA_BYTES; }
            const uint8_t getHeaderSize() const { return effects.size() * HEADER_BYTES; }

            const uint16_t getHeaderAddrStart(const uint8_t id) const
            {
                return uint16_t(0x01 + HEADER_BYTES * id);
            }

            const uint16_t getEffectAddrStart(const uint8_t id) const
            {
                uint16_t offset = 0;
                for (uint8_t i = 0; i < id; ++i) offset += effects[i]->size();
                return uint16_t(0x01 + getHeaderSize() + offset);
            }
            const uint8_t getEffectAddrStartL(const uint8_t id) const
            {
                return uint8_t(getEffectAddrStart(id) & 0x00FF);
            }
            const uint8_t getEffectAddrStartH(const uint8_t id) const
            {
                return uint8_t((SYNTH_MODE_BITS + uint16_t((getEffectAddrStart(id) >> 8) & 0x00FF)));
            }
            const uint16_t getEffectAddrStop(const uint8_t id) const
            {
                return uint16_t(getEffectAddrStart(id) + bytes(id) - 1);
            }
            const uint8_t getEffectAddrStopL(const uint8_t id) const
            {
                return uint8_t(getEffectAddrStop(id) & 0x00FF);
            }
            const uint8_t getEffectAddrStopH(const uint8_t id) const
            {
                return uint8_t((getEffectAddrStop(id) >> 8) & 0x00FF);
            }
            const uint8_t getRepeatCount(const uint8_t id) const
            {
                return effects[id]->repeat();
            }

            const uint8_t getEffectAddrChunk(const uint8_t id, const uint8_t n) const
            {
                return getEffectAddrStartL(id) + SYNTH_DATA_BYTES * n;
            }

            const uint8_t amp(const uint8_t i, const uint8_t j) const { return effects[i]->amp(j); }
            const uint8_t freq(const uint8_t i, const uint8_t j) const { return effects[i]->freq(j); }
            const uint8_t cycle(const uint8_t i, const uint8_t j) const { return effects[i]->cycle(j); }
            const uint8_t envelop(const uint8_t i, const uint8_t j) const { return effects[i]->envelop(j); }

            void amp(const uint8_t i, const uint8_t j, const uint8_t v) { effects[i]->amp(j, v); }
            void freq(const uint8_t i, const uint8_t j, const uint8_t v) { effects[i]->freq(j, v); }
            void cycle(const uint8_t i, const uint8_t j, const uint8_t v) { effects[i]->cycle(j, v); }
            void envelop(const uint8_t i, const uint8_t j, const uint8_t v) { effects[i]->envelop(j, v); }

            const uint8_t data(uint8_t i, uint8_t j) const { return effects[i]->data(j); }

            const uint8_t getEffectStartPage(uint8_t i) { return getEffectAddrStartH(i) + 1; }
            const PlayMode getPlayMode(uint8_t i) { return effects[i]->getPlayMode(); }

            const bool isMaxSize() { return (size() >= MAX_WAVEFORM_SIZE); }

            void setRepeat(uint8_t i, uint8_t r) { effects[i]->setRepeat(r); }

        private:

            std::vector<std::shared_ptr<ChunkBase>> effects;
        };

#endif // #ifndef __AVR__
    }
}

#endif // DRV2667_EFFECT_H
