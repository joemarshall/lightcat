#pragma once

#include <FastLED.h>

class LightOutput
{
  public:
    bool hasLights;

    static const int HEAD_BAR_LEDS=58;
    static const int SIDE_ROOM_LEDS=116;

    static const int SIDE_WALL_LEDS=116;
    static const int TAIL_BAR_LEDS=60;

    static const int NUM_LED1 = HEAD_BAR_LEDS+SIDE_ROOM_LEDS;
    static const int NUM_LED2 = SIDE_WALL_LEDS+TAIL_BAR_LEDS;

    // LED count of first strip of
    // each one - STRIP1 = short then long
    // STRIP2 = long then short
    static const int STRIP1_SPLIT_POINT = HEAD_BAR_LEDS;
    static const int STRIP2_SPLIT_POINT = SIDE_WALL_LEDS;
    static const int NUM_LEDS = (NUM_LED1 + NUM_LED2);
    static const int STRIP1_PIN = 32;
    static const int STRIP2_PIN = 33;

    // points where each section of the real LED bars exists inside
    // the array
    // - used for setting LEDS as 6 separate bars
    static const int point1 = NUM_LEDS - STRIP1_SPLIT_POINT / 2;
    static const int point2 = (point1 + STRIP1_SPLIT_POINT) % NUM_LEDS;
    static const int point3 = (point2 + (NUM_LED1 - STRIP1_SPLIT_POINT) / 2) % NUM_LEDS;
    static const int point4 = (NUM_LED1 + point1) % NUM_LEDS;
    static const int point5 = (point4 + (NUM_LED2 - STRIP2_SPLIT_POINT)) % NUM_LEDS;
    static const int point6 = (point5 + STRIP2_SPLIT_POINT / 2) % NUM_LEDS;
    static const int point7 = (point1 % NUM_LEDS);
    static constexpr int points[7] = {point1, point2, point3, point4, point5, point6, point7};

    void init(bool inHasLights)
    {
        nextBlockPos = 0;
        hasLights = inHasLights;
//        if (hasLights)
        {
            FastLED.addLeds<WS2813, STRIP1_PIN, GRB>(strip1Buffer, NUM_LED1);
            FastLED.addLeds<WS2813, STRIP2_PIN, GRB>(strip2Buffer, NUM_LED2);
        }
        singleton = this;
        globalFade=255;
    }

    void setGlobalFade(int val)
    {
        globalFade=val;
        updateRawBuffer();
    }

    int getGlobalFade(){
        return globalFade;
    }

    static LightOutput *GetLightOutput()
    {
        return singleton;
    }

    // Gamma brightness lookup table <https://victornpb.github.io/gamma-table-generator>
    // gamma = 2.80 steps = 256 range = 0-70
    // n.b. this also dims the LEDs to approx 50% of full brightness (1/3 of full current)
    const uint8_t GAMMA_TABLE[256] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,
        2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
        4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,
        7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  10, 10, 10, 10, 10, 11, 11,
        11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17,
        17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 24, 24, 24,
        25, 25, 26, 26, 26, 27, 27, 28, 28, 29, 29, 29, 30, 30, 31, 31, 32, 32, 33, 33, 34, 34,
        34, 35, 35, 36, 36, 37, 37, 38, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 45, 45, 46,
        46, 47, 47, 48, 49, 49, 50, 51, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 58, 58, 59, 60,
        60, 61, 62, 63, 63, 64, 65, 65, 66, 67, 68, 68, 69, 70,
    };

    enum EffectType
    {
        EFFECT_CONSTANT, // constant colour and brightness
        EFFECT_SPIN,     // colour and brightness cycle round all LEDs
        EFFECT_SWIRL,    // colour and light zoom out in both directions on the LEDs
        EFFECT_BLOCKS,   // colour and light on blocks - each colour sets another block
    };
    void onMultipleColours(std::array<int, 6> &h, std::array<int, 6> &s, std::array<int, 6> &v)
    {
        if (currentEffect == EFFECT_BLOCKS)
        {
            // set each block individually
            int count = 0;
            for (int b = 0; b < 6; b++)
            {
                for (int c = points[b]; c != points[b + 1]; c = ((c + 1) % NUM_LEDS))
                {
                    light_buffer[c] = CHSV(h[b], s[b], v[b]);
                    count++;
                }
            }
        }
        else if (currentEffect == EFFECT_SWIRL)
        {
            // inject into the swirl
            // first one goes to position 0
            const int offset1 = STRIP1_SPLIT_POINT / 2 + 1;
            const int offset2 = offset1 + (NUM_LED1 - STRIP1_SPLIT_POINT) / 2;
            const int inject_points[6] = {0,       NUM_LEDS - 1,      offset1, NUM_LEDS - offset1,
                                          offset2, NUM_LEDS - offset2};
            for (int c = 0; c < 6; c++)
            {
                // only inject if there is a loud enough signal
                if (v[c] > 30)
                {
                    light_buffer[inject_points[c]] = CHSV(h[c], s[c], v[c]);
                }
            }
        }
        updateRawBuffer();
    }

    // lerp a 16 bit number by a 0-256 factor
    static inline int lerp(int start,int end,int factor)
    {
        if(factor>256)factor=256;
        int32_t scaled = start *(256-factor) + end*factor;
        return scaled>>8;
    }

    // set current colour with level==-1 if no colour selected
    // for spin/whirl, where it will output blank space
    void onColour(int h, int s, int v, bool updateBuffer = true, bool onTouch = false)
    {
        lastH = h;
        lastS = s;
        lastV = v;
        if (!updateBuffer)
        {
            return;
        }
        CHSV hsv = CHSV(h, s, v);
        if (currentEffect == EFFECT_CONSTANT && v != -1)
        {
            // constant -
            //   level=255 = max
            //   when level<128, fill first 6th of each end of the strip
            //   at 170, fade from first 6th to end
            //   at 213, fill all of strip
            int fillAmount = (v-128)*3;
            if(fillAmount<0)fillAmount=0;
            if(fillAmount>255)fillAmount=255;
            int32_t startFadePos = lerp((NUM_LEDS/12),(NUM_LEDS/2),fillAmount);
            int32_t endFadePos = lerp((NUM_LEDS/12),(NUM_LEDS/2),fillAmount*2);
            Serial.print(v);
            Serial.print(":");
            Serial.print(startFadePos);
            Serial.print(",");
            Serial.println(endFadePos);
            for (int c = 0; c < startFadePos; c++)
            {
                light_buffer[c].h = h;
                light_buffer[c].s = s;
                light_buffer[c].v = v;
            }
            for (int c = startFadePos; c < endFadePos; c++)
            {
                int thisV = (v * (endFadePos - c)) / (endFadePos - startFadePos);
                light_buffer[c].h = h;
                light_buffer[c].s = s;
                light_buffer[c].v = thisV;
            }
            for (int c = endFadePos; c < NUM_LEDS / 2; c++)
            {
                light_buffer[c].h = 0;
                light_buffer[c].s = 0;
                light_buffer[c].v = 0;
            }
            // other side is symmetrical
            for (int c = 0, d = NUM_LEDS - 1; d >= NUM_LEDS / 2; d--, c++)
            {
                light_buffer[d] = light_buffer[c];
            }
        }
        else if (currentEffect == EFFECT_SPIN || currentEffect == EFFECT_SWIRL)
        {
            // set the first value to the current input colour
            light_buffer[0].h = h;
            light_buffer[0].s = s;
            light_buffer[0].v = v;
        }
        else if (currentEffect == EFFECT_BLOCKS)
        {
            if (onTouch)
            {
                nextBlockPos++;
            }
            if (nextBlockPos >= 6)
                nextBlockPos = 0;
            // change colour of only the selected block
            // but change value of everything
            for (int c = points[nextBlockPos]; c != points[nextBlockPos + 1];
                 c = ((c + 1) % NUM_LEDS))
            {
                light_buffer[c].h = h;
                light_buffer[c].s = s;
            }
            for (int c = 0; c < NUM_LEDS; c++)
            {
                light_buffer[c].v = v;
            }
        }

        updateRawBuffer();
    }

    void setEffect(EffectType effect)
    {
        currentEffect = effect;
    }

    inline uint8_t fadeAtEnd(uint8_t value)
    {
        uint32_t newVal = (((uint32_t)value) * 3) >> 2;
        return (uint8_t)newVal;
    }

    void scroll(bool fade)
    {
        // by default we don't add the colour at the end
        // if we are fading (because we get it from somewhere else in
        // looping modes)
        bool addColour = !fade;
        switch (currentEffect)
        {
        case EFFECT_CONSTANT:
            if (fade)
            {
                if (lastV > 0)
                {
                    lastV--;
                }
            }
            addColour = true;
            break;
        case EFFECT_BLOCKS:
            // only switch block colours on touch
            // but do update for everything at once

            break;
        case EFFECT_SPIN:
            if (fade)
            {
                // no current colour, loop colours round
                CHSV endCol = light_buffer[NUM_LEDS - 1];
                memmove(&light_buffer[1], &light_buffer[0], (NUM_LEDS - 1) * sizeof(CHSV));
                endCol.v = fadeAtEnd(endCol.v);
                light_buffer[0] = endCol;
                // don't update colour, because we've spun instead
            }
            else
            {
                // spin colours, and then add the current colour below
                memmove(&light_buffer[1], &light_buffer[0], (NUM_LEDS - 1) * sizeof(CHSV));
            }
            break;
        case EFFECT_SWIRL:
            // whirling effect - goes down each side to meet at the end
            int break_point = NUM_LEDS >> 1;
            CHSV prev_0,prev_1;
            if (fade)
            {
                // copy last value from one side to first value
                // on other side
                prev_0=light_buffer[break_point];
                prev_0.v=fadeAtEnd(prev_0.v);
                prev_1=light_buffer[break_point - 1];
                prev_1.v=fadeAtEnd(prev_1.v);
            }

            // scroll one side forwards
            memmove(&light_buffer[1], &light_buffer[0], sizeof(CHSV) * (break_point - 1));
            // and the other side backwards
            memmove(&light_buffer[break_point], &light_buffer[break_point + 1],
                    sizeof(CHSV) * (NUM_LEDS - break_point - 1));

            if(fade){
                // copy end points into the start of the LED array
                light_buffer[0]=prev_0;
                light_buffer[NUM_LEDS - 1] = prev_1;
            }

            break;
        }
        if (addColour)
        {
            //            Serial.println("!");
            onColour(lastH, lastS, lastV);
        }
        else
        {
            updateRawBuffer();
        }
    }

    CRGB *getStripBuffer(uint32_t stripNum, int *count, int *splitPoint)
    {
        if (stripNum == 0)
        {
            *count = NUM_LED1;
            *splitPoint = STRIP1_SPLIT_POINT;
            return strip1Buffer;
        }
        else if (stripNum == 1)
        {
            *count = NUM_LED2;
            *splitPoint = STRIP2_SPLIT_POINT;
            return strip2Buffer;
        }
        return NULL;
    }

  protected:
    inline CRGB applyGamma(CRGB val)
    {
        if(globalFade<255)
        {
            val.fadeLightBy(255-globalFade);
        }
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
            &strip1Buffer[0],                      // 1st half of last bar, correct way
        };
        int barCounts[3] = {NUM_LED1 - (STRIP1_SPLIT_POINT / 2), -NUM_LED2,
                            (STRIP1_SPLIT_POINT / 2)};

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
                    hsv2rgb_rainbow(light_buffer[inPos], outBuf[pos]);
                    outBuf[pos] = applyGamma(outBuf[pos]);
                    inPos++;
                }
            }
            else
            {
                // from start to start+count
                for (int pos = 0; pos < count; pos++)
                {
                    hsv2rgb_rainbow(light_buffer[inPos], outBuf[pos]);
                    outBuf[pos] = applyGamma(outBuf[pos]);
                    inPos++;
                }
            }
        }
        // Serial.println("Light ordering");
        // for (int c = 0; c < NUM_LED1; c++)
        // {
        //     Serial.print("1:");
        //     Serial.print(c);
        //     Serial.print(":");
        //     Serial.println(strip1Buffer[c].r | (strip1Buffer[c].g) << 8);
        // }
        // for (int c = 0; c < NUM_LED2; c++)
        // {
        //     Serial.print("2:");
        //     Serial.print(c);
        //     Serial.print(":");
        //     Serial.println(strip2Buffer[c].r | (strip2Buffer[c].g) << 8);
        // }
        // show the LEDs
//        if (hasLights)
        {
            FastLED.show();
        }
    }

    uint8_t lastH, lastS, lastV;
    EffectType currentEffect;
    // the logical light buffer (ordered by logical position round the
    // circle), as linear HSV
    CHSV light_buffer[NUM_LEDS];

    // fastLED light buffer ordered by electrical position on the two strings
    // and gamma corrected
    CRGB strip1Buffer[NUM_LED1];
    CRGB strip2Buffer[NUM_LED2];

    inline static LightOutput *singleton;

    int nextBlockPos;
    int globalFade;
};