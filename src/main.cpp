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

void drawDebugLights()
{
    #ifdef TEST_NO_LIGHTS
    LightOutput*lo = LightOutput::GetLightOutput();
    int lednum=0;
    CRGB* buffer = lo->getStripBuffer(0,&lednum);
    M5.Display.pushImage<lgfx::v1::bgr888_t>(0, 0, 1, lednum, (lgfx::v1::bgr888_t *)buffer);
    buffer = lo->getStripBuffer(1,&lednum);
    M5.Display.pushImage<lgfx::v1::bgr888_t>(319, 0, 1, lednum, (lgfx::v1::bgr888_t *)buffer);

    #endif

}

void my_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lv_draw_sw_rgb565_swap(px_map, w * h);
    M5.Display.pushImageDMA<uint16_t>(area->x1, area->y1, w, h, (uint16_t *)px_map);
    drawDebugLights();
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
        screens[c]->SetTouchPos(lastX, lastY);
    }
}

void my_log_cb(lv_log_level_t level, const char *buf)
{
    Serial.write(buf);
}

void init_screens()
{
    //  screens[0]->SetCurrent(false);
    screens[0] = new StandbyScreen();
    screens[1] = new ColourScreen();
    screens[2] = new DiscoScreen();
    screens[1]->SetNext(screens[0]);
    screens[2]->SetNext(screens[1]);
    screens[2]->SetCurrent(false);
}

SoundInput sound;
LightOutput light;

// continue setup code
void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("Test");

    M5.begin();
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
    light.init();
}

void loop()
{
    lv_task_handler();
    vTaskDelay(1);
    sound.doProcessing();
    drawDebugLights();
}