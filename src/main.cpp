#include <Arduino.h>
#include <M5Unified.h>

#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include <esp_timer.h>

LV_IMAGE_DECLARE(wheel);
LV_IMAGE_DECLARE(wheel_pressed);
LV_IMAGE_DECLARE(crosshairs);
lv_obj_t * up;
lv_obj_t * down;
lv_obj_t * up_img;
lv_obj_t * down_img;
lv_obj_t * crosshair_img;



constexpr int32_t HOR_RES=320;
constexpr int32_t VER_RES=240;

lv_display_t *display;
lv_indev_t *indev;

void my_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  lv_draw_sw_rgb565_swap(px_map, w*h);
  M5.Display.pushImageDMA<uint16_t>(area->x1, area->y1, w, h, (uint16_t *)px_map);
  lv_disp_flush_ready(disp);
}

uint32_t my_tick_function() {
  return (esp_timer_get_time() / 1000LL);
}

int lastX=-1;
int lastY=-1;

void my_touchpad_read(lv_indev_t * drv, lv_indev_data_t * data) {
  M5.update();
  auto count = M5.Touch.getCount();

  if ( count == 0 ) {
    data->state = LV_INDEV_STATE_RELEASED;
    lastX=-1;
    lastY=-1;
  } else {
    auto touch = M5.Touch.getDetail(0);
    if(touch.y>=VER_RES){
      data->state = LV_INDEV_STATE_PRESSED; 
      data->point.x = touch.x;
      data->point.y = VER_RES-1;

    }else{
      data->state = LV_INDEV_STATE_PRESSED; 
      data->point.x = touch.x;
      data->point.y = touch.y;
    }
    lastX=data->point.x;
    lastY=data->point.y;
  }
}

// LVGL code - Handle multiple events demo
static void wheel_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *label = reinterpret_cast<lv_obj_t *>(lv_event_get_user_data(e));

  char text[64];


  switch (code)
  {
    case LV_EVENT_PRESSING:
    if(lastX!=-1){
      
      float offsetSq = (lastX-160)*(lastX-160)+(lastY-120)*(lastY-120);
      if(offsetSq>99.0*99.0){
        // shrink to 
        float ox = lastX-160;
        float oy = lastY-120;
        float scale = 99.0/sqrt(offsetSq);
        ox*=scale;
        oy*=scale;
        ox+=160.0;
        oy+=120.0;
        lastX=ox;
        lastY=oy;
      }

      uint16_t *rgb565Data = (uint16_t*)wheel.data;
      uint16_t rgb = rgb565Data[lastX+lastY*320];
      int r,g,b;
      r=(rgb>>8)&0b11111000;
      g=(rgb>>2)&0b11111100;
      b=(rgb<<3)&0b11111000;
      snprintf(text,64,"Button touching at %d %d (%d %d %d)",lastX,lastY,r,g,b);
      lv_label_set_text(label, text);
      // move crosshairs to point here
      lv_obj_set_x(crosshair_img,lastX-(crosshairs->header.w>>1));
      lv_obj_set_y(crosshair_img,lastY-(crosshairs->header.h>>1));  
    }
    break;

  case LV_EVENT_PRESSED:
//    lv_label_set_text(label, "The last button event:\nLV_EVENT_PRESSED");
    break;
  case LV_EVENT_CLICKED:
//    lv_label_set_text(label, "The last button event:\nLV_EVENT_CLICKED");
    break;
  case LV_EVENT_LONG_PRESSED:
//    lv_label_set_text(label, "The last button event:\nLV_EVENT_LONG_PRESSED");
    break;
  case LV_EVENT_LONG_PRESSED_REPEAT:
//    lv_label_set_text(label, "The last button event:\nLV_EVENT_LONG_PRESSED_REPEAT");
    break;
  default:
    break;
  }
}

static void updown_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *cat_pic = reinterpret_cast<lv_obj_t *>(lv_event_get_user_data(e));

  lv_anim_t a;

  switch(code){
    case LV_EVENT_PRESSED:
      lv_anim_init(&a);
      lv_anim_set_var(&a, cat_pic);
      lv_anim_set_values(&a, lv_obj_get_y(cat_pic), 240-100);
      lv_anim_set_duration(&a, 200);
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
      lv_anim_set_path_cb(&a, lv_anim_path_bounce);
      lv_anim_start(&a);
      Serial.println("Pressed");
    break;
    case LV_EVENT_RELEASED:
      lv_anim_init(&a);
      lv_anim_set_var(&a, cat_pic);
      lv_anim_set_values(&a, lv_obj_get_y(cat_pic), 240);
      lv_anim_set_duration(&a, 200);
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
      lv_anim_set_path_cb(&a, lv_anim_path_bounce);
      lv_anim_start(&a);
      Serial.println("Released");
  break;
  }
  
}

void my_log_cb(lv_log_level_t level, const char * buf)
{
  Serial.write(buf);
}





/**
 * Handle multiple events
 */
void lv_example_event_2(void)
{
  lv_obj_t *btn = lv_button_create(lv_screen_active());
  lv_obj_set_size(btn, 100, 50);
  lv_obj_center(btn);

  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Click me!");
  lv_obj_center(btn_label);



  lv_obj_t * img;
  img = lv_imgbtn_create(lv_scr_act() );
//  img = lv_image_create(lv_screen_active());
  lv_imgbtn_set_src(img, LV_IMGBTN_STATE_RELEASED,NULL,&wheel,NULL);
  lv_imgbtn_set_src(img, LV_IMGBTN_STATE_PRESSED,NULL,&wheel_pressed,NULL);
  lv_obj_center(img);

  crosshair_img = lv_image_create(lv_screen_active());
  lv_image_set_src(crosshair_img,&crosshairs);
  lv_obj_center(img);
  lv_obj_add_flag(crosshair_img, LV_OBJ_FLAG_FLOATING);  


  LV_IMAGE_DECLARE(cat_less);
  LV_IMAGE_DECLARE(cat_more);

  LV_IMAGE_DECLARE(uparrow);
  LV_IMAGE_DECLARE(downarrow);
  
  up = lv_imgbtn_create(lv_scr_act() );
  lv_imgbtn_set_src(up, LV_IMGBTN_STATE_RELEASED,NULL,&uparrow,NULL);
  lv_imgbtn_set_src(up, LV_IMGBTN_STATE_PRESSED,NULL,&uparrow,NULL);
  lv_obj_align(up, LV_ALIGN_BOTTOM_RIGHT, 0, 0);


  up_img = lv_image_create(lv_screen_active());
  lv_image_set_src(up_img,&cat_more);
  lv_obj_set_y(up_img, 240);
  lv_obj_set_x(up_img,220);
  lv_obj_add_flag(up_img, LV_OBJ_FLAG_FLOATING);  

  
  down = lv_imgbtn_create(lv_scr_act() );
  lv_imgbtn_set_src(down, LV_IMGBTN_STATE_RELEASED,NULL,&downarrow,NULL);
  lv_imgbtn_set_src(down, LV_IMGBTN_STATE_PRESSED,NULL,&downarrow,NULL);
  lv_obj_align(down, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  
  down_img = lv_image_create(lv_screen_active());
  lv_obj_add_flag(down_img, LV_OBJ_FLAG_FLOATING);  
  lv_image_set_src(down_img,&cat_less);
  lv_obj_set_y(down_img, 2r0);
  lv_obj_set_x(down_img,0);

  lv_obj_t *info_label = lv_label_create(lv_screen_active());
  lv_label_set_text(info_label, "The last button event:\nNone");
  lv_obj_set_style_text_color(info_label,LV_COLOR_MAKE(255, 255, 255),LV_PART_MAIN);



  lv_obj_add_event_cb(img, wheel_cb, LV_EVENT_ALL, info_label);
  lv_obj_add_event_cb(crosshair_img, wheel_cb, LV_EVENT_ALL, info_label);
  

  lv_obj_add_event_cb(up, updown_cb, LV_EVENT_ALL, up_img);
  lv_obj_add_event_cb(down, updown_cb, LV_EVENT_ALL, down_img);

  lv_obj_add_event_cb(up_img,updown_cb,LV_EVENT_ALL,up_img);
  lv_obj_add_event_cb(down_img,updown_cb,LV_EVENT_ALL,down_img);
}

// continue setup code
void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Test");

  M5.begin();
  lv_init();
  lv_log_register_print_cb(my_log_cb);

  lv_tick_set_cb(my_tick_function);
 
  display = lv_display_create(HOR_RES, VER_RES);
  lv_display_set_flush_cb(display, my_display_flush);

  static lv_color_t buf1[HOR_RES * 15] __attribute__((aligned (4)));; 
  lv_display_set_buffers(display, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);

  lv_indev_set_read_cb(indev, my_touchpad_read);

  lv_example_event_2();
}

void loop() {
  lv_task_handler();
  vTaskDelay(1);
}