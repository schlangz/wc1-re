#include "wc1.h"

#include "video_internal.h"

#include <limits.h>
#include <string.h>

static int CheckViewport(int width, int height, int expectedLeft,
                         int expectedBottom, int expectedWidth,
                         int expectedHeight)
{
    int bottom;
    int left;
    int viewportHeight;
    int viewportWidth;

    Wc1SdlCalculateOutputViewport(width, height, &left, &bottom,
                                  &viewportWidth, &viewportHeight);
    if (left == expectedLeft && bottom == expectedBottom &&
        viewportWidth == expectedWidth && viewportHeight == expectedHeight)
        return 1;
    fprintf(stderr,
            "Viewport %dx%d was %d,%d %dx%d; expected %d,%d %dx%d.\n",
            width, height, left, bottom, viewportWidth, viewportHeight,
            expectedLeft, expectedBottom, expectedWidth, expectedHeight);
    return 0;
}

static int CheckCoordinateMapping(SDL_Window *window,
                                  Wc1SdlVideoBackend backend)
{
    int coordinate;
    int logicalX;
    int logicalY;
    int windowX;
    int windowY;

    Wc1SdlSetVideoBackend(backend);
    if (!Wc1SdlMapLogicalToWindow(window, 0, 0, &windowX, &windowY) ||
        windowX != 33 || windowY != 0)
        return 0;
    if (!Wc1SdlMapLogicalToWindow(window, 160, 100, &windowX, &windowY) ||
        windowX != 500 || windowY != 350)
        return 0;
    if (!Wc1SdlMapWindowToLogical(window, 500, 350, &logicalX, &logicalY) ||
        logicalX != 160 || logicalY != 100)
        return 0;

    coordinate = 0;
    while (coordinate < WC1_SDL_FRAME_WIDTH) {
        if (!Wc1SdlMapLogicalToWindow(window, coordinate, 100,
                                      &windowX, &windowY) ||
            !Wc1SdlMapWindowToLogical(window, windowX, windowY,
                                      &logicalX, &logicalY) ||
            logicalX != coordinate || logicalY != 100)
            return 0;
        coordinate++;
    }
    coordinate = 0;
    while (coordinate < WC1_SDL_FRAME_HEIGHT) {
        if (!Wc1SdlMapLogicalToWindow(window, 160, coordinate,
                                      &windowX, &windowY) ||
            !Wc1SdlMapWindowToLogical(window, windowX, windowY,
                                      &logicalX, &logicalY) ||
            logicalX != 160 || logicalY != coordinate)
            return 0;
        coordinate++;
    }
    if (!Wc1SdlMapWindowToLogical(window, 0, 350, &logicalX, &logicalY) ||
        logicalX >= 0)
        return 0;
    if (!Wc1SdlMapWindowToLogical(window, 999, 350, &logicalX, &logicalY) ||
        logicalX < WC1_SDL_FRAME_WIDTH)
        return 0;
    return 1;
}

static int ReadArgbPixel(SDL_Renderer *renderer, int x, int y, Uint32 *pixel)
{
    SDL_Rect sample;

    sample.x = x;
    sample.y = y;
    sample.w = 1;
    sample.h = 1;
    return SDL_RenderReadPixels(renderer, &sample, SDL_PIXELFORMAT_ARGB8888,
                                pixel, (int)sizeof(*pixel)) == 0;
}

static int CheckIndexedPresentation(SDL_Window *window)
{
    unsigned char palette[256 * 4];
    unsigned char pixels[WC1_SDL_FRAME_WIDTH * WC1_SDL_FRAME_HEIGHT];
    SDL_Renderer *renderer;
    Uint32 pixel;
    int result;

    memset(palette, 0, sizeof(palette));
    memset(pixels, 1, sizeof(pixels));
    palette[4] = 0xff;
    palette[5] = 0xff;
    palette[6] = 0xff;
    Wc1SdlSetVideoBackend(WC1_SDL_VIDEO_BACKEND_INDEXED);
    if (!Wc1SdlInitializeVideo(window))
        return 0;
    renderer = SDL_GetRenderer(window);
    result = renderer != 0 && Wc1SdlPresentIndexedFrame(pixels, palette) &&
             ReadArgbPixel(renderer, 0, 350, &pixel) &&
             (pixel & 0x00ffffffU) == 0;
    result = result && ReadArgbPixel(renderer, 500, 350, &pixel) &&
             (pixel & 0x00ffffffU) == 0x00ffffffU;
    Wc1SdlShutdownVideo();
    return result;
}

static int CheckSpriteCaptureClipping(void)
{
    unsigned int shapeStorage[8];
    unsigned char *shape;
    unsigned char pixels[8];
    unsigned char background[4];
    unsigned char expected[4];
    unsigned short rowOffsets[2];
    Viewport viewport;

    memset(shapeStorage, 0, sizeof(shapeStorage));
    shape = (unsigned char *)shapeStorage;
    shape[4] = 8;
    shape[16] = 4;
    shape[22] = 1;
    shape[23] = 2;

    pixels[0] = 11;
    pixels[1] = 12;
    pixels[2] = 13;
    pixels[3] = 14;
    pixels[4] = 21;
    pixels[5] = 22;
    pixels[6] = 23;
    pixels[7] = 24;
    rowOffsets[0] = 0;
    rowOffsets[1] = 4;
    viewport.pixels = pixels;
    viewport.rowOffsets = rowOffsets;
    viewport.left = 0;
    viewport.top = 0;
    viewport.right = 3;
    viewport.bottom = 1;
    viewport.allocation = pixels;

    shape[20] = 0xff;
    shape[21] = 0xff;
    memset(background, 0x5a, sizeof(background));
    memset(expected, 0x5a, sizeof(expected));
    CaptureSpriteBackground(&viewport, background, 0, 0, shape, 0);
    if (memcmp(background, expected, sizeof(background)) != 0)
        return 0;

    shape[18] = 0xff;
    shape[19] = 0xff;
    shape[20] = 0;
    shape[21] = 0;
    memset(background, 0, sizeof(background));
    CaptureSpriteBackground(&viewport, background, 0, 0, shape, 0);
    return background[0] == pixels[0];
}

static int CheckSpriteRestoreClipping(void)
{
    unsigned int shapeStorage[8];
    unsigned char *shape;
    unsigned char pixels[8];
    unsigned char background[4];
    unsigned char expected[8];
    unsigned short rowOffsets[2];
    Viewport viewport;

    memset(shapeStorage, 0, sizeof(shapeStorage));
    shape = (unsigned char *)shapeStorage;
    shape[4] = 8;
    shape[16] = 4;
    shape[22] = 1;
    shape[23] = 2;

    memset(pixels, 0x2a, sizeof(pixels));
    memcpy(expected, pixels, sizeof(expected));
    memset(background, 0x6b, sizeof(background));
    rowOffsets[0] = 0;
    rowOffsets[1] = 4;
    viewport.pixels = pixels;
    viewport.rowOffsets = rowOffsets;
    viewport.left = 0;
    viewport.top = 0;
    viewport.right = 3;
    viewport.bottom = 1;
    viewport.allocation = pixels;

    shape[20] = 2;
    RestoreSpriteBackground(&viewport, background, 0, 0, shape, 0);
    if (memcmp(pixels, expected, sizeof(pixels)) != 0)
        return 0;

    shape[20] = 0xff;
    shape[21] = 0xff;
    RestoreSpriteBackground(&viewport, background, 0, 0, shape, 0);
    if (memcmp(pixels, expected, sizeof(pixels)) != 0)
        return 0;

    shape[18] = 0xff;
    shape[19] = 0xff;
    shape[20] = 0;
    shape[21] = 0;
    RestoreSpriteBackground(&viewport, background, 0, 0, shape, 0);
    return pixels[0] == background[0];
}

int main(int argumentCount, char **arguments)
{
    unsigned char tripletPalette[256 * 3];
    unsigned char *pixels;
    unsigned short cached[3];
    short red[3];
    SDL_Window *window;
    int result;

    if (GetFixedOneMillionThunkAlt(0) != 0x3e8000 ||
        GetFixedOneMillionThunk(4) != 0x3e8000)
        return 1;
    if (!CheckSpriteCaptureClipping())
        return 1;
    if (!CheckSpriteRestoreClipping())
        return 1;
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
        return 1;
    window =
        SDL_CreateWindow("WC1 video test", 0, 0, 1000, 700,
                         SDL_WINDOW_HIDDEN);
    if (window == 0) {
        SDL_Quit();
        return 1;
    }
    result = CheckViewport(320, 200, 27, 0, 266, 200) &&
             CheckViewport(960, 600, 80, 0, 800, 600) &&
             CheckViewport(1000, 700, 33, 0, 933, 700) &&
             CheckViewport(1920, 1080, 240, 0, 1440, 1080) &&
             CheckViewport(600, 1000, 0, 275, 600, 450) &&
             CheckViewport(0, 0, 0, 0, 1, 1) &&
             CheckViewport(1, 1, 0, 0, 1, 1) &&
             CheckViewport(INT_MAX, INT_MAX, 0, 268435456, INT_MAX,
                           1610612735) &&
             CheckCoordinateMapping(window, WC1_SDL_VIDEO_BACKEND_INDEXED) &&
             CheckCoordinateMapping(
                 window, WC1_SDL_VIDEO_BACKEND_GL_SHARP_BILINEAR) &&
             CheckIndexedPresentation(window);
    if (!result) {
        fprintf(stderr, "SDL video aspect-ratio test failed.\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    DIBinstall((HWND)window);
    stMouseCursorState.viewport = &stScreen;
    pixels = GetDIBPixelBuffer();
    if (pixels == 0 || nDIBWidth != 320 || nDIBHeight != 200)
        return 1;
    red[0] = 255;
    red[1] = 0;
    red[2] = 0;
    DIBsetPalette(1, red);
    GetPaletteEntryAsWords(1, cached);
    if (cached[0] != 255 || cached[1] != 0 || cached[2] != 0)
        return 1;
    pixels[0] = 1;
    pixels[319] = 1;
    pixels[320 * 199] = 1;
    pixels[320 * 200 - 1] = 1;
    DIBslam();
    DIBslamReal();
    if (bDIBSlamPending != 0)
        return 1;
    DIBupdate(0, 0, 319, 199);
    memset(tripletPalette, 0, sizeof(tripletPalette));
    tripletPalette[2 * 3 + 1] = 127;
    DIBwholePaletteFromTriplets(tripletPalette);
    GetPaletteEntryAsWords(2, cached);
    if (cached[0] != 0 || cached[1] != 127 || cached[2] != 0)
        return 1;
    DIBunInstall();
    stMouseCursorState.viewport = 0;
    if (GetDIBPixelBuffer() != 0)
        return 1;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
