#include "wc1.h"

#include "video_internal.h"

static SDL_Renderer *g_pSdlRenderer;
static unsigned int g_dwSdlStaticNoiseSeed = 0x1f123bb5U;
static SDL_Texture *g_pSdlFrameTexture;
static Uint32 g_adwSdlFramePixels[
    SDL_PORT_FRAME_WIDTH * SDL_PORT_FRAME_HEIGHT];

/* EGA dither filter (--ega). Not a retail feature -- WC1's real EGA mode
 * has never been reconstructed here; this reproduces the *look* of it on
 * top of the VGA rendering path this project does reconstruct.
 *
 * The table and algorithm below come from live-tracing retail WING
 * COMMANDER's own DOS installer (INSTALL.EXE), the program that actually
 * performed the one-time VGA->EGA art conversion at install time on real
 * EGA installs, via a paused DOSBox Staging debugger session (see
 * D:\Git\openprivateer\doc\dosbox_debugger_bridge.md for the bridge this
 * used). Traced live, not guessed:
 *   - INSTALL.EXE's own conversion routine loads a 256-entry table, one
 *     byte per possible VGA palette index, where each byte packs *two*
 *     4-bit EGA color indices (high nibble, low nibble).
 *   - Its inner per-pixel loop looks up this table for the current VGA
 *     index, then does `xor bx, 100h` after every single emitted pixel --
 *     alternating between the table's high-nibble and low-nibble half on
 *     every pixel, seeded per-scanline so the pattern lines up into a
 *     checkerboard rather than plain horizontal stripes.
 *   - This is a genuine 2-color ordered dither using a hand-authored
 *     VGA-index -> EGA-color-pair table, not per-channel RGB math -- a
 *     "nearest EGA color" quantizer would not reproduce this look.
 * The table itself was read directly out of live DOSBox memory mid-
 * conversion (the installer's own in-RAM copy, table selector 0 -- the
 * only selector observed live; if other selectors exist for different
 * image classes they were not captured). The per-scanline checkerboard
 * seed `(x+y)&1` reproduces the confirmed "toggle every pixel" mechanism
 * exactly; the installer's own precise seed computation was not
 * independently re-derived bit-for-bit, so this is the standard ordered-
 * dither seed, not a byte-exact trace of that one instruction sequence. */
static int g_bEgaDitherEnabled;
static unsigned char g_abEgaDitherPixels[
    SDL_PORT_FRAME_WIDTH * SDL_PORT_FRAME_HEIGHT];
static unsigned char g_abEgaDitherPalette[16 * 4];

static const unsigned char g_abEgaDitherTable[256] = {
    0x00, 0x00, 0x00, 0x80, 0x80, 0x88, 0x88, 0x78, 0x78, 0x77, 0x77, 0x77, 0xf7, 0xf7, 0xff, 0xff,
    0xff, 0xff, 0xf7, 0x7f, 0x76, 0x76, 0x76, 0x76, 0x76, 0x86, 0x86, 0x86, 0x08, 0x08, 0x00, 0x00,
    0xff, 0xf7, 0x77, 0x79, 0x99, 0x99, 0x91, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x00, 0x00, 0x00,
    0xff, 0xf7, 0x77, 0x79, 0x79, 0x89, 0x89, 0x81, 0x81, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xec, 0xec, 0xcc, 0xcc, 0xc4, 0xc4, 0xc4,
    0xc4, 0xc4, 0xc4, 0x44, 0x44, 0x44, 0x44, 0x44, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xf7, 0xfd, 0x3d, 0x3d, 0x9d, 0x9d, 0x35, 0x95, 0x95, 0x15, 0x15, 0x15, 0x10, 0x00, 0x00,
    0xff, 0xf7, 0x77, 0x77, 0x76, 0x76, 0x66, 0x66, 0x64, 0x64, 0x06, 0x06, 0x06, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xfc, 0x77, 0xcc, 0xcc, 0xcc, 0xc6, 0xc6, 0x64, 0x64, 0x48, 0x04, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xfb, 0xb7, 0xb7, 0xb3, 0xb3, 0x33, 0x39, 0x39, 0x31, 0x90, 0x11, 0x01, 0x01, 0x00,
    0xff, 0xfe, 0xeb, 0xea, 0xaa, 0xaa, 0xaa, 0xa6, 0x22, 0x26, 0x88, 0x88, 0x80, 0x00, 0x00, 0x00,
    0xff, 0xf7, 0x77, 0x77, 0xc7, 0xc6, 0xd5, 0xd5, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xee, 0xee, 0xea, 0xaa, 0xaa, 0xaa, 0xa2, 0x22, 0x22, 0x22, 0x22, 0x22, 0x82, 0x02, 0x00, 0x00,
    0xff, 0xfa, 0xfa, 0x7a, 0x7a, 0x7a, 0x72, 0x72, 0x72, 0x82, 0x82, 0x88, 0x80, 0x00, 0x00, 0x00,
    0x80, 0x08, 0x82, 0x82, 0x72, 0x7a, 0xa7, 0xeb, 0xfe, 0x86, 0x86, 0x76, 0x76, 0x76, 0xe6, 0xee,
    0xff, 0x77, 0x77, 0x77, 0x76, 0x86, 0x06, 0x06, 0x04, 0x04, 0x46, 0x64, 0x46, 0x66, 0x66, 0x66,
};

/* Standard IBM EGA/VGA 16-color hardware palette, R/G/B 0-255. */
static const unsigned char g_abEgaHardwarePaletteRgb[16][3] = {
    {0x00, 0x00, 0x00}, {0x00, 0x00, 0xaa}, {0x00, 0xaa, 0x00}, {0x00, 0xaa, 0xaa},
    {0xaa, 0x00, 0x00}, {0xaa, 0x00, 0xaa}, {0xaa, 0x55, 0x00}, {0xaa, 0xaa, 0xaa},
    {0x55, 0x55, 0x55}, {0x55, 0x55, 0xff}, {0x55, 0xff, 0x55}, {0x55, 0xff, 0xff},
    {0xff, 0x55, 0x55}, {0xff, 0x55, 0xff}, {0xff, 0xff, 0x55}, {0xff, 0xff, 0xff},
};

void SdlEnableEgaDither(void)
{
    int index;

    g_bEgaDitherEnabled = 1;
    for (index = 0; index < 16; index++) {
        /* Same [B,G,R,pad] layout SdlPresentIndexedFrame reads below. */
        g_abEgaDitherPalette[index * 4 + 0] =
            g_abEgaHardwarePaletteRgb[index][2];
        g_abEgaDitherPalette[index * 4 + 1] =
            g_abEgaHardwarePaletteRgb[index][1];
        g_abEgaDitherPalette[index * 4 + 2] =
            g_abEgaHardwarePaletteRgb[index][0];
        g_abEgaDitherPalette[index * 4 + 3] = 0;
    }
}

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

    if (g_bEgaDitherEnabled && pixels != 0) {
        for (pixel = 0;
             pixel < SDL_PORT_FRAME_WIDTH * SDL_PORT_FRAME_HEIGHT;
             pixel++) {
            int x = pixel % SDL_PORT_FRAME_WIDTH;
            int y = pixel / SDL_PORT_FRAME_WIDTH;
            unsigned char packed = g_abEgaDitherTable[pixels[pixel]];

            g_abEgaDitherPixels[pixel] =
                ((x + y) & 1) ? (packed >> 4) : (packed & 0x0f);
        }
        pixels = g_abEgaDitherPixels;
        palette = g_abEgaDitherPalette;
    }
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
