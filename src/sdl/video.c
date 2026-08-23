#include "wc1.h"

#include "video_internal.h"

static SDL_Renderer *g_pSdlRenderer;
static unsigned int g_dwSdlStaticNoiseSeed = 0x1f123bb5U;
static SDL_Texture *g_pSdlFrameTexture;
static Uint32 g_adwSdlFramePixels[
    SDL_PORT_FRAME_WIDTH * SDL_PORT_FRAME_HEIGHT];

int SdlInitializeVideo(SDL_Window *window)
{
    SdlShutdownVideo();
    if (SdlUsingGlRenderer())
        return SdlGlRendererInitialize(window);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    g_pSdlRenderer =
        SDL_CreateRenderer(window, -1,
                           SDL_RENDERER_ACCELERATED |
                               SDL_RENDERER_PRESENTVSYNC);
    if (g_pSdlRenderer == 0)
        g_pSdlRenderer =
            SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (g_pSdlRenderer == 0)
        return 0;
    g_pSdlFrameTexture =
        SDL_CreateTexture(g_pSdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_STREAMING,
                          SDL_PORT_FRAME_WIDTH,
                          SDL_PORT_FRAME_HEIGHT);
    if (g_pSdlFrameTexture == 0) {
        SdlShutdownVideo();
        return 0;
    }
    SDL_SetRenderDrawColor(g_pSdlRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(g_pSdlRenderer);
    SDL_RenderPresent(g_pSdlRenderer);
    return 1;
}

static int SdlComputeFrameDestRect(SDL_Rect *dest)
{
    int bottom;
    int height;
    int width;

    if (SDL_GetRendererOutputSize(g_pSdlRenderer, &width, &height) != 0 ||
        width < 1 || height < 1)
        return 0;
    SdlCalculateOutputViewport(width, height, &dest->x, &bottom, &dest->w,
                                  &dest->h);
    dest->y = height - bottom - dest->h;
    return 1;
}

void SdlShutdownVideo(void)
{
    SdlGlRendererShutdown();
    if (g_pSdlFrameTexture != 0) {
        SDL_DestroyTexture(g_pSdlFrameTexture);
        g_pSdlFrameTexture = 0;
    }
    if (g_pSdlRenderer != 0) {
        SDL_DestroyRenderer(g_pSdlRenderer);
        g_pSdlRenderer = 0;
    }
}

int SdlPresentIndexedFrame(const unsigned char *pixels,
                              const unsigned char *palette)
{
    SDL_Rect dest;
    int pixel;

    if (SdlUsingGlRenderer())
        return SdlGlRendererPresent(pixels, palette);
    if (g_pSdlRenderer == 0 || g_pSdlFrameTexture == 0 || pixels == 0 ||
        palette == 0 || !SdlComputeFrameDestRect(&dest))
        return 0;
    pixel = 0;
    while (pixel < SDL_PORT_FRAME_WIDTH * SDL_PORT_FRAME_HEIGHT) {
        int paletteOffset;
        unsigned char colour;

        colour = pixels[pixel];
        paletteOffset = colour * 4;
        g_adwSdlFramePixels[pixel] =
            0xff000000U |
            (Uint32)palette[paletteOffset + 2] << 16 |
            (Uint32)palette[paletteOffset + 1] << 8 |
            palette[paletteOffset];
        pixel++;
    }
    if (SDL_UpdateTexture(g_pSdlFrameTexture, 0, g_adwSdlFramePixels,
                          SDL_PORT_FRAME_WIDTH *
                              (int)sizeof(Uint32)) != 0)
        return 0;
    if (SDL_RenderClear(g_pSdlRenderer) != 0)
        return 0;
    if (SDL_RenderCopy(g_pSdlRenderer, g_pSdlFrameTexture, 0, &dest) != 0)
        return 0;
    SDL_RenderPresent(g_pSdlRenderer);
    return 1;
}

void SdlWaitForVerticalBlank(void)
{
    SDL_Rect dest;

    if (SdlUsingGlRenderer()) {
        SdlGlRendererWaitForVerticalBlank();
        return;
    }
    if (g_pSdlRenderer == 0 || g_pSdlFrameTexture == 0) {
        SDL_Delay(1);
        return;
    }
    if (!SdlComputeFrameDestRect(&dest)) {
        SDL_Delay(1);
        return;
    }
    SDL_RenderClear(g_pSdlRenderer);
    SDL_RenderCopy(g_pSdlRenderer, g_pSdlFrameTexture, 0, &dest);
    SDL_RenderPresent(g_pSdlRenderer);
}

void SdlBeginSpaceFrame(
    const struct ScreenViewportGeometry *geometry, int viewportMode,
    int fullViewportCopy, unsigned char clearColour)
{
    if (SdlUsingGlRenderer()) {
        SdlGlRendererBeginSpaceFrame(
            geometry, viewportMode, fullViewportCopy, clearColour);
    }
}

void SdlCompleteSpaceFrame(void)
{
    if (SdlUsingGlRenderer())
        SdlGlRendererCompleteSpaceFrame();
}

void SdlCancelSpaceFrame(void)
{
    /* RunSpaceFlight calls this when the spaceflight session ends. */
    SdlEndJoystickSpaceflight();
    if (SdlUsingGlRenderer())
        SdlGlRendererCancelSpaceFrame();
}

int SdlRecordSpaceSprite(
    const struct Viewport *viewport, float x, float y,
    unsigned char *shape, short frame, short angle, short scale,
    short flip)
{
    if (!SdlUsingGlRenderer())
        return 0;
    return SdlGlRendererRecordSpaceSprite(
        viewport, x, y, shape, frame, angle, scale, flip);
}

/* Restores the display static the Kilrathi Saga port dropped.  Retail's
 * snow_viewport only slams the DIB and calls the empty RasterLineHook marker,
 * so a knocked-out display shows nothing while malf_noise still plays the
 * sound.  The DOS rasteriser's exact pattern is not recovered here; this
 * scatters noise over the viewport to restore the player-facing cue.
 *
 * The generator is local on purpose.  Drawing thousands of pixels through the
 * game's RandomBelowOrEqual would consume its sequence and change gameplay,
 * so the port keeps its own. */
static unsigned int SdlNextStaticNoise(void)
{
    g_dwSdlStaticNoiseSeed ^= g_dwSdlStaticNoiseSeed << 13;
    g_dwSdlStaticNoiseSeed ^= g_dwSdlStaticNoiseSeed >> 17;
    g_dwSdlStaticNoiseSeed ^= g_dwSdlStaticNoiseSeed << 5;
    return g_dwSdlStaticNoiseSeed;
}

void SdlDrawViewportStatic(struct Viewport *viewport, int effect,
                              unsigned short colour)
{
    unsigned int sample;
    short bright;
    short x;
    short y;

    if (viewport == 0 || viewport->pixels == 0)
        return;
    /* The two call sites use effect 1 for a damaged display and 3 for the
     * heavier comm dropout, so the busier pattern goes to the latter. */
    bright = (short)(effect >= 3 ? 1 : 2);
    for (y = viewport->top; y <= viewport->bottom; y++) {
        for (x = viewport->left; x <= viewport->right; x++) {
            sample = SdlNextStaticNoise() >> 16;
            if ((short)(sample & 3) > bright)
                continue;
            DrawViewportPixel(viewport, x, y,
                              (short)((sample & 4) != 0 ? colour
                                                        : cBlackColour));
        }
    }
}
