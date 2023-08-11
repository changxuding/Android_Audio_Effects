/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_EFFECT_PRESETREVERB_CORE_H_
#define ANDROID_EFFECT_PRESETREVERB_CORE_H_

#include "audio_effect.h"

#if __cplusplus
extern "C" {
#endif

/* enumerated parameter settings for preset reverb effect */
typedef enum
{
    REVERB_PARAM_PRESET
} t_preset_reverb_params;


typedef enum
{
    REVERB_PRESET_NONE,
    REVERB_PRESET_SMALLROOM,
    REVERB_PRESET_MEDIUMROOM,
    REVERB_PRESET_LARGEROOM,
    REVERB_PRESET_MEDIUMHALL,
    REVERB_PRESET_LARGEHALL,
    REVERB_PRESET_PLATE,
    REVERB_PRESET_LAST = REVERB_PRESET_PLATE
} t_reverb_presets;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_PRESETREVERB_CORE_H_*/
