#pragma once

#define NUM_LEDS 60 * 6

class LightOutput
{

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
            for (int c = endPos; c < NUM_LEDS/2; c++)
            {
                light_buffer[c * 3] = 0;
                light_buffer[c * 3 + 1] = 0;
                light_buffer[c * 3 + 2] = 0;
            }
            // other end is symmetrical
            for(int c=0,int d=NUM_LEDS-1;d>=NUM_LEDS/2;d--,c++){
                light_buffer[d*3] = light_buffer[c*3];
                light_buffer[d*3+1] = light_buffer[c*3+1];
                light_buffer[d*3+2] = light_buffer[c*3+2];
            }
            updateOutputBuffer();
        }
    }

  protected:
    void updateRawBuffer()
    {
        // LEDs are in 12 parts logically
        int barStarts[6]={
            NUM_LEDS/12, 1 = 2nd half of first bar
            2* NUM_LEDS/12, 2 = 2nd and 3rd bars
            11 * NUM_LEDS/12, // 2nd half of last bar, reversed
            10*NUM_LEDS/12, // 1st half of last bar, reversed
            8 * NUM_LEDS/12, // 4th and 5th bars, reversed
            0, // 1st half of 1st bar, not reversed
        };
        int barCounts[6]={
            NUM_LEDS/12,4*NUM_LEDS/12,-NUM_LEDS/12,-NUM_LEDS/12,4*-NUM_LEDS/12,NUM_LEDS/12
        };

        // TODO: gamma correct here
        int inPos=0;
        for(int c=0;c<6;c++){
            int start =barStarts[c];
            int count = barCounts[c];
            if(count<0){
                // from start+count-1 backwards
                count=-count;
                for(int pos=start+count-1;pos>=start;pos--){
                    raw_light_buffer[pos*3]=light_buffer[inPos*3];
                    raw_light_buffer[pos*3+1]=light_buffer[inPos*3+1];
                    raw_light_buffer[pos*3+2]=light_buffer[inPos*3+2];
                    inPos++;
                }
            }else{
                // from start to start+count
                for(int pos=start;pos<start+count;pos++){
                    raw_light_buffer[pos*3]=light_buffer[inPos*3];
                    raw_light_buffer[pos*3+1]=light_buffer[inPos*3+1];
                    raw_light_buffer[pos*3+2]=light_buffer[inPos*3+2];
                    inPos++;
                }
            }
        }

    }
    EFFECT_TYPE currentEffect;
    // the logical light buffer (ordered by logical position round the
    // circle)
    uint8_t light_buffer[3 * NUM_LEDS];

    // light buffer ordered by electrical position on the two strings
    // and gamma corrected
    uint8_t raw_light_buffer[3 * NUM_LEDS];
};