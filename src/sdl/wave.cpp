#include "wc1.h"

#define WC1_SDL_WAVE_CENTRE_PAN 64
#define WC1_SDL_WAVE_MAXIMUM_PAN 127

static unsigned short g_nWc1SdlPendingWavePan;
static int g_nWc1SdlPendingWavePanDepth;

extern "C" IxSound *Wc1SdlNewWaveSound(IxSample *sample)
{
    IxSound *sound;

    sound = ix_system_new_sound(sample);
    if (sound != 0 && g_nWc1SdlPendingWavePanDepth != 0)
        sound->ix_system_sound_set_pan(g_nWc1SdlPendingWavePan);
    return sound;
}

extern "C" void Wc1SdlPlayWaveWithPan(
    const char *filename, int looping, int volume, int pan)
{
    unsigned short previousPan;
    int previousDepth;

    if (pan < 0)
        pan = 0;
    else if (pan > WC1_SDL_WAVE_MAXIMUM_PAN)
        pan = WC1_SDL_WAVE_MAXIMUM_PAN;
    previousPan = g_nWc1SdlPendingWavePan;
    previousDepth = g_nWc1SdlPendingWavePanDepth;
    g_nWc1SdlPendingWavePan = (unsigned short)(
        (WC1_SDL_WAVE_CENTRE_PAN - pan) * 0x100);
    g_nWc1SdlPendingWavePanDepth++;
    playWAVE(filename, looping, volume);
    g_nWc1SdlPendingWavePanDepth = previousDepth;
    g_nWc1SdlPendingWavePan = previousPan;
}
