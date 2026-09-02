#include "wc1.h"

#include "video_internal.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
extern __declspec(dllimport) BOOL __stdcall ImmDisableIME(DWORD threadId);
#endif

static int SdlParsePortArguments(int *argumentCount, char **arguments,
                                    int *useEnhancedRenderer)
{
    char *argument;
    int argumentIndex;
    int outputArgumentIndex;

    outputArgumentIndex = 1;
    *useEnhancedRenderer = 0;
    argumentIndex = 1;
    while (argumentIndex < *argumentCount) {
        argument = arguments[argumentIndex];
        if (strcmp(argument, "--enhanced") == 0) {
            *useEnhancedRenderer = 1;
        } else if (strcmp(argument, "--joystick-debug") == 0) {
            SdlEnableJoystickDebug();
        } else if (strcmp(argument, "--joystick-rumble") == 0) {
            SdlEnableJoystickRumble();
        } else if (strncmp(argument, "--joystick-mode=", 16) == 0) {
            if (!SdlSetJoystickMode(argument + 16)) {
                fprintf(stderr, "Unknown joystick mode: %s\n",
                        argument + 16);
                return 0;
            }
        } else if (strncmp(argument, "--joystick-axes=", 16) == 0) {
            if (!SdlSetJoystickAxesMode(argument + 16)) {
                fprintf(stderr, "Unknown joystick axes mode: %s\n",
                        argument + 16);
                return 0;
            }
        } else if (strcmp(argument, "--ega") == 0) {
            SdlEnableEgaDither();
        } else {
            arguments[outputArgumentIndex++] = argument;
        }
        argumentIndex++;
    }
    *argumentCount = outputArgumentIndex;
    arguments[outputArgumentIndex] = 0;
    return 1;
}

static void SdlApplyLegacyArguments(int argumentCount, char **arguments)
{
    const char *argument;
    char command;
    int argumentIndex;

    argumentIndex = 1;
    while (argumentIndex < argumentCount) {
        argument = arguments[argumentIndex];
        if (strcmp(argument, "$#SAGA.EXE") == 0)
            bShowKilrathiSagaCredits = 1;
        command = argument[0] == '-' ? argument[1] : argument[0];
        switch (command) {
        case 'b':
            *(unsigned char *)&bPlayerCollisionResponse = 0;
            break;
        case 'c':
            bCockpitEnabled = 0;
            break;
        case 'f':
            bShowFrameRate = 1;
            break;
        case 'k':
            *(unsigned char *)&bPlayerVulnerable = 0;
            break;
        case 'q':
            bDirectDrawModeCascadeEnabled = 0;
            break;
        default:
            break;
        }
        argumentIndex++;
    }
}

static int SdlRunRuntimeChecks(void)
{
    unsigned char framePalette[256 * 4];
    unsigned char framePixels[
        SDL_PORT_FRAME_WIDTH * SDL_PORT_FRAME_HEIGHT];

    /* Exercise the full presentation contract, including the GL backend's
     * copy of all 256 palette entries. */
    memset(framePixels, 0, sizeof(framePixels));
    memset(framePalette, 0, sizeof(framePalette));
    if (!SdlPresentIndexedFrame(framePixels, framePalette))
        return 1;

    aShipWeapons[1][0] = 2;
    remove_weapon(1, 0);
    if (aShipWeapons[1][0] != 1)
        return 1;

    acShipTarget[0] = -1;
    cTargetDisplayObject = -1;
    nRenderedSpaceFrame = 1;
    DrawTargetRangeReadout();

    aeObjectClass[1] = OBJECT_CLASS_SHIP;
    acObjectOwner[1] = -1;
    nYourWingman = -1;
    send_appropriate_message(1, 0);

    acShipTarget[0] = -1;
    aeShipManeuver[0] = MANEUVER_NONE;
    perform_maneuver(0);

    pSnowStaticSound = ix_system_new_sound(0);
    if (pSnowStaticSound == 0)
        return 1;
    stop_all_sounds();
    if (pSnowStaticSound != 0)
        return 1;

    cCockpitView = 4;
    vdu_polygon(2, 50);
    if (stCockpitBar.left != -99)
        return 1;

    anShipMissionShip[1] = -1;
    aeShipObjective[1] = OBJECTIVE_HOME_BASE;
    strike_mission(1);
    if (aeShipMissionType[1] != MISSION_TYPE_ROUT)
        return 1;

    nCommPortraitIndex = -1;
    FreeCommDisplayResources();
    return 0;
}

int main(int argumentCount, char **arguments)
{
    SDL_Window *window;
    Uint32 windowFlags;
    int checkOnly;
    int gameResult;
    int useEnhancedRenderer;
    int usingDosData;

#ifdef _WIN32
    ImmDisableIME((DWORD)-1);
#endif
    if (!SdlParsePortArguments(&argumentCount, arguments,
                                   &useEnhancedRenderer))
        return 1;
    if (useEnhancedRenderer) {
        SdlSetVideoBackend(
            SDL_PORT_VIDEO_BACKEND_GL_SHARP_BILINEAR);
    }
    checkOnly = argumentCount == 2 && strcmp(arguments[1], "--check") == 0;
    /* The build defines SDL_MAIN_HANDLED, so tell SDL the process is already
     * up rather than letting it rename main() out from under us. */
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER |
                 SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    windowFlags = SDL_WINDOW_RESIZABLE;
    if (!SdlConfigureVideoWindow(&windowFlags)) {
        fprintf(stderr, "SDL video configuration failed: %s\n",
                SDL_GetError());
        SDL_Quit();
        return 1;
    }
    if (checkOnly)
        windowFlags |= SDL_WINDOW_HIDDEN;
    window =
        SDL_CreateWindow("Wing Commander SDL2 port", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 960, 600, windowFlags);
    if (window == 0) {
        fprintf(stderr, "SDL window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    if (useEnhancedRenderer)
        fprintf(stderr,
                "Experimental enhanced rendering enabled "
                "(GL sharp bilinear).\n");

    DIBinstall((HWND)window);
    hMainWindow = (HWND)window;
    SdlStartEventPump();
    if (checkOnly) {
        gameResult = SdlRunRuntimeChecks();
    } else {
        CheckLauncherAndConfig();
        usingDosData = SdlUsingDosData();
        if (usingDosData) {
            /* DOS audio drivers cannot be used by the native SDL2 host. */
            bIxAudioEnabled = 0;
        }
        if (usingDosData || useEnhancedRenderer) {
            if (!SdlInitializeOriginFxAudio(usingDosData)) {
                if (usingDosData) {
                    fprintf(stderr,
                            "DOS audio is unavailable.\n");
                } else {
                    fprintf(stderr,
                            "OriginFX intro music is unavailable.\n");
                }
            }
        }
        SdlApplyLegacyArguments(argumentCount, arguments);
        MonoDebug_install();
        InitializeAudioSystem((HWND)window);
        InitializeAudioStreamer((HWND)window);
        srand((unsigned int)time(0));
        InitGameClockEpoch();
        CreateDebugOverlayConsole(0, (HWND)window, 60, 20);
        nSessionStartTime = (unsigned int)time(0);
        pEventManagerPump = 0;
        SDL_ShowCursor(SDL_DISABLE);
        gameResult = GameMain((short)(argumentCount - 1), arguments);
        SdlSetMouseGrab(0);
        SDL_ShowCursor(SDL_ENABLE);
        DestroyGlobalDebugOverlayConsole();
        if ((dwStreamerState & 1) != 0)
            ix_streamer_destroy();
        ServiceAudioStream();
        SdlShutdownOriginFxAudio();
    }

    DIBunInstall();
    SdlShutdownJoysticks();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return gameResult;
}
