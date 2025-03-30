#pragma once

#include "colour_screen.h"
#include <array>

#include "light_output.h"
#include "sound_input.h"

#include "core/lv_obj_event_private.h"
class DiscoScreen : public ColourScreen
{
  public:
    DiscoScreen()
    {
        audioV = 0;
        LV_IMAGE_DECLARE(audiomode_amplitude);
        LV_IMAGE_DECLARE(audiomode_fft);
        LV_IMAGE_DECLARE(audiomode_off);

        audio_mode_images[0] = &audiomode_off;
        audio_mode_images[1] = &audiomode_amplitude;
        audio_mode_images[2] = &audiomode_fft;

        LV_IMAGE_DECLARE(outmode_flash);
        LV_IMAGE_DECLARE(outmode_parts);
        LV_IMAGE_DECLARE(outmode_spin);
        LV_IMAGE_DECLARE(outmode_swirl);

        output_mode_images[0] = &outmode_flash;
        output_mode_images[1] = &outmode_parts;
        output_mode_images[2] = &outmode_spin;
        output_mode_images[3] = &outmode_swirl;

        cur_audio_mode = 0;
        cur_output_mode = 0;
        audio_mode = lv_image_create(screen);
        lv_obj_add_flag(audio_mode, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(audio_mode, LV_OBJ_FLAG_ADV_HITTEST);

        setAudioModeImage();

        output_mode = lv_image_create(screen);
        lv_obj_add_flag(output_mode, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(output_mode, LV_OBJ_FLAG_ADV_HITTEST);
        setOutputModeImage();

        HandleEvent(audio_mode, LV_EVENT_ALL);
        HandleEvent(output_mode, LV_EVENT_ALL);
        scroll_timer = enableTimer(20);
        screenTimeout=3600; // keep disco on for an hour before timing out
    }

  protected:
    virtual void onScreenLoad()
    {
        onColourChange(false);
    }

    void setAudioModeImage()
    {
        if (cur_audio_mode < 0 || cur_audio_mode >= audio_mode_images.size())
        {
            cur_audio_mode = 0;
        }
        lv_image_set_src(audio_mode, audio_mode_images[cur_audio_mode]);
        lv_obj_align(audio_mode, LV_ALIGN_TOP_LEFT, 0, 0);
        showUpDown(cur_audio_mode==0);
    }

    virtual void onTimer(int id)
    {
        ColourScreen::onTimer(id);
        if (id != scroll_timer)
        {
            return;
        }
        LightOutput *lo = LightOutput::GetLightOutput();
        SoundInput *si = SoundInput::GetSoundInput();
        if (lo != NULL)
        {
            switch (cur_audio_mode)
            {
            case 0:
                lo->scroll(!pressed);
                break;
            case 1: // audio level only - pass it in
                audioV = (si->getLevel());
                lo->onColour(h, s, audioV, false);
                lo->scroll(audioV<30);
                break;
            case 2: // auto colour and audio level
                // handle differently depending on
                // output mode
                handleFFTData();
                break;
            }
        }
    }

    void handleFFTData()
    {
        LightOutput *lo = LightOutput::GetLightOutput();
        SoundInput *si = SoundInput::GetSoundInput();
        const SoundInput::SpectrumBufType &lastSpectrum = si->getSpectrum();
        switch (cur_output_mode)
        {
        case 0: // constant
                // choose a colour based on fft
                // n.b. value = level
                //      hue = most powerful pitch
                //      saturation based on average / most powerful,
                //      i.e. white noise = white, single tone = colour of pitch
        case 2: {
            int16_t maxMag = -1;
            int16_t maxPos = 0;
            uint32_t sumMag = 0;
            for (int c = 0; c < lastSpectrum.size(); c++)
            {
                if (lastSpectrum[c] > maxMag)
                {
                    maxMag = lastSpectrum[c];
                    maxPos = c;
                }
                sumMag += lastSpectrum[c];
            }

            if (SoundInput::FFT_SIZE == 256)
            {
                h = maxPos << 1;
            }
            else
            {
                h = (maxPos * 512) / (SoundInput::FFT_SIZE);
            }
            // cycle hue so that we don't just get one colour
            h=(h*6)&0xff;

            uint32_t avgMag = sumMag <<1; // average * 256
            if(maxMag<=0)maxMag=1;
            avgMag = avgMag / maxMag;
            avgMag = avgMag<<3; // multiply by 8
            // i.e. if avg > max/32 it will output 255
            if(avgMag<120){
                avgMag=0;
            }else{
                avgMag=avgMag-120;
            }


            if (avgMag > 255)
                avgMag = 255;
            s = avgMag;
            s = 255 - s;
            audioV = (si->getLevel());
            // rotate - choose a colour based on fft and rotate it
            // same as 0
            // unless v is low in which case don't send a colour and fade instead
            lo->onColour(h, s, audioV, true, true);
            lo->scroll((audioV < 30));
        }
        break;
        case 1: // segmented - visualise fft on the sides
                // value = level, hue = highest pitch in each section,
                // saturation = avg/most powerful in section
        case 3: // swirl - inject colours at 6 points based on fft
            // same as above re: colour
            {
                auto segmentIndices = si->getSpectrumSegmentIndices();
                int32_t curLevel = si->getLevel();
                std::array<int32_t, 6> segmentH;
                std::array<int32_t, 6> segmentS;
                std::array<int32_t, 6> segmentV;
                int32_t maxMax = 0;
                int spectrumPos = 1;
                for (int segment = 0; segment < 6; segment++)
                {
                    int32_t segStart = spectrumPos;
                    int32_t segEnd = spectrumPos;
                    int32_t segSum = 0;
                    int32_t segMax = -1;
                    int32_t segMaxPos = spectrumPos;
                    while (segEnd < lastSpectrum.size() && segmentIndices[segEnd] <= segment)
                    {
                        if (segMax < lastSpectrum[spectrumPos])
                        {
                            segMax = lastSpectrum[spectrumPos];
                            segMaxPos = spectrumPos;
                        }
                        segSum += lastSpectrum[spectrumPos];
                        segEnd++;
                        spectrumPos++;
                    }
                    if (segEnd != segStart)
                    {
                        int32_t segAvg = segSum / (segEnd - segStart);
                        segmentH[segment] = (255 * (segMaxPos - segStart)) / (segEnd - segStart);
                        if (segMax < 40)
                        {
                            segmentS[segment] = 0;
                            segmentV[segment] = 0;
                        }
                        else
                        {
                            segmentS[segment] = (255 * segAvg) / segMax;
                            segmentV[segment] = segMax;
                        }
                        if (segMax > maxMax)
                            maxMax = segMax;

                        // Serial.print(segment);
                        // Serial.print(":");
                        // Serial.print(si->getLevel());
                        // Serial.print("Max:");
                        // Serial.print(segMax);
                        // Serial.print("@");
                        // Serial.print(segMaxPos);
                        // Serial.print(",avg:");
                        // Serial.print(segAvg);
                        // Serial.print("(");
                        // Serial.print(segmentH[segment]);
                        // Serial.print(",");
                        // Serial.print(segmentS[segment]);
                        // Serial.print(",");
                        // Serial.print(segmentV[segment]);
                        // Serial.println(")");
                    }
                    else
                    {
                        Serial.print("Zero segment");
                        Serial.println(segment);
                    }
                }
                if (curLevel < 30)
                {
                    for (int segment = 0; segment < 6; segment++)
                    {
                        segmentV[segment] = 0;
                    }
                }
                else
                {
                    if (maxMax > 0)
                    {
                        for (int segment = 0; segment < 6; segment++)
                        {
                            segmentV[segment] = (curLevel * segmentV[segment]) / maxMax;
                        }
                    }
                }
                lo->onMultipleColours(segmentH, segmentS, segmentV);
                lo->scroll(true);
            }
            break;
        }
    }

    virtual void onColourChange(bool onTouch)
    {
        LightOutput *lo = LightOutput::GetLightOutput();
        if (lo != NULL)
        {
            if (cur_audio_mode != 0)
            {
                lo->onColour(h, s, audioV, true, onTouch);
            }
            else
            {
                lo->onColour(h, s, v, true, onTouch);
            }
            switch (cur_output_mode)
            {
            default:
                lo->setEffect(LightOutput::EFFECT_CONSTANT);
                break;
            case 1:
                lo->setEffect(LightOutput::EFFECT_BLOCKS);
                break;
            case 2:
                lo->setEffect(LightOutput::EFFECT_SPIN);
                break;
            case 3:
                lo->setEffect(LightOutput::EFFECT_SWIRL);
                break;
            }
        }
    }

    void setOutputModeImage()
    {
        if (cur_output_mode < 0 || cur_output_mode >= output_mode_images.size())
        {
            cur_output_mode = 0;
        }
        lv_image_set_src(output_mode, output_mode_images[cur_output_mode]);
        lv_obj_align(output_mode, LV_ALIGN_TOP_RIGHT, 0, 0);
        onColourChange(false);
    }

    void OnAudioModeButton(lv_event_code_t code, lv_event_t *e)
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

    void OnOutputModeButton(lv_event_code_t code, lv_event_t *e)
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
        if (code == LV_EVENT_HIT_TEST && (target==output_mode|| target==audio_mode))
        {
            auto info = lv_event_get_hit_test_info(e);
            auto obj = lv_event_get_target_obj(e);
            int x = info->point->x - lv_obj_get_x(obj);
            int y = info->point->y - lv_obj_get_y(obj);
            info->res = true;
            const lv_img_dsc_t *img = (const lv_img_dsc_t *)lv_image_get_src(obj);
            if (x < 0 || x >= img->header.w || y < 0 || y >= img->header.h)
            {
                info->res = false;
            }
            else
            {
                const uint8_t *mask = &img->data[img->header.h * img->header.stride];
                info->res = mask[x + y * (img->header.stride >> 1)] > 0x7f;
            }
        }
        if (target == audio_mode)
        {
            OnAudioModeButton(code, e);
        }
        else if (target == output_mode)
        {
            OnOutputModeButton(code, e);
        }

        if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING)
        {
            pressed = true;
        }
        else if (code == LV_EVENT_RELEASED)
        {
            pressed = false;
        }
        ColourScreen::OnEvent(e, target, code);
    }

    std::array<const lv_img_dsc_t *, 3> audio_mode_images;
    std::array<const lv_img_dsc_t *, 4> output_mode_images;
    lv_obj_t *audio_mode;
    lv_obj_t *output_mode;

    bool pressed;

    int cur_audio_mode;
    int cur_output_mode;

    uint8_t audioV;

    int scroll_timer;
};