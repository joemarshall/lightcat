#include <Arduino.h>
#include <M5Unified.h>

#include <array>

#define LV_CONF_INCLUDE_SIMPLE
#include <esp_timer.h>
#include <lvgl.h>

#include "colour_screen.h"
#include "disco_screen.h"
#include "light_output.h"
#include "sound_input.h"
#include "standby_screen.h"

std::array<Screen *, 3> screens;

constexpr int32_t HOR_RES = 320;
constexpr int32_t VER_RES = 240;

lv_display_t *display;
lv_indev_t *indev;

CRGB leftSideDebug[240];
CRGB rightSideDebug[240];
CRGB topSideDebug[320];
CRGB bottomSideDebug[320];

CRGB unfade(CRGB &inVal)
{
    return CRGB(inVal.r * 3, inVal.g * 3, inVal.b * 3);
}

void drawDebugLights()
{
    LightOutput *lo = LightOutput::GetLightOutput();
    int splitPoint = 0;
    int ledNum = 0;
    CRGB *buffer = lo->getStripBuffer(0, &ledNum, &splitPoint);
    int x = 0;
    int y = 100;
    int w = splitPoint;
    int h = ledNum - splitPoint;

    // first one goes top of screen to left
    for (int c = 0; c < splitPoint; c++)
    {
        topSideDebug[splitPoint - c - 1] = unfade(buffer[c]);
    }
    for (int c = splitPoint; c < ledNum; c++)
    {
        leftSideDebug[c - splitPoint] = unfade(buffer[c]);
    }
    M5.Display.pushImage<lgfx::v1::bgr888_t>(x, y, splitPoint, 1,
                                             (lgfx::v1::bgr888_t *)topSideDebug);
    M5.Display.pushImage<lgfx::v1::bgr888_t>(x + 1, y, 1, ledNum - splitPoint,
                                             (lgfx::v1::bgr888_t *)leftSideDebug);

    buffer = lo->getStripBuffer(1, &ledNum, &splitPoint);

    for (int c = 0; c < splitPoint; c++)
    {
        rightSideDebug[c] = unfade(buffer[c]);
    }
    for (int c = splitPoint; c < ledNum; c++)
    {
        bottomSideDebug[ledNum - c - 1] = unfade(buffer[c]);
    }
    M5.Display.pushImage<lgfx::v1::bgr888_t>(x + w, y, 1, splitPoint,
                                             (lgfx::v1::bgr888_t *)rightSideDebug);
    M5.Display.pushImage<lgfx::v1::bgr888_t>(x, y + h, ledNum - splitPoint, 1,
                                             (lgfx::v1::bgr888_t *)bottomSideDebug);
}

void my_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lv_draw_sw_rgb565_swap(px_map, w * h);
    M5.Display.pushImageDMA<uint16_t>(area->x1, area->y1, w, h, (uint16_t *)px_map);
    if(!M5.Power.Axp192.isVBUS()){
        drawDebugLights();
    }
    lv_disp_flush_ready(disp);
}

uint32_t my_tick_function()
{
    return (esp_timer_get_time() / 1000LL);
}

int lastX = -1;
int lastY = -1;

void my_touchpad_read(lv_indev_t *drv, lv_indev_data_t *data)
{
    M5.update();
    auto count = M5.Touch.getCount();

    if (count == 0)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        lastX = -1;
        lastY = -1;
    }
    else
    {
        auto touch = M5.Touch.getDetail(0);
        if (touch.y >= VER_RES)
        {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = touch.x;
            data->point.y = VER_RES - 1;
        }
        else
        {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = touch.x;
            data->point.y = touch.y;
        }
        lastX = data->point.x;
        lastY = data->point.y;
    }
    for (int c = 0; c < screens.size(); c++)
    {
        screens[c]->setTouchPos(lastX, lastY);
    }
}

void my_log_cb(lv_log_level_t level, const char *buf)
{
    Serial.write(buf);
}

void init_screens()
{
    //  screens[0]->setCurrent(false);
    screens[0] = new StandbyScreen();
    screens[1] = new ColourScreen();
    screens[2] = new DiscoScreen();
    screens[0]->setNext(screens[1]);
    screens[1]->setPrev(screens[0]);
    screens[1]->setNext(screens[2]);
    screens[2]->setPrev(screens[1]);
    screens[2]->setNext(screens[1]);
    screens[0]->setCurrent(false);
}

SoundInput sound;
LightOutput light;
TickType_t xLastWakeTime;

// continue setup code
void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("Startup");

    m5::M5Unified::config_t cfg;
    cfg.pmic_button = true;
    cfg.output_power = false;
    M5.begin(cfg);
    Serial.print("Has lights:");
    bool hasLights = M5.Power.Axp192.isVBUS() == 1;
    Serial.println(hasLights);
    lv_init();
    lv_log_register_print_cb(my_log_cb);

    lv_tick_set_cb(my_tick_function);

    display = lv_display_create(HOR_RES, VER_RES);
    lv_display_set_flush_cb(display, my_display_flush);

    static lv_color_t buf1[HOR_RES * 15] __attribute__((aligned(4)));
    lv_display_set_buffers(display, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);

    lv_indev_set_read_cb(indev, my_touchpad_read);

    init_screens();
    sound.init();
    light.init(hasLights);
    xLastWakeTime = xTaskGetTickCount ();
}

bool switchedOff = false;

void loop()
{
    lv_task_handler();
    vTaskDelayUntil(&xLastWakeTime,1);
    sound.doProcessing();
    if (!light.hasLights)
    {
        if(!M5.Power.Axp192.isVBUS()){
            drawDebugLights();
        }
    }
    if (M5.Power.getKeyState() == 2)
    {
        switchedOff = !switchedOff;
        if (switchedOff)
        {
            screens[0]->setCurrent(false, 0);
        }
        else
        {
            screens[1]->setCurrent(false, 0);
        }
    }
    if (switchedOff)
    {
        int fade = light.getGlobalFade();
        if (fade > 0)
        {
            light.setGlobalFade(fade - 1);
            screens[0]->tickTimer(-1);
        }
    }
    else
    {
        int fade = light.getGlobalFade();
        if (fade < 255)
        {
            fade = fade + 3;
            if (fade > 255)
                fade = 255;
            light.setGlobalFade(fade);
            screens[0]->tickTimer(-1);
        }
    }
}