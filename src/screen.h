#pragma once

#include <vector>

class Screen
{
  public:
    Screen(lv_obj_t *parent = NULL, lv_obj_t *customScreen = NULL)
    {
        active = false;
        lastX = -1;
        lastY = -1;
        if (!customScreen)
        {
            screen = lv_obj_create(parent);
        }
        else
        {
            screen = customScreen;
        }
        nextScreen = NULL;
        prevScreen = NULL;
        lv_obj_add_event_cb(screen, Screen::staticOnEvent, LV_EVENT_SCREEN_LOADED, this);
    }

    virtual ~Screen()
    {
    }

    lv_obj_t *getScreen()
    {
        return screen;
    }

    void setNext(Screen *next)
    {
        nextScreen = next;
    }

    void setPrev(Screen *prev)
    {
        prevScreen = prev;
    }

    void setCurrent(bool fadeThis, int fadeTime = 500)
    {
        if (fadeThis)
        {
            lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_OUT, fadeTime, 0, false);
        }
        else
        {
            lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, fadeTime, 0, false);
        }
    }

    void setTouchPos(int x, int y)
    {
        lastX = x;
        lastY = y;
    }

    int enableTimer(int delay)
    {
        int id = timers.size();
        TimerInfo *t = new TimerInfo();
        t->pThis = this;
        t->timer_id = id;
        timers.push_back(t);
        lv_timer_create(onTimerStatic, delay, t);
        return id;
    }

    void tickTimer(int id)
    {
        onTimer(id);
    }

  protected:
    struct TimerInfo
    {
        Screen *pThis;
        int timer_id;
    };
    std::vector<TimerInfo *> timers;

    virtual void onTimer(int id)
    {
    }

    virtual void onScreenLoad()
    {
    }

    virtual void onEvent(lv_event_t *e, lv_obj_t *target, lv_event_code_t code)
    {
        // do nothing default
        Serial.println("Fall through event fn called");
    }

    virtual void handleEvent(lv_obj_t *obj, lv_event_code_t filter)
    {
        lv_obj_add_event_cb(obj, Screen::staticOnEvent, filter, this);
    }

    static void staticOnEvent(lv_event_t *e)
    {
        Screen *pThis = reinterpret_cast<Screen *>(lv_event_get_user_data(e));
        lv_obj_t *target = (lv_obj_t *)lv_event_get_current_target(e);
        lv_event_code_t code = lv_event_get_code(e);

        if (code == LV_EVENT_SCREEN_LOADED)
        {
            pThis->onScreenLoad();
        }
        else
        {
            pThis->onEvent(e, target, code);
        }
    }

    static void onTimerStatic(lv_timer_t *data)
    {
        TimerInfo *evt = (TimerInfo *)lv_timer_get_user_data(data);
        Screen *pThis = evt->pThis;
        if (lv_screen_active() == pThis->screen)
        {
            pThis->onTimer(evt->timer_id);
        }
    }

    Screen *prevScreen;
    Screen *nextScreen;
    lv_obj_t *screen;
    int lastX, lastY;

    bool active;
};