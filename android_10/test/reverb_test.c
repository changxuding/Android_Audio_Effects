/*
 * Copyright (C) 2020 The Android Open Source Project
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
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LVREV_Api.h"

// Global Variables
enum ReverbParams {
    ARG_HELP = 1,
    ARG_INPUT,
    ARG_OUTPUT,
    ARG_FS,
    ARG_CH_MASK,
    ARG_PRESET,
    ARG_AUX,
    ARG_MONO_MODE,
    ARG_FILE_CH,
};

// structures
typedef struct{
    int fChannels;
    int monoMode;
    int frameLength;
    int preset;
    int nrChannels;
    int sampleRate;
    int auxiliary;
    audio_channel_mask_t chMask;
}reverbConfigParams_t;

int reverbCreateEffect(ReverbContext* pContext, 
                        effect_config_t* pConfig) 
{
    // status = reverbCreateEffect(&effectHandle, &config, sessionId, ioId, revConfigParams.auxiliary);
    int ret;
    printf("\t\nEffectCreate start");
    pContext->hInstance = NULL;
#if 1
    pContext->auxiliary = 0;
    pContext->preset = 0;
#else
    pContext->auxiliary = 1;
    pContext->preset = 1;
    pContext->curPreset = REVERB_PRESET_LAST + 1;
    pContext->nextPreset = REVERB_DEFAULT_PRESET;
#endif
    printf("\tEffectCreate - Calling Reverb_init");
    ret = Reverb_init(pContext);
    if (ret < 0){
        printf("\tLVM_ERROR : EffectCreate() init failed");
        free(pContext);
        return ret;
    }

    int channels = 2;
    // Allocate memory for reverb process (*2 is for STEREO)
    pContext->bufferSizeIn = LVREV_MAX_FRAME_SIZE * sizeof(process_buffer_t) * channels;
    pContext->bufferSizeOut = LVREV_MAX_FRAME_SIZE * sizeof(process_buffer_t) * FCC_2;
    pContext->InFrames  = (process_buffer_t *)calloc(pContext->bufferSizeIn, 1 /* size */);
    pContext->OutFrames = (process_buffer_t *)calloc(pContext->bufferSizeOut, 1 /* size */);

    printf("\tEffectCreate %p, size %zu", pContext, sizeof(ReverbContext));
    ret = Reverb_setConfig(pContext, pConfig);

    return ret;
}

int reverbSetConfigParam(uint32_t paramType, uint32_t paramValue, ReverbContext* context) {
    int reply = 0;
    uint32_t replySize = sizeof(reply);
    uint32_t paramData[2] = {paramType, paramValue};
    effect_param_t* effectParam = (effect_param_t*)malloc(sizeof(*effectParam) + sizeof(paramData));
    memcpy(&effectParam->data[0], &paramData[0], sizeof(paramData));
    effectParam->psize = sizeof(paramData[0]);
    effectParam->vsize = sizeof(paramData[1]);
    #if 1
    int status= Reverb_setParameter(context, (void *)effectParam->data, effectParam->data + effectParam->psize, effectParam->vsize);
    #endif
    free(effectParam);
    if (status != 0) {
        printf("Reverb set config returned an error = %d\n", status);
        return status;
    }
    return reply;
}

int EffectRelease(ReverbContext* pContext){

    free(pContext->InFrames);
    free(pContext->OutFrames);
    pContext->bufferSizeIn = 0;
    pContext->bufferSizeOut = 0;
    Reverb_free(pContext);
    free(pContext);
    return 0;
} /* end EffectRelease */

void printUsage() {
    printf("\nUsage: ");
    printf("\n     <executable> [options]\n");
    printf("\nwhere options are, ");
    printf("\n     --input <inputfile>");
    printf("\n           path to the input file");
    printf("\n     --output <outputfile>");
    printf("\n           path to the output file");
    printf("\n     --help");
    printf("\n           prints this usage information");
    printf("\n     --chMask <channel_mask>\n");
    printf("\n           0  - AUDIO_CHANNEL_OUT_MONO");
    printf("\n           1  - AUDIO_CHANNEL_OUT_STEREO");
    printf("\n           2  - AUDIO_CHANNEL_OUT_2POINT1");
    printf("\n           3  - AUDIO_CHANNEL_OUT_2POINT0POINT2");
    printf("\n           4  - AUDIO_CHANNEL_OUT_QUAD");
    printf("\n           5  - AUDIO_CHANNEL_OUT_QUAD_BACK");
    printf("\n           6  - AUDIO_CHANNEL_OUT_QUAD_SIDE");
    printf("\n           7  - AUDIO_CHANNEL_OUT_SURROUND");
    printf("\n           8  - canonical channel index mask for 4 ch: (1 << 4) - 1");
    printf("\n           9  - AUDIO_CHANNEL_OUT_2POINT1POINT2");
    printf("\n           10 - AUDIO_CHANNEL_OUT_3POINT0POINT2");
    printf("\n           11 - AUDIO_CHANNEL_OUT_PENTA");
    printf("\n           12 - canonical channel index mask for 5 ch: (1 << 5) - 1");
    printf("\n           13 - AUDIO_CHANNEL_OUT_3POINT1POINT2");
    printf("\n           14 - AUDIO_CHANNEL_OUT_5POINT1");
    printf("\n           15 - AUDIO_CHANNEL_OUT_5POINT1_BACK");
    printf("\n           16 - AUDIO_CHANNEL_OUT_5POINT1_SIDE");
    printf("\n           17 - canonical channel index mask for 6 ch: (1 << 6) - 1");
    printf("\n           18 - AUDIO_CHANNEL_OUT_6POINT1");
    printf("\n           19 - canonical channel index mask for 7 ch: (1 << 7) - 1");
    printf("\n           20 - AUDIO_CHANNEL_OUT_5POINT1POINT2");
    printf("\n           21 - AUDIO_CHANNEL_OUT_7POINT1");
    printf("\n           22 - canonical channel index mask for 8 ch: (1 << 8) - 1");
    printf("\n           default 0");
    printf("\n     --fs <sampling_freq>");
    printf("\n           Sampling frequency in Hz, default 48000.");
    printf("\n     --preset <preset_value>");
    printf("\n           0 - None");
    printf("\n           1 - Small Room");
    printf("\n           2 - Medium Room");
    printf("\n           3 - Large Room");
    printf("\n           4 - Medium Hall");
    printf("\n           5 - Large Hall");
    printf("\n           6 - Plate");
    printf("\n           default 0");
    printf("\n     --fch <file_channels>");
    printf("\n           number of channels in input file (1 through 8), default 1");
    printf("\n     --M");
    printf("\n           Mono mode (force all input audio channels to be identical)");
    printf("\n     --aux <auxiliary_flag> ");
    printf("\n           0 - Insert Mode on");
    printf("\n           1 - auxiliary Mode on");
    printf("\n           default 0");
    printf("\n");
}

static inline int16_t clamp16_from_float(float f)
{
    static const float scale = 1 << 15;
    return roundf(fmaxf(fminf(f * scale, scale - 1.f), -scale));
}
static inline float float_from_i16(int16_t ival)
{
    static const float scale = 1. / (float)(1UL << 15);
    return ival * scale;
}

void memcpy_to_float_from_i16(float *dst, const int16_t *src, size_t count)
{
    dst += count;
    src += count;
    for (; count > 0; --count) {
        *--dst = float_from_i16(*--src);
    }
}

void memcpy_to_i16_from_float(int16_t *dst, const float *src, size_t count)
{
    for (; count > 0; --count) {
        *dst++ = clamp16_from_float(*src++);
    }
}

int main() 
{
    reverbConfigParams_t revConfigParams;  // default initialize
    int status;

    revConfigParams.fChannels = 2;
    revConfigParams.monoMode = 0;
    revConfigParams.frameLength = 256;
    revConfigParams.preset = 0;
    revConfigParams.nrChannels = 2;
    revConfigParams.sampleRate = 48000;
    revConfigParams.auxiliary = 0;
    revConfigParams.chMask = AUDIO_CHANNEL_OUT_STEREO;

    const char* infile = "D:/audio_effects_test/music.pcm";
    const char* outfile ="D:/audio_effects_test/music_cs_out.pcm";
    FILE *finp = fopen(infile, "rb");
    if (finp == NULL) 
    {
        printf("Cannot open input file %s", infile);
        return -1;
    }

    FILE *fout = fopen(outfile, "wb");
    if (fout == NULL) 
    {
        printf("Cannot open output file %s", outfile);
        fclose(finp);
        return -1;
    }
   
    ReverbContext context;
    effect_config_t config;
    config.inputCfg.samplingRate = config.outputCfg.samplingRate = revConfigParams.sampleRate;
    config.inputCfg.channels = config.outputCfg.channels = revConfigParams.chMask;
    config.inputCfg.format = config.outputCfg.format = AUDIO_FORMAT_PCM_FLOAT;

    status = reverbCreateEffect(&context, &config);
    if(status != 0) {
        printf("Create effect call returned error %i", status);
        return EXIT_FAILURE;
    }

    // set enable 
    LVREV_ControlParams_st ActiveParams; 
    context.bEnabled = LVM_TRUE;
    /* Get the current settings */
    int LvmStatus = LVREV_GetControlParameters(context.hInstance, &ActiveParams);
    context.SamplesToExitCount =
            (ActiveParams.T60 * context.config.inputCfg.samplingRate)/1000;
    // force no volume ramp for first buffer processed after enabling the effect
    context.volumeMode = REVERB_VOLUME_FLAT;

    // set preset config
    status = reverbSetConfigParam(REVERB_PARAM_PRESET, (uint32_t)revConfigParams.preset, &context);
    if(status != 0) {
        printf("Invalid reverb preset. Error %d\n", status);
        return EXIT_FAILURE;
    }

    revConfigParams.nrChannels = 2;// audio_channel_count_from_out_mask(revConfigParams.chMask);
    const int channelCount = revConfigParams.nrChannels;
    const int frameLength = revConfigParams.frameLength;
#ifdef BYPASS_EXEC
    const int frameSize = (int)channelCount * sizeof(float);
#endif
    const int ioChannelCount = revConfigParams.fChannels;
    const int ioFrameSize = ioChannelCount * sizeof(short);
    const int maxChannelCount = channelCount>ioChannelCount?channelCount:ioChannelCount;
    int frameCounter = 0;

    short *in = (short*)calloc(frameLength * maxChannelCount, sizeof(short));
    short *out = (short*)calloc(frameLength * maxChannelCount, sizeof(short));
    float *floatIn = (float*)calloc(frameLength * maxChannelCount, sizeof(float));
    float *floatOut = (float*)calloc(frameLength * maxChannelCount, sizeof(float));

    // loop process
    while (fread(in, ioFrameSize, frameLength, finp) == (size_t)frameLength) 
    {
        if (ioChannelCount != channelCount)
        {
            printf("ioChannelCount must be equal to channelCount");
            return -1;
        }
        memcpy_to_float_from_i16(floatIn, in, frameLength * channelCount);

        if (revConfigParams.monoMode && channelCount > 1) {
            for (int i = 0; i < frameLength; ++i) {
                float* fp = &floatIn[i * channelCount];
                memcpy(fp+1, fp, sizeof(float));// replicate ch 0
            }
        }

        audio_buffer_t inputBuffer, outputBuffer;
        inputBuffer.frameCount = outputBuffer.frameCount = frameLength;
        inputBuffer.f32 = floatIn;
        outputBuffer.f32 = floatOut;
#ifndef BYPASS_EXEC
        status = Reverb_process(&context, &inputBuffer, &outputBuffer);
        if (status != 0) {
            printf("\nError: Process returned with error %d\n", status);
            return EXIT_FAILURE;
        }
#else
        memcpy(floatOut, floatIn, frameLength * frameSize);
#endif
        memcpy_to_i16_from_float(out, floatOut, frameLength * channelCount);

        if (ioChannelCount != channelCount) {
            printf("ioChannelCount must be equal to channelCount");
            return -1;
        }
        (void)fwrite(out, ioFrameSize, frameLength, fout);
        frameCounter += frameLength;
    }

    status = EffectRelease(&context); 
    if(status != 0) {
        printf("Audio Preprocessing release returned an error = %d\n", status);
        return EXIT_FAILURE;
    }
    printf("frameCounter: [%d]\n", frameCounter);

    return EXIT_SUCCESS;
}
