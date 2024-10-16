// Copyright 2014 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Discriminate an ADC signal into audio or CV ; and provide RMS stats.

#pragma once

#include <stmlib/stmlib.h>

namespace streams
{

class AudioCvMeter
{
public:
    AudioCvMeter() { }
    ~AudioCvMeter() { }

    void Init()
    {
        peak_ = 0;

        zero_crossing_interval_ = 0;
        average_zero_crossing_interval_ = 0;

        previous_sample_ = 0;
        cv_ = false;
    }

    void Process(int32_t sample, uint32_t timestep_us)
    {
        if ((sample >> 1) * previous_sample_ < 0 ||
            zero_crossing_interval_ >= (4096L * kHardwareTimestep_us) / (int32_t)timestep_us)
        {
            int32_t error = zero_crossing_interval_ - average_zero_crossing_interval_;
            average_zero_crossing_interval_ += error >> 3;
            zero_crossing_interval_ = 0;
        }
        else
        {
            ++zero_crossing_interval_;
        }

        if (cv_ && average_zero_crossing_interval_ < (200L * kHardwareTimestep_us) / (int32_t)timestep_us)
        {
            cv_ = false;
        }
        else if (!cv_ && average_zero_crossing_interval_ > (400L * kHardwareTimestep_us) / (int32_t)timestep_us)
        {
            cv_ = true;
        }

        previous_sample_ = sample;

        if (sample < 0)
        {
            sample = -sample;
        }

        int32_t error = sample - peak_;
        int32_t coefficient = 33;  // 250ms at 1kHz

        if (error > 0)
        {
            coefficient = 809;  // 10ms at 1kHz
        }

        coefficient = (coefficient * timestep_us) / kHardwareTimestep_us;

        peak_ += error * coefficient >> 15;
    }

    inline bool cv() const
    {
        return cv_;
    }
    inline int32_t peak() const
    {
        return peak_;
    }

private:
    // Hardware Streams updates its LEDs at 4kHz (250us period), but we may be
    // using a different rate here. We take this into account when filtering
    // or interval-counting so that the software LEDs feel the same.
    static constexpr int32_t kHardwareTimestep_us = 250;

    bool cv_;
    int32_t peak_;
    int32_t zero_crossing_interval_;
    int32_t average_zero_crossing_interval_;
    int32_t previous_sample_;
};

}  // namespace streams
