#pragma once

#include <FastLED.h>

#define TEST_NO_LIGHTS

class LightOutput
{
  public:
#ifdef TEST_NO_LIGHTS
    static const int NUM_LED1 = 180;
    static const int NUM_LED2 = 180;

    // LED count of first strip of
    // each one - STRIP1 = short then long
    // STRIP2 = long then short
    static const int STRIP1_SPLIT_POINT = 60;
    static const int STRIP2_SPLIT_POINT = 120;
#else
    static const int NUM_LED1 = 60 * 3;
    static const int NUM_LED2 = 60 * 3;
    // LED count of first strip of
    // each one - STRIP1 = short then long
    // STRIP2 = long then short
    static const int STRIP1_SPLIT_POINT = 60;
    static const int STRIP2_SPLIT_POINT = 120;
#endif
    static const int NUM_LEDS = (NUM_LED1 + NUM_LED2);
    static const int STRIP1_PIN = 32;
    static const int STRIP2_PIN = 33;

    static const int point1 = NUM_LEDS-STRIP1_SPLIT_POINT/2;
    static const int point2 = (point1+STRIP1_SPLIT_POINT)%NUM_LEDS;
    static const int point3 = (point2+ (NUM_LED1 - STRIP1_SPLIT_POINT)/2)%NUM_LEDS;
    static const int point4 = (NUM_LED1+point1)%NUM_LEDS;
    static const int point5 = (point4 + (NUM_LED2-STRIP2_SPLIT_POINT))%NUM_LEDS; 
    static const int point6 = (point5 + STRIP2_SPLIT_POINT/2)%NUM_LEDS;
    static const int point7 = (point1%NUM_LEDS);
    static constexpr int points[7] = {point1,point2,point3,point4,point5,point6,point7};



    void init()
    {
        nextBlockPos=0;
#ifndef TEST_NO_LIGHTS
        FastLED.addLeds<WS2813, STRIP1_PIN>(strip1Buffer, NUM_LED1);
        FastLED.addLeds<WS2813, STRIP2_PIN>(strip2Buffer, NUM_LED2);
#endif
        singleton = this;
    }

    static LightOutput *GetLightOutput()
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

    enum EffectType
    {
        EFFECT_CONSTANT, // constant colour and brightness
        EFFECT_SPIN,     // colour and brightness cycle round all LEDs
        EFFECT_SWIRL,    // colour and light zoom out in both directions on the LEDs
        EFFECT_BLOCKS,   // colour and light on blocks - each colour sets another block
    };
    void onMultipleColours(std::array<int,6> &h,std::array<int,6> &s,std::array<int,6> &v)
    {
        if(currentEffect== EFFECT_BLOCKS){
            // set each block individually
            int count=0;
            for(int b=0;b<6;b++){
                for(int c=points[b];c!=points[b+1];c=((c+1)%NUM_LEDS))
                {
                    light_buffer[c]=CHSV(h[b],s[b],v[b]);
                    count++;
                }
            }
        }else if(currentEffect==EFFECT_SWIRL){
            // inject into the swirl
            // first one goes to position 0
            int offset1 = STRIP1_SPLIT_POINT/2 +1;
            int offset2 = offset1 + (NUM_LED1- STRIP1_SPLIT_POINT)/2;
            const int inject_points[6] = {0,NUM_LEDS-1,offset1,NUM_LEDS-offset1,offset2,NUM_LEDS-offset2};
            for(int c=0;c<6;c++){
                // only inject if there is a loud enough signal
                if(v[c]>30){
                    light_buffer[inject_points[c]]=CHSV(h[c],s[c],v[c]);
                }
            }
        }
        updateRawBuffer();
    }
    // set current colour with level==-1 if no colour selected
    // for spin/whirl, where it will output blank space
    void onColour(int h, int s, int v,bool updateBuffer=true,bool onTouch=false)
    {
        lastH = h;
        lastS = s;
        lastV = v;
        if(!updateBuffer){
            return;
        }
        CHSV hsv = CHSV(h, s, v);
        if (currentEffect == EFFECT_CONSTANT && v != -1)
        {
            // constant -
            //   level=128 = max
            //   when level<32, fill first 6th of each end of the strip
            //   at 96, fill all of strip
            int startPos = NUM_LEDS / 12;
            int endPos = NUM_LEDS / 12;
            if (v > 32 && v < 96)
            {
                endPos = (v - 32) * ((NUM_LEDS / 2) - endPos);
                endPos >>= 6; // divide by 64
            }
            else if (v >= 96)
            {
                endPos = NUM_LEDS / 2;
            }

            for (int c = 0; c < startPos; c++)
            {
                light_buffer[c].h = h;
                light_buffer[c].s = s;
                light_buffer[c].v = v;
            }
            for (int c = startPos; c < endPos; c++)
            {
                int thisV = (v * (endPos - c)) / (endPos - startPos);
                light_buffer[c].h = h;
                light_buffer[c].s = s;
                light_buffer[c].v = thisV;
            }
            for (int c = endPos; c < NUM_LEDS / 2; c++)
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
        }else if (currentEffect == EFFECT_SPIN || currentEffect == EFFECT_SWIRL)
        {
            // set the first value to the current input colour
            light_buffer[0].h = h;
            light_buffer[0].s = s;
            light_buffer[0].v = v;
        }else if(currentEffect== EFFECT_BLOCKS){
            if(onTouch){
                nextBlockPos++;
            }
            if(nextBlockPos>=6)nextBlockPos=0;
            // change colour of only the selected block
            // but change value of everything
            for(int c=points[nextBlockPos];c!=points[nextBlockPos+1];c=((c+1)%NUM_LEDS))
            {
                light_buffer[c].h=h;
                light_buffer[c].s=s;
            }
            for(int c=0;c<NUM_LEDS;c++){
                light_buffer[c].v=v;
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
                endCol.v=fadeAtEnd(endCol.v);
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

            if (fade)
            {
                // copy last value to first value
                light_buffer[0] = light_buffer[break_point - 1];
                light_buffer[0].v = fadeAtEnd(light_buffer[0].v);
            }
            // now make sure first pixel of both sides is the same
            light_buffer[NUM_LEDS - 1] = light_buffer[0];

            // scroll one side forwards
            memmove(&light_buffer[1], &light_buffer[0], sizeof(CHSV) * (break_point - 1));
            // and the other side backwards
            memmove(&light_buffer[break_point], &light_buffer[break_point + 1],
                    sizeof(CHSV) * (NUM_LEDS - break_point - 1));
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

    CRGB* getStripBuffer(uint32_t stripNum,int *count)
    {
        if(stripNum==0){
            *count=NUM_LED1;
            return strip1Buffer;
        }else if(stripNum==1){
            *count=NUM_LED2;
            return strip2Buffer;
        }
        return NULL;
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
#ifdef TEST_NO_LIGHTS
        for (int c = 0; c < NUM_LED1; c++)
        {        
            strip1Buffer[c]=light_buffer[c];
        }
        for (int c = NUM_LED1; c < NUM_LEDS; c++)
        {         
            strip2Buffer[c-NUM_LED1]=light_buffer[c];
        }
        return;
#endif

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
        // show the LEDs
        FastLED.show();
    }

    uint8_t lastH, lastS, lastV;
    EffectType currentEffect;
    // the logical light buffer (ordered by logical position round the
    // circle)
    CHSV light_buffer[NUM_LEDS];

    // fastLED light buffer ordered by electrical position on the two strings
    // and gamma corrected
    CRGB strip1Buffer[NUM_LED1];
    CRGB strip2Buffer[NUM_LED2];

    inline static LightOutput *singleton;

    int nextBlockPos;
};