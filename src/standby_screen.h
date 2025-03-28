#pragma once
#include "screen.h"
#include <M5Unified.hpp>

#define FULL_BRIGHTNESS 30

class StandbyScreen : public Screen
{
    public:
    StandbyScreen()
    {
        currentBrightness=FULL_BRIGHTNESS+2;
        Core2Brightness(currentBrightness);
        LV_IMAGE_DECLARE(standby);
        background = lv_imgbtn_create(screen);
        lv_imgbtn_set_src(background, LV_IMGBTN_STATE_RELEASED, NULL, &standby, NULL);
        lv_imgbtn_set_src(background, LV_IMGBTN_STATE_PRESSED, NULL, &standby, NULL);
        lv_obj_center(background);
        HandleEvent(background, LV_EVENT_ALL);
        enableTimer(1000);
    }


    virtual void OnEvent(lv_event_t *e, lv_obj_t *target, lv_event_code_t code) 
    {
        if(code==LV_EVENT_PRESSED){
            currentBrightness=FULL_BRIGHTNESS;
            Core2Brightness(currentBrightness);
            if(nextScreen!=NULL){
                nextScreen->SetCurrent(true);
            }
        }
    }

    protected:

    virtual void onTimer(int id)
    {
        if(currentBrightness>0){
            currentBrightness-=1;
        }
        Core2Brightness(currentBrightness);
    }

  
    void Core2Brightness(uint8_t lvl) {
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


};