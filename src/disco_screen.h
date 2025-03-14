#pragma once

#include "colour_screen.h"
#include <array>

#include "core/lv_obj_event_private.h"
class DiscoScreen : public ColourScreen
{
  public:
    DiscoScreen()
    {
        LV_IMAGE_DECLARE(audiomode_amplitude);
        LV_IMAGE_DECLARE(audiomode_fft);
        LV_IMAGE_DECLARE(audiomode_off);

        audio_mode_images[0] = &audiomode_off;
        audio_mode_images[1] = &audiomode_amplitude;
        audio_mode_images[2] = &audiomode_fft;

        LV_IMAGE_DECLARE(outmode_flash);
        LV_IMAGE_DECLARE(outmode_parts);
        LV_IMAGE_DECLARE(outmode_spin);

        output_mode_images[0] = &outmode_spin;
        output_mode_images[1] = &outmode_parts;
        output_mode_images[2] = &outmode_flash;

        cur_audio_mode = 0;
        cur_output_mode = 0;
        audio_mode = lv_image_create(screen);
        lv_obj_add_flag(audio_mode,LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(audio_mode,LV_OBJ_FLAG_ADV_HITTEST);
        
        setAudioModeImage();

        output_mode = lv_image_create(screen);
        lv_obj_add_flag(output_mode,LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(output_mode,LV_OBJ_FLAG_ADV_HITTEST);
        setOutputModeImage();

        HandleEvent(audio_mode, LV_EVENT_ALL);
        HandleEvent(output_mode, LV_EVENT_ALL);
    }

  protected:
    void setAudioModeImage()
    {
        if (cur_audio_mode < 0 || cur_audio_mode >= audio_mode_images.size())
        {
            cur_audio_mode = 0;
        }
        lv_image_set_src(audio_mode, audio_mode_images[cur_audio_mode]);
        lv_obj_align(audio_mode, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    void setOutputModeImage()
    {
        if (cur_output_mode < 0 || cur_output_mode >= output_mode_images.size())
        {
            cur_output_mode = 0;
        }
        lv_image_set_src(output_mode, output_mode_images[cur_output_mode]);
        lv_obj_align(output_mode, LV_ALIGN_TOP_RIGHT, 0, 0);
        Serial.println("WOO");
    }

    void OnAudioModeButton(lv_event_code_t code,lv_event_t *e)
    {
        if (code == LV_EVENT_PRESSED)
        {
            cur_audio_mode++;
            if (cur_audio_mode >= audio_mode_images.size())
            {
                cur_audio_mode = 0;
            }
            setAudioModeImage();
        }
    }

    void OnOutputModeButton(lv_event_code_t code,lv_event_t *e)
    {
        if (code == LV_EVENT_PRESSED)
        {
            cur_output_mode++;
            if (cur_output_mode >= output_mode_images.size())
            {
                cur_output_mode = 0;
            }
            setOutputModeImage();
        }
    }

    virtual void OnEvent(lv_event_t *e, lv_obj_t *target, lv_event_code_t code)
    {
      if(code==LV_EVENT_HIT_TEST)
      {
        auto info=lv_event_get_hit_test_info(e);
        auto obj=lv_event_get_target_obj(e);
        int x=info->point->x -lv_obj_get_x(obj);
        int y=info->point->y -lv_obj_get_y(obj);
        Serial.print(x);
        Serial.print(":");
        Serial.print(y);
        Serial.print("=");
        info->res=true;
        const lv_img_dsc_t *img = (const lv_img_dsc_t*)lv_image_get_src(obj);
        if(x<0 || x>=img->header.w || y<0 || y>=img->header.h)
        {
          info->res=false;
        }else{
          const uint8_t* mask = &img->data[img->header.h*img->header.stride];
          Serial.println(mask[x+y*img->header.w]);
          info->res = mask[x+y*(img->header.stride>>1)]>0x7f;
        }
      }
      if (target == audio_mode)
        {
            OnAudioModeButton(code,e);
        }
        else if (target == output_mode)
        {
            OnOutputModeButton(code,e);
        }
        ColourScreen::OnEvent(e,target,code);
    }

    std::array<const lv_img_dsc_t *, 3> audio_mode_images;
    std::array<const lv_img_dsc_t *, 3> output_mode_images;
    lv_obj_t *audio_mode;
    lv_obj_t *output_mode;

    int cur_audio_mode;
    int cur_output_mode;
};