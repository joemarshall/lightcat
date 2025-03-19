#pragma once

#include <FastLED.h>

class LightOutput
{
    public:
    static const int NUM_LED1= 60 * 3;
    static const int NUM_LED2= 60 * 3;
    static const int NUM_LEDS = (NUM_LED1 + NUM_LED2);
    static const int STRIP1_PIN = 32;
    static const int STRIP2_PIN = 32;

    // LED count of first strip of
    // each one - STRIP1 = short then long
    // STRIP2 = long then short
    static const int STRIP1_SPLIT_POINT = 60;
    static const int STRIP2_SPLIT_POINT = 120;

    void init()
    {
        FastLED.addLeds<WS2813, STRIP1_PIN>(strip1Buffer, NUM_LED1);
        FastLED.addLeds<WS2813, STRIP2_PIN>(strip2Buffer, NUM_LED2);
        singleton=this;
    }

    static LightOutput* GetLightOutput()
    {  
        return singleton;
    }

    const uint8_t GAMMA_TABLE[256] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,
        3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,
        7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,
        14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,
        23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,
        35,  36,  37,  38,  39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,
        51,  52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,
        72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,  90,  92,  93,  95,
        96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 124,
        126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158,
        160, 162, 164, 167, 169, 171, 173, 175, 177, 180, 182, 184, 186, 189, 191, 193, 196, 198,
        200, 203, 205, 208, 210, 213, 215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244,
        247, 249, 252, 255};

    enum EFFECT_TYPE
    {
        EFFECT_CONSTANT, // constant colour and brightness
        EFFECT_SPIN,     // colour and brightness cycle round all LEDs
        EFFECT_WHIRL,    // colour and light zoom out in both directions on the LEDs
        EFFECT_BLOCKS,   // colour and light on blocks - each colour sets another block
    };

    // set current colour as RGB - or level==-1 if no colour selected
    // for spin/whirl, where it will output blank space
    void onColour(int r, int g, int b, int level)
    {
        if (currentEffect == EFFECT_CONSTANT && level != -1)
        {
            // constant -
            //   level=128 = max
            //   when level<32, fill first 6th of each end of the strip
            //   at 96, fill all of strip
            int startPos = NUM_LEDS / 12;
            int endPos = NUM_LEDS / 12;
            if (level > 32 && level < 96)
            {
                endPos = (level - 32) * ((NUM_LEDS / 2) - endPos);
                endPos >>= 6; // divide by 64
            }
            else if (level >= 96)
            {
                endPos = NUM_LEDS / 2;
            }

            int multipliedR = r * level;
            int multipliedG = g * level;
            int multipliedB = b * level;

            for (int c = 0; c < startPos; c++)
            {
                light_buffer[c * 3] = (multipliedR >> 7);     // divide by 128
                light_buffer[c * 3 + 1] = (multipliedG >> 7); // divide by 128
                light_buffer[c * 3 + 2] = (multipliedB >> 7); // divide by 128
            }
            for (int c = startPos; c < endPos; c++)
            {
                int thisLevel = (level * (endPos - c)) / (endPos - startPos);
                light_buffer[c * 3] = (r * thisLevel) >> 8;
                light_buffer[c * 3 + 1] = (g * thisLevel) >> 8;
                light_buffer[c * 3 + 2] = (b * thisLevel) >> 8;
            }
            for (int c = endPos; c < NUM_LEDS / 2; c++)
            {
                light_buffer[c * 3] = 0;
                light_buffer[c * 3 + 1] = 0;
                light_buffer[c * 3 + 2] = 0;
            }
            // other side is symmetrical
            for (int c = 0, d = NUM_LEDS - 1; d >= NUM_LEDS / 2; d--, c++)
            {
                light_buffer[d * 3] = light_buffer[c * 3];
                light_buffer[d * 3 + 1] = light_buffer[c * 3 + 1];
                light_buffer[d * 3 + 2] = light_buffer[c * 3 + 2];
            }
            updateRawBuffer();
        }
    }

  protected:
    inline CRGB applyGamma(CRGB val)
    {
        val.r = GAMMA_TABLE[val.r];
        val.g = GAMMA_TABLE[val.g];
        val.b = GAMMA_TABLE[val.b];
        return val;
    }

    void updateRawBuffer()
    {
        // LEDs are in 2 strips, split into 3 bars each
        CRGB *barStarts[3] = {
            &strip1Buffer[STRIP1_SPLIT_POINT / 2], // from half way along 1st bar
            &strip2Buffer[0],                      // all of strip2 reversed
            &strip1Buffer[0],                      // 1st half of last bar, reversed
        };
        int barCounts[3] = {NUM_LED1 - (STRIP1_SPLIT_POINT / 2), -NUM_LED2,
                            -(STRIP1_SPLIT_POINT / 2)};

        int inPos = 0;
        for (int c = 0; c < 3; c++)
        {
            CRGB *outBuf = barStarts[c];
            int count = barCounts[c];
            if (count < 0)
            {
                // from start+count-1 backwards
                count = -count;
                for (int pos = count - 1; pos >= 0; pos--)
                {
                    outBuf[pos] = applyGamma(CRGB(light_buffer[inPos], light_buffer[inPos + 1],
                                                  light_buffer[inPos + 2]));
                    inPos += 3;
                }
            }
            else
            {
                // from start to start+count
                for (int pos = 0; pos < count; pos++)
                {
                    outBuf[pos] = applyGamma(CRGB(light_buffer[inPos], light_buffer[inPos + 1],
                                                  light_buffer[inPos + 2]));
                    inPos += 3;
                }
            }
        }
        // show the LEDs
        FastLED.show();
    }
    EFFECT_TYPE currentEffect;
    // the logical light buffer (ordered by logical position round the
    // circle)
    uint8_t light_buffer[3 * NUM_LEDS];

    // fastLED light buffer ordered by electrical position on the two strings
    // and gamma corrected
    CRGB strip1Buffer[NUM_LED1];
    CRGB strip2Buffer[NUM_LED2];

    inline static LightOutput* singleton;
};