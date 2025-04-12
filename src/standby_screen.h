#pragma once
#include "screen.h"
#include <M5Unified.hpp>

#include "light_output.h"

#define FULL_BRIGHTNESS 30

class StandbyScreen : public Screen
{
    public:
    StandbyScreen()
    {
        currentBrightness=FULL_BRIGHTNESS+20;
        core2Brightness(currentBrightness);
        LV_IMAGE_DECLARE(standby);
        background = lv_imgbtn_create(screen);
        lv_imgbtn_set_src(background, LV_IMGBTN_STATE_RELEASED, NULL, &standby, NULL);
        lv_imgbtn_set_src(background, LV_IMGBTN_STATE_PRESSED, NULL, &standby, NULL);
        lv_obj_center(background);
        handleEvent(background, LV_EVENT_ALL);
        standby_timer = enableTimer(1000);
    }


    virtual void onEvent(lv_event_t *e, lv_obj_t *target, lv_event_code_t code) 
    {
        int fadeAmount=LightOutput::GetLightOutput()->getGlobalFade();
        if(code==LV_EVENT_PRESSED && fadeAmount==255){
            currentBrightness=FULL_BRIGHTNESS;
            core2Brightness(currentBrightness);
            if(nextScreen!=NULL){
                nextScreen->setCurrent(true);
            }
        }
    }

    protected:

    virtual void onTimer(int id)
    {
        
        int fadeAmount=LightOutput::GetLightOutput()->getGlobalFade();
        if(fadeAmount!=255){
            currentBrightness = (fadeAmount * FULL_BRIGHTNESS)>>8;
        }else{
            if(id==standby_timer){
                if(currentBrightness>0){
                    currentBrightness-=1;
                }
            }
        }
        core2Brightness(currentBrightness);
    }

  
    void core2Brightness(uint8_t lvl) {
        // The backlight brightness is in steps of 25 in AXP192.cpp
        // calculation in SetDCVoltage: ( (voltage - 700) / 25 )
        // 2325 is the minimum "I can just about see a glow in a dark room" level of brightness.
        // 3100 is roughly what the default startup brightness is
        if(lvl>FULL_BRIGHTNESS)lvl=FULL_BRIGHTNESS;
        if(lvl<3)lvl=0; // ignore too low brightness levels
        int v = lvl * 25 + 2300;
      
        // Clamp to safe values.
        if (v < 2300) v = 2300;
        if (v > FULL_BRIGHTNESS*25 + 2300) v = FULL_BRIGHTNESS*25 + 2300; 
      
      
        if(v==2300){
            v=0;
        }
        // Set the LCD backlight voltage. 
        M5.Power.Axp192.setDCDC3(v);
      }
  
    lv_obj_t* background;
    Screen *mNextScreen;

    int currentBrightness;
    int standby_timer;


};