#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "LVM_Private.h"
#include "audio_effect.h"
#include "lvmtest.h"

typedef struct{
    int               samplingFreq;   
    int               nrChannels;      
    int               chMask;          
    int               vcBal;           
    int               fChannels;       
    int               monoMode;        
    int               bassEffectLevel; 
    int               eqPresetLevel;  
    int               frameLength;    
    LVM_BE_Mode_en    bassEnable;     
    LVM_TE_Mode_en    trebleEnable;    
    LVM_EQNB_Mode_en  eqEnable;       
    LVM_Mode_en       csEnable;       
}lvmConfigParams_t; 


void LvmEffect_free(EffectContext *pContext) 
{
    LVM_ReturnStatus_en LvmStatus = LVM_SUCCESS; /* Function call status */
    LVM_MemTab_t MemTab;

    /* Free the algorithm memory */
    LvmStatus = LVM_GetMemoryTable(pContext->pBundledContext->hInstance, &MemTab, LVM_NULL);

    for (int i = 0; i < LVM_NR_MEMORY_REGIONS; i++) 
    {
        if (MemTab.Region[i].Size != 0) 
        {
            if (MemTab.Region[i].pBaseAddress != NULL) 
            {
                free(MemTab.Region[i].pBaseAddress);
            } 
        }
    }
} 


int LvmBundle_init(EffectContext *pContext, LVM_ControlParams_t *params) 
{
    pContext->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    pContext->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.inputCfg.format = EFFECT_BUFFER_FORMAT;
    pContext->config.inputCfg.samplingRate = 44100;
    pContext->config.inputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.cookie = NULL;
    pContext->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    pContext->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    pContext->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.outputCfg.format = EFFECT_BUFFER_FORMAT;
    pContext->config.outputCfg.samplingRate = 44100;
    pContext->config.outputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.cookie = NULL;
    pContext->config.outputCfg.mask = EFFECT_CONFIG_ALL;
    if (pContext->pBundledContext->hInstance != NULL)  LvmEffect_free(pContext);
  

    LVM_ReturnStatus_en LvmStatus = LVM_SUCCESS; /* Function call status */
    LVM_InstParams_t InstParams;                 /* Instance parameters */
    LVM_EQNB_BandDef_t BandDefs[MAX_NUM_BANDS];  /* Equaliser band definitions */
    LVM_HeadroomParams_t HeadroomParams;         /* Headroom parameters */
    LVM_HeadroomBandDef_t HeadroomBandDef[LVM_HEADROOM_MAX_NBANDS];
    LVM_MemTab_t MemTab; /* Memory allocation table */
    bool bMallocFailure = LVM_FALSE;

    /* Set the capabilities */
    InstParams.BufferMode = LVM_UNMANAGED_BUFFERS;
    InstParams.MaxBlockSize = MAX_CALL_SIZE;
    InstParams.EQNB_NumBands = MAX_NUM_BANDS;
    InstParams.PSA_Included = LVM_PSA_ON;

    /* Allocate memory, forcing alignment */
    LvmStatus = LVM_GetMemoryTable(LVM_NULL, &MemTab, &InstParams);
    if (LvmStatus != LVM_SUCCESS) return -EINVAL;

    /* Allocate memory */
    for (int i = 0; i < LVM_NR_MEMORY_REGIONS; i++) 
    {
    if (MemTab.Region[i].Size != 0) 
    {
        MemTab.Region[i].pBaseAddress = malloc(MemTab.Region[i].Size);
        if (MemTab.Region[i].pBaseAddress == LVM_NULL) 
        {
          bMallocFailure = LVM_TRUE;
          break;
        } 
    }
    }

    if (bMallocFailure == LVM_TRUE) 
    {
    for (int i = 0; i < LVM_NR_MEMORY_REGIONS; i++) 
    {
        if (MemTab.Region[i].pBaseAddress == LVM_NULL) {} 
        else free(MemTab.Region[i].pBaseAddress);
    }
    return -EINVAL;
    }

    /* Initialise */
    pContext->pBundledContext->hInstance = LVM_NULL;

    /* Init sets the instance handle */
    LvmStatus = LVM_GetInstanceHandle(&pContext->pBundledContext->hInstance, &MemTab, &InstParams);
    if (LvmStatus != LVM_SUCCESS) return -EINVAL;

    /* Set the initial process parameters */
    /* General parameters */
    params->OperatingMode = LVM_MODE_ON;
    params->SampleRate = LVM_FS_44100;
    params->SourceFormat = LVM_STEREO;
    params->ChMask       = AUDIO_CHANNEL_OUT_STEREO;
    params->SpeakerType = LVM_HEADPHONES;
    pContext->pBundledContext->SampleRate = LVM_FS_44100;

    /* Concert Sound parameters */
    params->VirtualizerOperatingMode = LVM_MODE_OFF;
    params->VirtualizerType = LVM_CONCERTSOUND;
    params->VirtualizerReverbLevel = 100;
    params->CS_EffectLevel = LVM_CS_EFFECT_NONE;

    /* N-Band Equaliser parameters */
    params->EQNB_OperatingMode = LVM_EQNB_OFF;
    params->EQNB_NBands = FIVEBAND_NUMBANDS;
    params->pEQNB_BandDefinition = &BandDefs[0];
    for (int i = 0; i < FIVEBAND_NUMBANDS; i++) 
    {
        BandDefs[i].Frequency = EQNB_5BandPresetsFrequencies[i];
        BandDefs[i].QFactor = EQNB_5BandPresetsQFactors[i];
        BandDefs[i].Gain = EQNB_5BandNormalPresets[i];
    }

    /* Volume Control parameters */
    params->VC_EffectLevel = 0;
    params->VC_Balance = 0;

    /* Treble Enhancement parameters */
    params->TE_OperatingMode = LVM_TE_OFF;
    params->TE_EffectLevel = 0;

    /* PSA Control parameters */
    params->PSA_Enable = LVM_PSA_OFF;
    params->PSA_PeakDecayRate = (LVM_PSA_DecaySpeed_en)0;

    /* Bass Enhancement parameters */
    params->BE_OperatingMode = LVM_BE_OFF;
    params->BE_EffectLevel = 0;
    params->BE_CentreFreq = LVM_BE_CENTRE_90Hz;
    params->BE_HPF = LVM_BE_HPF_ON;

    /* PSA Control parameters */
    params->PSA_Enable = LVM_PSA_OFF;
    params->PSA_PeakDecayRate = LVM_PSA_SPEED_MEDIUM;

    /* TE Control parameters */
    params->TE_OperatingMode = LVM_TE_OFF;
    params->TE_EffectLevel = 0;

    /* Activate the initial settings */
    LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance, params);
    if (LvmStatus != LVM_SUCCESS) return -EINVAL;

    /* Set the headroom parameters */
    HeadroomBandDef[0].Limit_Low = 20;
    HeadroomBandDef[0].Limit_High = 4999;
    HeadroomBandDef[0].Headroom_Offset = 0;
    HeadroomBandDef[1].Limit_Low = 5000;
    HeadroomBandDef[1].Limit_High = 24000;
    HeadroomBandDef[1].Headroom_Offset = 0;
    HeadroomParams.pHeadroomDefinition = &HeadroomBandDef[0];
    HeadroomParams.Headroom_OperatingMode = LVM_HEADROOM_ON;
    HeadroomParams.NHeadroomBands = 2;
    LvmStatus = LVM_SetHeadroomParams(pContext->pBundledContext->hInstance, &HeadroomParams);
    if (LvmStatus != LVM_SUCCESS) return -EINVAL;

    return 0;
} /* end LvmBundle_init */

int lvmCreate(EffectContext *pContext,
              lvmConfigParams_t    *plvmConfigParams,
              LVM_ControlParams_t  *params) 
{
    int ret = 0;
    pContext->pBundledContext = NULL;
    pContext->pBundledContext = (BundledEffectContext *)malloc(sizeof(BundledEffectContext));
    if (NULL == pContext->pBundledContext)
    {
    return -EINVAL;
    }

    pContext->pBundledContext->SessionNo = 0;
    pContext->pBundledContext->SessionId = 0;
    pContext->pBundledContext->hInstance = NULL;
    pContext->pBundledContext->bVolumeEnabled = LVM_FALSE;
    pContext->pBundledContext->bEqualizerEnabled = LVM_FALSE;
    pContext->pBundledContext->bBassEnabled = LVM_FALSE;
    pContext->pBundledContext->bBassTempDisabled = LVM_FALSE;
    pContext->pBundledContext->bVirtualizerEnabled = LVM_FALSE;
    pContext->pBundledContext->bVirtualizerTempDisabled = LVM_FALSE;
    pContext->pBundledContext->nOutputDevice = AUDIO_DEVICE_NONE;
    pContext->pBundledContext->nVirtualizerForcedDevice = AUDIO_DEVICE_NONE;
    pContext->pBundledContext->NumberEffectsEnabled = 0;
    pContext->pBundledContext->NumberEffectsCalled = 0;
    pContext->pBundledContext->firstVolume = LVM_TRUE;
    pContext->pBundledContext->volume = 0;

    /* Saved strength is used to return the exact strength that was used in the
    * set to the get
    * because we map the original strength range of 0:1000 to 1:15, and this will
    * avoid
    * quantisation like effect when returning
    */
    pContext->pBundledContext->BassStrengthSaved = 0;
    pContext->pBundledContext->VirtStrengthSaved = 0;
    pContext->pBundledContext->CurPreset = PRESET_CUSTOM;
    pContext->pBundledContext->levelSaved = 0;
    pContext->pBundledContext->bMuteEnabled = LVM_FALSE;
    pContext->pBundledContext->bStereoPositionEnabled = LVM_FALSE;
    pContext->pBundledContext->positionSaved = 0;
    pContext->pBundledContext->workBuffer = NULL;
    pContext->pBundledContext->frameCount = -1;
    pContext->pBundledContext->SamplesToExitCountVirt = 0;
    pContext->pBundledContext->SamplesToExitCountBb = 0;
    pContext->pBundledContext->SamplesToExitCountEq = 0;
    for (int i = 0; i < FIVEBAND_NUMBANDS; i++) 
    {
      pContext->pBundledContext->bandGaindB[i] = EQNB_5BandSoftPresets[i];
    }
    pContext->config.inputCfg.channels = plvmConfigParams->nrChannels;
    ret = LvmBundle_init(pContext, params);
    if (ret < 0) return ret;

    return 0;
}

int lvmControl(EffectContext *pContext,
               lvmConfigParams_t    *plvmConfigParams,
               LVM_ControlParams_t  *params) 
{
    LVM_ReturnStatus_en LvmStatus = LVM_SUCCESS; /* Function call status */

    /* Set the initial process parameters */
    /* General parameters */
    params->OperatingMode = LVM_MODE_ON;
    params->SpeakerType = LVM_HEADPHONES;
    params->ChMask     = plvmConfigParams->chMask;
    params->NrChannels = plvmConfigParams->nrChannels;
    if (params->NrChannels == 1) 
    {
    params->SourceFormat = LVM_MONO;
    } 
    else if (params->NrChannels == 2) 
    {
    params->SourceFormat = LVM_STEREO;
    } 
    else if (params->NrChannels > 2 && params->NrChannels <= 8) 
    { // FCC_2 FCC_8
    params->SourceFormat = LVM_MULTICHANNEL;
    } 
    else {
        return -EINVAL;
    }

    LVM_Fs_en sampleRate;
    switch (plvmConfigParams->samplingFreq) 
    {
        case 8000:
            sampleRate = LVM_FS_8000;
            break;
        case 11025:
            sampleRate = LVM_FS_11025;
            break;
        case 12000:
            sampleRate = LVM_FS_12000;
            break;
        case 16000:
            sampleRate = LVM_FS_16000;
            break;
        case 22050:
            sampleRate = LVM_FS_22050;
            break;
        case 24000:
            sampleRate = LVM_FS_24000;
            break;
        case 32000:
            sampleRate = LVM_FS_32000;
            break;
        case 44100:
            sampleRate = LVM_FS_44100;
            break;
        case 48000:
            sampleRate = LVM_FS_48000;
            break;
        default:
            return -EINVAL;
    }
    params->SampleRate = sampleRate;

    /* Concert Sound parameters */
    params->VirtualizerOperatingMode = plvmConfigParams->csEnable;
    params->VirtualizerType = LVM_CONCERTSOUND;
    params->VirtualizerReverbLevel = 100;
    params->CS_EffectLevel = LVM_CS_EFFECT_HIGH;

    /* N-Band Equaliser parameters */
    const int eqPresetLevel = plvmConfigParams->eqPresetLevel;
    LVM_EQNB_BandDef_t BandDefs[MAX_NUM_BANDS];  /* Equaliser band definitions */
    for (int i = 0; i < FIVEBAND_NUMBANDS; i++) 
    {
    BandDefs[i].Frequency = EQNB_5BandPresetsFrequencies[i];
    BandDefs[i].QFactor = EQNB_5BandPresetsQFactors[i];
    BandDefs[i].Gain = EQNB_5BandNormalPresets[(FIVEBAND_NUMBANDS * eqPresetLevel) + i];
    }
    params->EQNB_OperatingMode = plvmConfigParams->eqEnable;
    params->pEQNB_BandDefinition = &BandDefs[0];

    /* Volume Control parameters */
    params->VC_EffectLevel = 0;
    params->VC_Balance = plvmConfigParams->vcBal;

    /* Treble Enhancement parameters */
    params->TE_OperatingMode = plvmConfigParams->trebleEnable;

    /* PSA Control parameters */
    params->PSA_Enable = LVM_PSA_OFF;

    /* Bass Enhancement parameters */
    params->BE_OperatingMode = plvmConfigParams->bassEnable;
    params->BE_EffectLevel = plvmConfigParams->bassEffectLevel;

    /* Activate the initial settings */
    LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance, params);
    if (LvmStatus != LVM_SUCCESS) return -EINVAL;

    LvmStatus = LVM_ApplyNewSettings(pContext->pBundledContext->hInstance);
    if (LvmStatus != LVM_SUCCESS) return -EINVAL;
    
    return 0;
}

int lvmExecute(float *floatIn, float *floatOut, EffectContext *pContext,
               lvmConfigParams_t *plvmConfigParams) 
{
    const int frameLength = plvmConfigParams->frameLength;
    return LVM_Process(pContext->pBundledContext->hInstance, /* Instance handle */
                    floatIn,                              /* Input buffer */
                    floatOut,                             /* Output buffer */
                    (LVM_UINT16)frameLength, /* Number of samples to read */
                    0);                      /* Audio Time */
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
    
int lvmMainProcess(EffectContext *pContext,
                   LVM_ControlParams_t *pParams,
                   lvmConfigParams_t *plvmConfigParams,
                   FILE *finp,
                   FILE *fout) 
{
    int errCode = lvmControl(pContext, plvmConfigParams, pParams);
    if (errCode) 
    {
        printf("Error: lvmControl returned with %d\n", errCode);
        return errCode;
    }

    const int channelCount = plvmConfigParams->nrChannels;
    const int frameLength = plvmConfigParams->frameLength;
    const int frameSize = channelCount * sizeof(float); 
    const int ioChannelCount = plvmConfigParams->fChannels;
    const int ioFrameSize = ioChannelCount * sizeof(short); 
    const int maxChannelCount = channelCount > ioChannelCount?channelCount:ioChannelCount;
    short *in = (short*)calloc(frameLength * maxChannelCount, sizeof(short));
    short *out = (short*)calloc(frameLength * maxChannelCount, sizeof(short));
    float *floatIn = (float*)calloc(frameLength * maxChannelCount, sizeof(float));
    float *floatOut = (float*)calloc(frameLength * maxChannelCount, sizeof(float));

    int frameCounter = 0;
    while (fread(in, ioFrameSize, frameLength, finp) == (size_t)frameLength) 
    {
        if (ioChannelCount != channelCount)
        {
            printf("ioChannelCount must be equal to channelCount");
            return -1;
            // adjust_channels(in, ioChannelCount, in, channelCount, sizeof(short), frameLength * ioFrameSize);
        }
        memcpy_to_float_from_i16(floatIn, in, frameLength * channelCount);

        // Mono mode will replicate the first channel to all other channels.
        // This ensures all audio channels are identical. This is useful for testing
        // Bass Boost, which extracts a mono signal for processing.
        if (plvmConfigParams->monoMode && channelCount > 1) 
        {
            for (int i = 0; i < frameLength; ++i) 
            {
                float* fp = &floatIn[i * channelCount];
                memcpy(fp+1, fp, sizeof(float));// replicate ch 0
            }
        }
    #ifndef BYPASS_EXEC
        errCode = lvmExecute(floatIn, floatOut, pContext, plvmConfigParams);
        if (errCode) 
        {
            printf("\nError: lvmExecute returned with %d\n", errCode);
            return errCode;
        }

        (void)frameSize;  // eliminate warning
    #else
        memcpy(floatOut, floatIn, frameLength * frameSize);
    #endif
        memcpy_to_i16_from_float(out, floatOut, frameLength * channelCount);
        if (ioChannelCount != channelCount) 
        {
            // adjust_channels(out, channelCount, out, ioChannelCount, sizeof(short), frameLength * channelCount * sizeof(short));
            printf("ioChannelCount must be equal to channelCount");
            return -1;
        }
        (void)fwrite(out, ioFrameSize, frameLength, fout);
        frameCounter += frameLength;
    }
    printf("frameCounter: [%d]\n", frameCounter);
    return 0;
}

int main() 
{

  /*Config*/
  const char infile[] = "D:/audio_effects_test/music.pcm";
  const char outfile[] ="D:/audio_effects_test/music_cs_out.pcm";

  lvmConfigParams_t lvmConfigParams; 
  lvmConfigParams.samplingFreq    = 16000;
  // only support stereo for this demo
  lvmConfigParams.nrChannels      = 2;
  lvmConfigParams.chMask          = AUDIO_CHANNEL_OUT_STEREO;
  lvmConfigParams.monoMode        = 0;
  lvmConfigParams.fChannels       = 2;
  //balance level -96~96
  lvmConfigParams.vcBal           = 0;
  //bassboost effect level 0~15
  lvmConfigParams.bassEffectLevel = 0;
  //eq preset 0~10
  lvmConfigParams.eqPresetLevel   = 0;
  lvmConfigParams.frameLength     = 256;

  // Switch
  lvmConfigParams.bassEnable      = LVM_BE_ON;
  lvmConfigParams.trebleEnable    = LVM_TE_ON;
  lvmConfigParams.eqEnable        = LVM_EQNB_ON;
  lvmConfigParams.csEnable        = LVM_MODE_ON;
  /*Config End*/

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

  EffectContext context;
  LVM_ControlParams_t params;
  int errCode = lvmCreate(&context, &lvmConfigParams, &params);
  if (errCode == 0) 
  {
    errCode = lvmMainProcess(&context, &params, &lvmConfigParams, finp, fout);
    if (errCode != 0)  printf("Error: lvmMainProcess returned with the error: %d",errCode);
  } 
  else printf("Error: lvmCreate returned with the error: %d", errCode);
  
  fclose(finp);
  fclose(fout);
  /* Free the allocated buffers */
  if (context.pBundledContext != NULL) 
  {
    if (context.pBundledContext->hInstance != NULL) LvmEffect_free(&context);
    free(context.pBundledContext);
  }
  if (errCode) return -1;

  return 0;
}
