#pragma once
#include "screen.h"

class StandbyScreen : public Screen
{
    public:
    StandbyScreen(Screen* nextScreen):mNextScreen(nextScreen)
    {
        LV_IMAGE_DECLARE(standby);
        background = lv_image_create(screen);
        lv_obj_center(background);
        HandleEvent(background, LV_EVENT_ALL);
    }


    virtual void OnEvent(lv_event_t *e, lv_obj_t *target, lv_event_code_t code) 
    {
        if(code==LV_EVENT_PRESSED){
            mNextScreen->SetCurrent(true);
        }
    }

    protected:
    lv_obj_t* background;
    Screen *mNextScreen;


};