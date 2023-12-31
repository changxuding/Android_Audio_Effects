#ifndef _LVM_TEST_
#define _LVM_TEST_


#include <limits.h>
#include "audio.h"
#include "LVM.h"
#include "audio_effect.h"


#define FIVEBAND_NUMBANDS          5
#define MAX_NUM_BANDS              5
#define MAX_CALL_SIZE              256
#define LVM_MAX_SESSIONS           32
#define LVM_UNUSED_SESSION         INT_MAX
#define BASS_BOOST_CUP_LOAD_ARM9E  150    // Expressed in 0.1 MIPS
#define VIRTUALIZER_CUP_LOAD_ARM9E 120    // Expressed in 0.1 MIPS
#define EQUALIZER_CUP_LOAD_ARM9E   220    // Expressed in 0.1 MIPS
#define VOLUME_CUP_LOAD_ARM9E      0      // Expressed in 0.1 MIPS
#define BUNDLE_MEM_USAGE           25     // Expressed in kB


typedef enum
{
    LVM_BASS_BOOST,
    LVM_VIRTUALIZER,
    LVM_EQUALIZER,
    LVM_VOLUME
} lvm_effect_en;

// Preset configuration.
typedef struct{
    const char * name;
}PresetConfig;

/* BundledEffectContext : One per session */
typedef struct{
    LVM_Handle_t                    hInstance;                /* Instance handle */
    int                             SessionNo;                /* Current session number */
    int                             SessionId;                /* Current session id */
    int                            bVolumeEnabled;           /* Flag for Volume */
    int                            bEqualizerEnabled;        /* Flag for EQ */
    int                            bBassEnabled;             /* Flag for Bass */
    int                            bBassTempDisabled;        /* Flag for Bass to be re-enabled */
    int                            bVirtualizerEnabled;      /* Flag for Virtualizer */
    int                            bVirtualizerTempDisabled; /* Flag for effect to be re-enabled */
    audio_devices_t                 nOutputDevice;            /* Output device for the effect */
    audio_devices_t                 nVirtualizerForcedDevice; /* Forced device virtualization mode*/
    int                             NumberEffectsEnabled;     /* Effects in this session */
    int                             NumberEffectsCalled;      /* Effects called so far */
    int                            firstVolume;              /* No smoothing on first Vol change */
    // Bass Boost
    int                             BassStrengthSaved;        /* Conversion between Get/Set */
    // Equalizer
    int                             CurPreset;                /* Current preset being used */
    // Virtualzer
    int                             VirtStrengthSaved;        /* Conversion between Get/Set */
    // Volume
    int                             levelSaved;     /* for when mute is set, level must be saved */
    int                             positionSaved;
    int                            bMuteEnabled;   /* Must store as mute = -96dB level */
    int                            bStereoPositionEnabled;
    LVM_Fs_en                       SampleRate;
    int                             SamplesPerSecond;
    int                             SamplesToExitCountEq;
    int                             SamplesToExitCountBb;
    int                             SamplesToExitCountVirt;
    effect_buffer_t                 *workBuffer;
    int                             frameCount;
    int32_t                         bandGaindB[FIVEBAND_NUMBANDS];
    int                             volume;
#ifdef SUPPORT_MC
    LVM_INT32                       ChMask;
#endif
}BundledEffectContext;



typedef struct{
    const struct effect_interface_s *itfe;
    effect_config_t                 config;
    lvm_effect_en                   EffectType;
    BundledEffectContext            *pBundledContext;
}EffectContext;

/* enumerated parameter settings for Volume effect */
typedef enum
{
    VOLUME_PARAM_LEVEL,                       // type SLmillibel = typedef SLuint16 (set & get)
    VOLUME_PARAM_MAXLEVEL,                    // type SLmillibel = typedef SLuint16 (get)
    VOLUME_PARAM_MUTE,                        // type SLboolean  = typedef SLuint32 (set & get)
    VOLUME_PARAM_ENABLESTEREOPOSITION,        // type SLboolean  = typedef SLuint32 (set & get)
    VOLUME_PARAM_STEREOPOSITION,              // type SLpermille = typedef SLuint16 (set & get)
} t_volume_params;

static const int PRESET_CUSTOM = -1;

static const uint32_t bandFreqRange[FIVEBAND_NUMBANDS][2] = {
                                       {30000, 120000},
                                       {120001, 460000},
                                       {460001, 1800000},
                                       {1800001, 7000000},
                                       {7000001, 20000000}};

//Note: If these frequencies change, please update LimitLevel values accordingly.
static const LVM_UINT16  EQNB_5BandPresetsFrequencies[] = {
                                       60,           /* Frequencies in Hz */
                                       230,
                                       910,
                                       3600,
                                       14000};

static const LVM_UINT16 EQNB_5BandPresetsQFactors[] = {
                                       96,               /* Q factor multiplied by 100 */
                                       96,
                                       96,
                                       96,
                                       96};

static const LVM_INT16 EQNB_5BandNormalPresets[] = {
                                       3, 0, 0, 0, 3,       /* Normal Preset */
                                       8, 5, -3, 5, 6,      /* Classical Preset */
                                       15, -6, 7, 13, 10,   /* Dance Preset */
                                       0, 0, 0, 0, 0,       /* Flat Preset */
                                       6, -2, -2, 6, -3,    /* Folk Preset */
                                       8, -8, 13, -1, -4,   /* Heavy Metal Preset */
                                       10, 6, -4, 5, 8,     /* Hip Hop Preset */
                                       8, 5, -4, 5, 9,      /* Jazz Preset */
                                      -6, 4, 9, 4, -5,      /* Pop Preset */
                                       10, 6, -1, 8, 10};   /* Rock Preset */

static const LVM_INT16 EQNB_5BandSoftPresets[] = {
                                        3, 0, 0, 0, 3,      /* Normal Preset */
                                        5, 3, -2, 4, 4,     /* Classical Preset */
                                        6, 0, 2, 4, 1,      /* Dance Preset */
                                        0, 0, 0, 0, 0,      /* Flat Preset */
                                        3, 0, 0, 2, -1,     /* Folk Preset */
                                        4, 1, 9, 3, 0,      /* Heavy Metal Preset */
                                        5, 3, 0, 1, 3,      /* Hip Hop Preset */
                                        4, 2, -2, 2, 5,     /* Jazz Preset */
                                       -1, 2, 5, 1, -2,     /* Pop Preset */
                                        5, 3, -1, 3, 5};    /* Rock Preset */

static const PresetConfig gEqualizerPresets[] = {
                                        {"Normal"},
                                        {"Classical"},
                                        {"Dance"},
                                        {"Flat"},
                                        {"Folk"},
                                        {"Heavy Metal"},
                                        {"Hip Hop"},
                                        {"Jazz"},
                                        {"Pop"},
                                        {"Rock"}};


#endif /*_LVM_TEST_*/
