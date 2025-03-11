#pragma once

class Screen
{
    public:
    Screen(lv_obj_t* parent=NULL,lv_obj_t* customScreen=NULL)
    {
        lastX=-1;
        lastY=-1;
        if(!customScreen){
            screen= lv_obj_create(parent);
        }else{
            screen=customScreen;
        }
        nextScreen=NULL;
    }

    virtual ~Screen(){}

    lv_obj_t *GetScreen(){
        return screen;
    }

    void SetNext(Screen* next)
    {
        nextScreen=next;
    }

    void SetCurrent(bool fadeThis,int fadeTime=500)
    {
        if(fadeThis)
        {
            lv_screen_load_anim(screen,LV_SCR_LOAD_ANIM_FADE_OUT,fadeTime,0,false);
        }else{
            lv_screen_load_anim(screen,LV_SCR_LOAD_ANIM_FADE_IN,fadeTime,0,false);
        }
    }

    void SetTouchPos(int x,int y)
    {
        lastX=x; lastY=y;
    }

protected:
    virtual void OnEvent(lv_event_t *e,lv_obj_t * target,lv_event_code_t code){
        // do nothing default
        Serial.println("Fall through event fn called");
    }

    virtual void HandleEvent(lv_obj_t*obj,lv_event_code_t filter)
    {
        lv_obj_add_event_cb(obj,Screen::StaticOnEvent,filter,this);
    }

    static void StaticOnEvent(lv_event_t *e){
        Screen* pThis = reinterpret_cast<Screen *>(lv_event_get_user_data(e));
        lv_obj_t*target = (lv_obj_t*)lv_event_get_current_target(e);
        lv_event_code_t code = lv_event_get_code(e);
        
        pThis->OnEvent(e,target,code);
    }

    Screen* nextScreen;
    lv_obj_t* screen;
    int lastX,lastY;
};