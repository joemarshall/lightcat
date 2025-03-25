#pragma once

#include "screen.h"
#include"light_output.h"
#include<math.h>

class ColourScreen : public Screen {
public:
  ColourScreen() {
    vStep=0;
#ifdef OUTPUT_COLOUR_PICKER_IMAGE    
    Serial.println("P3");
    Serial.println("201 201");
    Serial.println("255");
    for(int y=20;y<221;y++){
      for(int x=60;x<261;x++){
      {
        h = getHFromPosition(x,y);
        s = getSFromPosition(x,y);
        float oX=x-160,oY=y-120;
        if(sqrt(oX*oX+oY*oY)>100.0){
          v=0;
        }else{
          v=255;
        }
        CHSV hsv(h,s,v);
        CRGB rgb;
        hsv2rgb_rainbow(hsv, rgb);
        Serial.print(rgb.r);
        Serial.print(" ");
        Serial.print(rgb.g);
        Serial.print(" ");
        Serial.print(rgb.b);
        Serial.print(" ");      
      }
      Serial.println("");
    }
    Serial.println("END");
#endif // OUTPUT_COLOUR_IMAGE    
    h,s,v=255,255,128;
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

    HandleEvent(screen,LV_EVENT_ALL);

    HandleEvent(colour_wheel, LV_EVENT_ALL);
    HandleEvent(crosshair_img, LV_EVENT_ALL);

    HandleEvent(up, LV_EVENT_ALL);
    HandleEvent(up_img, LV_EVENT_ALL);

    HandleEvent(down, LV_EVENT_ALL);
    HandleEvent(down_img, LV_EVENT_ALL);

    cat_timer=enableTimer(5);
  }

protected:
  virtual void onTimer(int id)
  {
    if(id==cat_timer){
      if(vStep>0){
        if((255-v)<vStep){
          v=255;
        }else{
          v=v+vStep;
        }
      }else if(vStep<0){
        if(v<-vStep){
          v=0;
        }else{
          v=v+vStep;
        }
      }else{
        return;
      }
      onColourChange(false);
    }
  }
    

  virtual void onScreenLoad()
  {
    onColourChange(false);
  }

  uint8_t getHFromPosition(int x,int y)
  {
    float ox = 160.0-(float)x;
    float oy = 120.0-(float)y;
    float hue = atan2(oy,ox) / PI;
    hue=(1.0+hue)*0.5;
    hue=hue*255.0;
    assert(hue>=0 && hue<=255);
    return uint8_t(hue);
  }

  uint8_t getSFromPosition(int x,int y)
  {
    const float innerDeadZone=0.2; // 20% of centre=white
    const float outerDeadZone=0.05; // 5% of outside = 100% saturated
    float ox = 160.0-(float)x;
    float oy = 120.0-(float)y;
    float r = sqrt(ox*ox + oy*oy);
    if(r>100.0)r=100.0;
    r=r/100.0;
    if(r<innerDeadZone)return 0;
    if(r>1.0-outerDeadZone)return 255;    
    r=(r-innerDeadZone)/((1.0-outerDeadZone) - innerDeadZone);

    r*=255.0;
    return uint8_t(r);
  }

  void OnColourwheel(lv_event_code_t code) {
    if (code == LV_EVENT_PRESSING || code == LV_EVENT_PRESSED) {

      if (lastX != -1) {
        LV_IMAGE_DECLARE(wheel);

        float offsetSq =
            (lastX - 160) * (lastX - 160) + (lastY - 120) * (lastY - 120);
        h = getHFromPosition(lastX,lastY);
        s = getSFromPosition(lastX,lastY);
        char text[256];
//        snprintf(text, 64, "Wheel touching at %d %d (%d %d %d)", lastX, lastY,
//                 r, g, b);
//        Serial.println(text);
        // move crosshairs to point here
        LV_IMAGE_DECLARE(crosshairs);

        lv_obj_set_x(crosshair_img, lastX - (crosshairs.header.w >> 1));
        lv_obj_set_y(crosshair_img, lastY - (crosshairs.header.h >> 1));
        onColourChange(code == LV_EVENT_PRESSED);
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
      vStep=up?1:-1;
      lv_anim_init(&a);
      lv_anim_set_var(&a, cat_pic);
      lv_anim_set_values(&a, lv_obj_get_y(cat_pic), 240 - 100);
      lv_anim_set_duration(&a, 50);
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
      lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
      lv_anim_start(&a);
      
      break;
    case LV_EVENT_RELEASED:
      vStep=0;
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

  virtual void onColourChange(bool onPress)
  {
    LightOutput* lo = LightOutput::GetLightOutput();
    if(lo!=NULL){
      lo->onColour(h,s,v);
      lo->setEffect(LightOutput::EFFECT_CONSTANT);
    }
  }

  int h,s,v;

  lv_obj_t *up;
  lv_obj_t *down;
  lv_obj_t *up_img;
  lv_obj_t *down_img;

  lv_obj_t *crosshair_img;

  lv_obj_t *colour_wheel;

  int cat_timer;

  int vStep;
};