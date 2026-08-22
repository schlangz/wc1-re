#include <SDL.h>

#include <stdio.h>

int main(void)
{
    SDL_version compiledVersion;
    SDL_version linkedVersion;

    SDL_VERSION(&compiledVersion);
    SDL_GetVersion(&linkedVersion);
    if (compiledVersion.major != 2 || linkedVersion.major != 2) {
        fprintf(stderr, "SDL2 version mismatch\n");
        return 1;
    }
    if (SDL_Init(0) != 0) {
        fprintf(stderr, "SDL2 initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Quit();
    return 0;
}
