#pragma once

#include "screen.h"

class ColourScreen : public Screen {
public:
  ColourScreen() {
    LV_IMAGE_DECLARE(wheel);
    LV_IMAGE_DECLARE(wheel_pressed);
    LV_IMAGE_DECLARE(crosshairs);

    LV_IMAGE_DECLARE(cat_less);
    LV_IMAGE_DECLARE(cat_more);

    LV_IMAGE_DECLARE(uparrow);
    LV_IMAGE_DECLARE(downarrow);

    colour_wheel = lv_imgbtn_create(screen);

    lv_imgbtn_set_src(colour_wheel, LV_IMGBTN_STATE_RELEASED, NULL, &wheel,
                      NULL);
    lv_imgbtn_set_src(colour_wheel, LV_IMGBTN_STATE_PRESSED, NULL,
                      &wheel_pressed, NULL);
    lv_obj_center(colour_wheel);

    crosshair_img = lv_image_create(screen);
    lv_image_set_src(crosshair_img, &crosshairs);
    lv_obj_add_flag(crosshair_img, LV_OBJ_FLAG_FLOATING);

    up = lv_imgbtn_create(screen);
    lv_imgbtn_set_src(up, LV_IMGBTN_STATE_RELEASED, NULL, &uparrow, NULL);
    lv_imgbtn_set_src(up, LV_IMGBTN_STATE_PRESSED, NULL, &uparrow, NULL);
    lv_obj_align(up, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    up_img = lv_image_create(screen);
    lv_image_set_src(up_img, &cat_more);
    lv_obj_set_y(up_img, 240);
    lv_obj_set_x(up_img, 220);
    lv_obj_add_flag(up_img, LV_OBJ_FLAG_FLOATING);

    down = lv_imgbtn_create(screen);
    lv_imgbtn_set_src(down, LV_IMGBTN_STATE_RELEASED, NULL, &downarrow, NULL);
    lv_imgbtn_set_src(down, LV_IMGBTN_STATE_PRESSED, NULL, &downarrow, NULL);
    lv_obj_align(down, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    down_img = lv_image_create(screen);
    lv_obj_add_flag(down_img, LV_OBJ_FLAG_FLOATING);
    lv_image_set_src(down_img, &cat_less);
    lv_obj_set_y(down_img, 240);
    lv_obj_set_x(down_img, 0);

    HandleEvent(colour_wheel, LV_EVENT_ALL);
    HandleEvent(crosshair_img, LV_EVENT_ALL);

    HandleEvent(up, LV_EVENT_ALL);
    HandleEvent(up_img, LV_EVENT_ALL);

    HandleEvent(down, LV_EVENT_ALL);
    HandleEvent(down_img, LV_EVENT_ALL);
  }

protected:
  void OnColourwheel(lv_event_code_t code) {
    if (code == LV_EVENT_PRESSING || code == LV_EVENT_PRESSED) {

      if (lastX != -1) {
        LV_IMAGE_DECLARE(wheel);

        float offsetSq =
            (lastX - 160) * (lastX - 160) + (lastY - 120) * (lastY - 120);
        if (offsetSq > 99.0 * 99.0) {
          // shrink to
          float ox = lastX - 160;
          float oy = lastY - 120;
          float scale = 99.0 / sqrt(offsetSq);
          ox *= scale;
          oy *= scale;
          ox += 160.0;
          oy += 120.0;
          lastX = ox;
          lastY = oy;
        }

        uint16_t *rgb565Data = (uint16_t *)wheel.data;
        uint16_t rgb = rgb565Data[lastX + lastY * 320];
        int r, g, b;
        r = (rgb >> 8) & 0b11111000;
        g = (rgb >> 2) & 0b11111100;
        b = (rgb << 3) & 0b11111000;
        char text[256];
        snprintf(text, 64, "Wheel touching at %d %d (%d %d %d)", lastX, lastY,
                 r, g, b);
        Serial.println(text);
        // move crosshairs to point here
        LV_IMAGE_DECLARE(crosshairs);

        lv_obj_set_x(crosshair_img, lastX - (crosshairs.header.w >> 1));
        lv_obj_set_y(crosshair_img, lastY - (crosshairs.header.h >> 1));
      }
    }
  }

  void OnUpDown(bool up, lv_event_code_t code) {
    lv_obj_t *cat_pic;
    if (up) {
      cat_pic = up_img;
    } else {
      cat_pic = down_img;
    }
    lv_anim_t a;

    switch (code) {
    case LV_EVENT_PRESSED:
      lv_anim_init(&a);
      lv_anim_set_var(&a, cat_pic);
      lv_anim_set_values(&a, lv_obj_get_y(cat_pic), 240 - 100);
      lv_anim_set_duration(&a, 50);
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
      lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
      lv_anim_start(&a);
      break;
    case LV_EVENT_RELEASED:
      lv_anim_init(&a);
      lv_anim_set_var(&a, cat_pic);
      lv_anim_set_values(&a, lv_obj_get_y(cat_pic), 240);
      lv_anim_set_duration(&a, 200);
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
      lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
      lv_anim_start(&a);
      break;
    }
  }

  virtual void OnEvent(lv_event_t *e, lv_obj_t *target, lv_event_code_t code) {

    if (target == colour_wheel || target == crosshair_img) {
      // handle colour pressed
      OnColourwheel(code);
    }
    if (target == up || target == up_img) {
      OnUpDown(true, code);
    }
    if (target == down || target == down_img) {
      OnUpDown(false, code);
    }
  }

  lv_obj_t *up;
  lv_obj_t *down;
  lv_obj_t *up_img;
  lv_obj_t *down_img;

  lv_obj_t *crosshair_img;

  lv_obj_t *colour_wheel;
};