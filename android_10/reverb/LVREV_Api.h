#ifndef _EFFECTREVERB_H_
#define _EFFECTREVERB_H_


#include <stdint.h>
#include "LVREV.h"
#include "audio_effect.h"
#include "effect_environmentalreverb.h"
#include "effect_presetreverb.h"
/*
CIRCULAR() calculates the array index using modulo arithmetic.
The "trick" is that modulo arithmetic is simplified by masking
the effective address where the mask is (2^n)-1. This only works
if the buffer size is a power of two.
*/
  
#define MAX_NUM_BANDS 5
#define MAX_CALL_SIZE 256
#define LVREV_MAX_T60 7000
#define LVREV_MAX_REVERB_LEVEL 2000
#define LVREV_MAX_FRAME_SIZE 2560
#define LVREV_CUP_LOAD_ARM9E 470                            // Expressed in 0.1 MIPS
#define LVREV_MEM_USAGE (71 + (LVREV_MAX_FRAME_SIZE >> 7))  // Expressed in kB

typedef struct _LPFPair_t {
    int16_t Room_HF;
    int16_t LPF;
} LPFPair_t;

#define CIRCULAR(base,offset,size) (uint32_t)(               \
            (                                               \
                ((int32_t)(base)) + ((int32_t)(offset))     \
            )                                               \
            & size                                          \
                                            )

#define NUM_OUTPUT_CHANNELS 2
#define OUTPUT_CHANNELS AUDIO_CHANNEL_OUT_STEREO
#define REVERB_BUFFER_SIZE_IN_SAMPLES_MAX   16384
#define REVERB_NUM_PRESETS  REVERB_PRESET_PLATE   // REVERB_PRESET_NONE is not included
#define REVERB_MAX_NUM_REFLECTIONS      5   // max num reflections per channel
#define REVERB_XFADE_PERIOD_IN_SECONDS      (double) (100.0 / 1000.0)        // xfade once every this many seconds

/**********/
/* the entire synth uses various flags in a bit field */

/* if flag is set, synth reset has been requested */
#define REVERB_FLAG_RESET_IS_REQUESTED          0x01    /* bit 0 */
#define MASK_REVERB_RESET_IS_REQUESTED          0x01
#define MASK_REVERB_RESET_IS_NOT_REQUESTED      (uint32_t)(~MASK_REVERB_RESET_IS_REQUESTED)

/*
by default, we always want to update ALL channel parameters
when we reset the synth (e.g., during GM ON)
*/
#define DEFAULT_REVERB_FLAGS                    0x0

/* coefficients for generating sin, cos */
#define REVERB_PAN_G2   4294940151          /* -0.82842712474619 = 2 - 4/sqrt(2) */
/*
int32_t nPanG1 = +1.0 for sin
int32_t nPanG1 = -1.0 for cos
*/
#define REVERB_PAN_G0   23170               /* 0.707106781186547 = 1/sqrt(2) */

/*************************************************************/
// define the input injection points
#define GUARD               5                       // safety guard of this many samples

#define MAX_AP_TIME         (int) ((20*65536)/1000)  // delay time in time units (65536th of sec)
#define MAX_DELAY_TIME      (int) ((65*65536)/1000)  // delay time in time units
#define MAX_EARLY_TIME      (int) ((65*65536)/1000)  // delay time in time units

#define AP0_IN              0


#define REVERB_DEFAULT_ROOM_NUMBER      1       // default preset number
#define DEFAULT_AP0_GAIN                19400
#define DEFAULT_AP1_GAIN                -19400

#define REVERB_DEFAULT_WET              32767
#define REVERB_DEFAULT_DRY              0

#define REVERB_WET_MAX              32767
#define REVERB_WET_MIN              0
#define REVERB_DRY_MAX              32767
#define REVERB_DRY_MIN              0

// constants for reverb density
// The density expressed in permilles changes the Allpass delay in a linear manner in the range defined by
// AP0_TIME_BASE to AP0_TIME_BASE + AP0_TIME_RANGE
#define AP0_TIME_BASE (int)((9*65536)/1000)
#define AP0_TIME_RANGE (int)((4*65536)/1000)
#define AP1_TIME_BASE (int)((12*65536)/1000)
#define AP1_TIME_RANGE (int)((8*65536)/1000)

// constants for reverb diffusion
// The diffusion expressed in permilles changes the Allpass gain in a linear manner in the range defined by
// AP0_GAIN_BASE to AP0_GAIN_BASE + AP0_GAIN_RANGE
#define AP0_GAIN_BASE (int)(9830)
#define AP0_GAIN_RANGE (int)(19660-9830)
#define AP1_GAIN_BASE (int)(6553)
#define AP1_GAIN_RANGE (int)(22936-6553)


#ifdef BUILD_FLOAT
typedef     float               process_buffer_t; // process in float
#else
typedef     int32_t             process_buffer_t; // process in Q4_27
#endif // BUILD_FLOAT
#define REVERB_DEFAULT_PRESET REVERB_PRESET_NONE

#ifdef BUILD_FLOAT
#define REVERB_SEND_LEVEL   0.75f // 0.75 in 4.12 format
#else
#define REVERB_SEND_LEVEL   (0x0C00) // 0.75 in 4.12 format
#endif
#define REVERB_UNIT_VOLUME  (0x1000) // 1.0 in 4.12 format
#define CHECK_ARG(cond) {                     \
    if (!(cond)) {                            \
        printf("\tLVM_ERROR : Invalid argument: "#cond);      \
        return -EINVAL;                       \
    }                                         \
}

#define LVM_ERROR_CHECK(LvmStatus, callingFunc, calledFunc){\
        if ((LvmStatus) == LVREV_NULLADDRESS){\
            printf("\tLVREV_ERROR : Parameter error - "\
                    "null pointer returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if ((LvmStatus) == LVREV_INVALIDNUMSAMPLES){\
            printf("\tLVREV_ERROR : Parameter error - "\
                    "bad number of samples returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if ((LvmStatus) == LVREV_OUTOFRANGE){\
            printf("\tLVREV_ERROR : Parameter error - "\
                    "out of range returned by %s in %s\n", callingFunc, calledFunc);\
        }\
    }

enum {
    REVERB_VOLUME_OFF,
    REVERB_VOLUME_FLAT,
    REVERB_VOLUME_RAMP,
};

typedef struct{
    effect_config_t                 config;
    LVREV_Handle_t                  hInstance;
    int16_t                         SavedRoomLevel;
    int16_t                         SavedHfLevel;
    int16_t                         SavedDecayTime;
    int16_t                         SavedDecayHfRatio;
    int16_t                         SavedReverbLevel;
    int16_t                         SavedDiffusion;
    int16_t                         SavedDensity;
    int16_t                         bEnabled;
    LVM_Fs_en                       SampleRate;
    process_buffer_t                *InFrames;
    process_buffer_t                *OutFrames;
    size_t                          bufferSizeIn;
    size_t                          bufferSizeOut;
    int16_t                         auxiliary;
    int16_t                         preset;
    uint16_t                        curPreset;
    uint16_t                        nextPreset;
    int                             SamplesToExitCount;
    LVM_INT16                       leftVolume;
    LVM_INT16                       rightVolume;
    LVM_INT16                       prevLeftVolume;
    LVM_INT16                       prevRightVolume;
    int                             volumeMode;
}ReverbContext;

enum reverb_state_e {
    REVERB_STATE_UNINITIALIZED,
    REVERB_STATE_INITIALIZED,
    REVERB_STATE_ACTIVE,
};

/* parameters for each allpass */
typedef struct
{
    uint16_t             m_zApOut;       // delay offset for ap out
    int16_t             m_nApGain;      // gain for ap
    uint16_t             m_zApIn;        // delay offset for ap in
} allpass_object_t;


/* parameters for early reflections */
typedef struct
{
    uint16_t            m_zDelay[REVERB_MAX_NUM_REFLECTIONS];   // delay offset for ap out
    int16_t             m_nGain[REVERB_MAX_NUM_REFLECTIONS];    // gain for ap
} early_reflection_object_t;

//demo
typedef struct
{
    int16_t             m_nRvbLpfFbk;
    int16_t             m_nRvbLpfFwd;
    int16_t             m_nRoomLpfFbk;
    int16_t             m_nRoomLpfFwd;

    int16_t             m_nEarlyGain;
    int16_t             m_nEarlyDelay;
    int16_t             m_nLateGain;
    int16_t             m_nLateDelay;

    early_reflection_object_t m_sEarlyL;
    early_reflection_object_t m_sEarlyR;

    uint16_t            m_nMaxExcursion; //28
    int16_t             m_nXfadeInterval;

    int16_t             m_nAp0_ApGain; //30
    int16_t             m_nAp0_ApOut;
    int16_t             m_nAp1_ApGain;
    int16_t             m_nAp1_ApOut;
    int16_t             m_nDiffusion;

    int16_t             m_rfu4;
    int16_t             m_rfu5;
    int16_t             m_rfu6;
    int16_t             m_rfu7;
    int16_t             m_rfu8;
    int16_t             m_rfu9;
    int16_t             m_rfu10; //43

} reverb_preset_t;

typedef struct
{
    reverb_preset_t     m_sPreset[REVERB_NUM_PRESETS]; // array of presets(does not include REVERB_PRESET_NONE)

} reverb_preset_bank_t;


/* parameters for each reverb */
typedef struct
{
    /* update counter keeps track of when synth params need updating */
    /* only needs to be as large as REVERB_UPDATE_PERIOD_IN_SAMPLES */
    int16_t             m_nUpdateCounter;
    uint16_t             m_nBaseIndex;                                   // base index for circular buffer

    // reverb delay line offsets, allpass parameters, etc:
    short             m_nRevFbkR;              // combine feedback reverb right out with dry left in
    short             m_zOutLpfL;              // left reverb output
    allpass_object_t    m_sAp0;                     // allpass 0 (left channel)
    uint16_t             m_zD0In;                    // delay offset for delay line D0 in
    short             m_nRevFbkL;              // combine feedback reverb left out with dry right in
    short             m_zOutLpfR;              // right reverb output
    allpass_object_t    m_sAp1;                     // allpass 1 (right channel)
    uint16_t             m_zD1In;                    // delay offset for delay line D1 in

    // delay output taps, notice criss cross order
    uint16_t             m_zD0Self;                  // self feeds forward d0 --> d0
    uint16_t             m_zD1Cross;                 // cross feeds across d1 --> d0
    uint16_t             m_zD1Self;                  // self feeds forward d1 --> d1
    uint16_t             m_zD0Cross;                 // cross feeds across d0 --> d1
    int16_t             m_nSin;                     // gain for self taps
    int16_t             m_nCos;                     // gain for cross taps
    int16_t             m_nSinIncrement;            // increment for gain
    int16_t             m_nCosIncrement;            // increment for gain
    int16_t             m_nRvbLpfFwd;                  // reverb feedback lpf forward gain (includes scaling for mixer)
    int16_t             m_nRvbLpfFbk;                  // reverb feedback lpf feedback gain
    int16_t             m_nRoomLpfFwd;                  // room lpf forward gain (includes scaling for mixer)
    int16_t             m_nRoomLpfFbk;                  // room lpf feedback gain
    uint16_t            m_nXfadeInterval;           // update/xfade after this many samples
    uint16_t            m_nXfadeCounter;            // keep track of when to xfade
    int16_t             m_nPhase;                   // -1 <= m_nPhase < 1
                                                    // but during sin,cos calculations
                                                    // use m_nPhase/2
    int16_t             m_nPhaseIncrement;          // add this to m_nPhase each frame
    int16_t             m_nNoise;                   // random noise sample
    uint16_t            m_nMaxExcursion;            // the taps can excurse +/- this amount
    uint16_t            m_bUseNoise;                // if TRUE, use noise as input signal
    uint16_t            m_bBypass;                  // if TRUE, then bypass reverb and copy input to output
    int16_t             m_nCurrentRoom;             // preset number for current room
    int16_t             m_nNextRoom;                // preset number for next room
    int16_t             m_nEarlyGain;               // gain for early (widen) signal
    int16_t             m_nEarlyDelay;              // initial dealy for early (widen) signal
    int16_t             m_nEarly0in;
    int16_t             m_nEarly1in;
    int16_t             m_nLateGain;               // gain for late reverb
    int16_t             m_nLateDelay;
    int16_t             m_nDiffusion;
    early_reflection_object_t   m_sEarlyL;          // left channel early reflections
    early_reflection_object_t   m_sEarlyR;          // right channel early reflections
    short             m_nDelayLine[REVERB_BUFFER_SIZE_IN_SAMPLES_MAX];    // one large delay line for all reverb elements
    reverb_preset_t     pPreset;
    reverb_preset_bank_t  m_sPreset;

    uint32_t            m_nSamplingRate;
    int32_t             m_nUpdatePeriodInBits;
    int32_t             m_nBufferMask;
    int32_t             m_nUpdatePeriodInSamples;
    int32_t             m_nDelay0Out;
    int32_t             m_nDelay1Out;
    int16_t             m_nCosWT_5KHz;
    uint16_t            m_Aux;                // if TRUE, is connected as auxiliary effect
    uint16_t            m_Preset;             // if TRUE, expose preset revert interface
    uint32_t            mState;
} reverb_object_t;



typedef struct reverb_module_s {
    // const struct effect_interface_s *itfe;
    effect_config_t config;
    reverb_object_t context;
} reverb_module_t;


//--- local function prototypes
int  Reverb_init            (ReverbContext *pContext);
void Reverb_free            (ReverbContext *pContext);
int  Reverb_setConfig       (ReverbContext *pContext, effect_config_t *pConfig);
void Reverb_getConfig       (ReverbContext *pContext, effect_config_t *pConfig);
int  Reverb_setParameter    (ReverbContext *pContext, void *pParam, void *pValue, int vsize);
int  Reverb_getParameter    (ReverbContext *pContext,
                             void          *pParam,
                             uint32_t      *pValueSize,
                             void          *pValue);
int Reverb_LoadPreset       (ReverbContext   *pContext);
int Reverb_paramValueSize   (int32_t param);

#endif /*_EFFECTREVERB_H_*/
