#pragma once

#include <array>
#include <esp_dsp.h>

class SoundInput
{
  public:
    static const int FFT_SIZE = 256;
    static const int BUFFER_SAMPLES = FFT_SIZE;
    static const int BUFFER_SHIFT = 8;
    static const int SAMPLE_RATE = 8192;
    static const int NUM_BUFFERS = 3;
    static const int MAX_AMPLITUDE_SQUARED = 65535;
    typedef std::array<int16_t, FFT_SIZE> BufType;
    typedef std::array<int32_t, FFT_SIZE / 2> SpectrumBufType;

    void init()
    {
        lastLevel = 0;
        smoothedPeak = -1;
        singleton = this;
        nextWriteBuffer = 0;
        // setup fft
        dsps_fft2r_init_sc16(NULL, FFT_SIZE);
        // create log10 table to give
        // 256 evenly spaced log10 points from
        // 0 to MAX_AMPLITUDE_SQUARED
        float max_power = log10(MAX_AMPLITUDE_SQUARED);
        for (int c = 0; c < 256; c++)
        {
            float power = (((float)c) * max_power) / 256.0;
            log10Table[c] = (int32_t)(pow10f(power));
            float sVal = ((float)c) / 256.0;
            sVal = sqrt(sVal);
            sqrtTable[c] = (uint16_t)(sVal * 256.0);
        }
        int curPos = 0;
        int breakPoint = 0;
        for (int c = 0; c < 6; c++)
        {
            breakPoint += 3 + 5 * (c + 1);
            while (curPos < breakPoint && curPos < spectrumSegmentIndices.size())
            {
                spectrumSegmentIndices[curPos] = c;
//                Serial.print(curPos);
//                Serial.print(":");
//                Serial.println(c);
                curPos++;
            }
        }
        while (curPos < spectrumSegmentIndices.size())
        {
            spectrumSegmentIndices[curPos] = 5;
            curPos++;
        }
        // create hann window for fft data
        float fWindow[FFT_SIZE];
        dsps_wind_hann_f32(fWindow, FFT_SIZE);
        for (int c = 0; c < FFT_SIZE; c++)
        {
            window[c] = (int16_t)(fWindow[c] * 32767.0);
        }
        // setup mic to read buffers of size multiple of FFT_SIZE
        auto cfg = M5.Mic.config();
        cfg.dma_buf_count = NUM_BUFFERS;
        cfg.dma_buf_len = BUFFER_SAMPLES * 2;
        cfg.over_sampling = 1;
        cfg.noise_filter_level = 0;
        cfg.sample_rate = SAMPLE_RATE;
        cfg.magnification = cfg.use_adc ? 16 : 1;
        M5.Mic.config(cfg);
        M5.Mic.begin();
    }

    static SoundInput *GetSoundInput()
    {
        return singleton;
    }

    virtual ~SoundInput()
    {
        // stop mic
        M5.Mic.end();
        // clear fft setup
        dsps_fft2r_deinit_sc16();
    }
    void doProcessing()
    {
        if (!M5.Mic.isEnabled())
        {
            Serial.println("No mic");
            return;
        }
        // pass buffers to mic
        // we keep N-1 buffers in the mic queue
        // meaning that we can assume that
        // the next write buffer is going to
        // be ready to process now
        bool processOutput = false;
        while (M5.Mic.isRecording() < NUM_BUFFERS - 1)
        {
            M5.Mic.record(inBuffers[nextWriteBuffer].data(), inBuffers[nextWriteBuffer].size(),
                          SAMPLE_RATE, false);
            nextWriteBuffer++;
            if (nextWriteBuffer >= inBuffers.size())
            {
                nextWriteBuffer = 0;
            }
            processOutput = true;
        }
        if (processOutput)
        {

            // process the next write buffer now
            calcAmplitude(inBuffers[nextWriteBuffer]);
            doFFT(inBuffers[nextWriteBuffer]);
        }
    }
    int16_t getLevel()
    {
        const int BASE_LEVEL=100;
        int audioV=lastLevel;
        if(audioV<BASE_LEVEL){
            audioV=0;
        }else{
            audioV=(255*(audioV-BASE_LEVEL))/(255-BASE_LEVEL);
        }
        //Serial.println(audioV);
        return audioV;
    }
    const SpectrumBufType &getSpectrum()
    {
        return lastSpectrum;
    }

    const SpectrumBufType &getSpectrumSegmentIndices()
    {
        return spectrumSegmentIndices;
    }

  protected:
    void calcAmplitude(BufType &inBuf)
    {
        int32_t sum = 0;
        for (int c = 0; c < inBuf.size(); c++)
        {
            if (inBuf[c] > 0)
            {
                sum += (int32_t)inBuf[c];
            }
            else
            {
                sum -= (int32_t)inBuf[c];
            }
        }
        sum = sum >> BUFFER_SHIFT;

  //      int32_t sumH=sum>>8;
  //      int32_t sumL= sum&0xff;
 //       if(sumH>255)sumH=255;
//        sum = (((int32_t)sqrtTable[sumH])<<8)|sumL;

        if (smoothedPeak == -1)
        {
            smoothedPeak = sum<<8;
        }
        else
        {
            if((sum<<8)>smoothedPeak){
                smoothedPeak = (smoothedPeak * 15 + (sum<<8)) >> 4;
            }else{
                smoothedPeak = (smoothedPeak *254)>>8;
            }
        }
        int32_t divisor=smoothedPeak>>7;
        if(divisor==0)divisor=1;
        lastLevel = (sum *255)/ divisor;
        if (lastLevel > 255)
            lastLevel = 255;
        
        //  Serial.print(sum);
        //  Serial.print("!");
        //  Serial.print(smoothedPeak>>8);
        //  Serial.print(":");
        //  Serial.println(smoothedPeak);


        // Serial.print(",");
        // Serial.println(lastLevel);



    }
    // do fft - n.b. this is in-place,
    // so do this *after* calculating amplitude
    void doFFT(BufType &inBuf)
    {
        int16_t *bufData = inBuf.data();
        // apply window
        dsps_mul_s16(bufData, window.data(), bufData, FFT_SIZE, 1, 1, 1, 15);
        // do fft as if it was complex
        dsps_fft2r_sc16(bufData, FFT_SIZE >> 1);
        // now flip things round
        // for a real power spectrum
        dsps_bit_rev_sc16_ansi(bufData, FFT_SIZE >> 1);
        dsps_cplx2real_sc16_ansi(bufData, FFT_SIZE >> 1);
        // now calculate magnitudes from the (complex)
        // output spectrum
        for (int c = 0; c < lastSpectrum.size(); c++)
        {
            int32_t sum1 = (int32_t)(bufData[c * 2]);
            int32_t sum2 = (int32_t)(bufData[c * 2 + 1]);
            sum1 *= sum1;
            sum2 *= sum2;
            lastSpectrum[c] = lookupLog10(sum1 + sum2);
        }
    }

    int16_t lookupLog10(int32_t input)
    {
        for (int16_t c = 0; c < 256; c++)
        {
            if (input < log10Table[c])
            {
                return c;
            }
        }
        return 255;
    }

    std::array<BufType, NUM_BUFFERS> inBuffers;
    int nextWriteBuffer;

    SpectrumBufType lastSpectrum;
    SpectrumBufType spectrumSegmentIndices;
    int32_t lastLevel;
    int32_t smoothedPeak;
    int16_t lastMaxFreq;
    BufType window;
    std::array<int32_t, 256> log10Table;
    std::array<uint8_t, 256> sqrtTable;

    inline static SoundInput *singleton = NULL;
};