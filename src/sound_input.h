#include <array>
#include <esp_dsp.h>

#define FFT_SIZE 256
#define BUFFER_SAMPLES FFT_SIZE
#define BUFFER_SHIFT 8
#define SAMPLE_RATE 8192
#define NUM_BUFFERS 3
#define MAX_AMPLITUDE_SQUARED 65535

class SoundInput
{
  public:
    typedef std::array<int16_t, FFT_SIZE> BufType;
    typedef std::array<int32_t, FFT_SIZE / 2> SpectrumBufType;

    void init()
    {
        nextWriteBuffer = 0;
        // setup fft
        dsps_fft2r_init_sc16(NULL, FFT_SIZE);
        // create log10 table to give 
        // 256 evenly spaced log10 points from
        // 0 to MAX_AMPLITUDE_SQUARED
        float max_power=log10(MAX_AMPLITUDE_SQUARED);
        for(int c=0;c<256;c++)
        {
            float power = (((float)c) *max_power)/256.0;
            log10Table[c] = (int32_t)(pow10f(power));
            //Serial.println(log10Table[c]);
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
        return lastLevel;
    }
    SpectrumBufType getSpectrum();

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
        if (sum > 32767)
        {
            Serial.println("Bad amplitude somehow");
            sum = 32767;
        }
        lastLevel = (int16_t)sum;
        //Serial.print("!");
        //Serial.println(lastLevel);
    }
    // do fft - n.b. this is in-place,
    // so do this *after* calculating amplitude
    void doFFT(BufType &inBuf)
    {
   //     Serial.println("*");
        int16_t *bufData = inBuf.data();
        // apply window
        dsps_mul_s16(bufData, window.data(), bufData, FFT_SIZE, 1, 1, 1, 15);
  //      Serial.println(bufData[FFT_SIZE>>3]);
        // do fft as if it was complex
        dsps_fft2r_sc16(bufData, FFT_SIZE >> 1);
  //      Serial.println(bufData[FFT_SIZE>>3]);
        // now flip things round
        // for a real power spectrum
        dsps_bit_rev_sc16_ansi(bufData, FFT_SIZE >> 1);
  //      Serial.println(bufData[FFT_SIZE>>3]);
        dsps_cplx2real_sc16_ansi(bufData, FFT_SIZE >> 1);
  //      Serial.println(bufData[FFT_SIZE>>3]);
        // now calculate magnitudes from the (complex)
        // output spectrum
        int16_t maxMag=-1;
        int16_t maxPos=0;
        for (int c = 0; c < lastSpectrum.size(); c++)
        {
            int32_t sum1 = (int32_t)(bufData[c * 2]);
            int32_t sum2 = (int32_t)(bufData[c * 2 + 1]);
            sum1 *= sum1;
            sum2 *= sum2;
            lastSpectrum[c] =lookupLog10(sum1+sum2);
            if(lastSpectrum[c]>maxMag){
                maxMag=lastSpectrum[c];
                maxPos=c;
            }
        }
/*        for(int c=0;c<lastSpectrum.size();c++){
            if(lastSpectrum[c]<(50)){
                Serial.print("_");
            }else if(lastSpectrum[c]<(100)){
                Serial.print("-");
            }else{
                Serial.print("^");
            }
        }
        Serial.println("");*/
    }

    int16_t lookupLog10(int32_t input)
    {
        for(int16_t c=0;c<256;c++)
        {
            if(input<log10Table[c])
            {
                return c;
            }
        }
        return 255;
    }

    std::array<BufType, NUM_BUFFERS> inBuffers;
    int nextWriteBuffer;

    SpectrumBufType lastSpectrum;
    int16_t lastLevel;
    int16_t lastMaxFreq;
    BufType window;
    std::array<int32_t,256> log10Table;
};