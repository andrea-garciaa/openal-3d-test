#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#else
#include <windows.h>
#pragma comment(lib, "openal32")
#pragma comment(lib, "libogg")
#pragma comment(lib, "libvorbis")
#pragma comment(lib, "libvorbisfile")
#endif
#include <AL/alc.h>
#include "sound.h"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

static void onWindowEvent(SDL_WindowEvent winevent);
static void onMouseMotion(SDL_MouseMotionEvent motion);
static void onMouseScroll(SDL_MouseWheelEvent whevent);
static void onMouseButton(SDL_MouseButtonEvent btnevent);
static void onConsoleInput(const char* buffer, int len);
static void showHelp(void);

static SDL_Window* window;
static int width;
static int height;
static int listenx; // listener position, defined by mouse coordinates
static int listeny;
static int sourcex; // sound emitter position
static int sourcey;
static int base_source_x;
static int base_source_y;
static int base_window_x = DEFAULT_WIDTH;
static int base_window_y = DEFAULT_HEIGHT;
static float volume;
static Sound sound;
static int keeprunning;

int main(int argc, char** argv)
{
    keeprunning = 1;
    int sdlok = SDL_Init(SDL_INIT_EVENTS);
    
    SDL_Event ev;

#ifndef _WIN32
    int stdinfd = fileno(stdin);
#else
    int stdinfd = GetStdHandle(STD_INPUT_HANDLE);
#endif

    if (window = SDL_CreateWindow("OGG Vorbis 3D player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_WIDTH, DEFAULT_HEIGHT, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE)) {
        // set the window size
        ev.type = SDL_WINDOWEVENT;
        ev.window.event = SDL_WINDOWEVENT_RESIZED;
        ev.window.data1 = DEFAULT_WIDTH;
        ev.window.data2 = DEFAULT_HEIGHT;

        SDL_PushEvent(&ev);
        SDL_GetMouseState(&listenx, &listeny);

        // we will read the console input without waiting (blocking) for it (only when using SDL)
#ifndef _WIN32
        int flags = fcntl(stdinfd, F_GETFL, 0);
        fcntl(stdinfd, F_SETFL, flags | O_NONBLOCK);
#else
        /* win32 specific code to make console reading non-blocking */
#endif
    } else {
        listenx = 0;
        listeny = 0;
    }

    ALCdevice* device = alcOpenDevice(NULL);
    ALCcontext* context = alcCreateContext(device, NULL);

    if (!alcMakeContextCurrent(context)) {
        if (sdlok >= 0) {
            // show a message box with SDL
            SDL_MessageBoxData data;
            memset(&data, 0, sizeof data);
            data.title = "Error";
            data.message = "OpenAL context creation failed. Application will close.";
            data.window = window;
            data.flags = SDL_MESSAGEBOX_ERROR;
            SDL_ShowMessageBox(&data, NULL);   
        }

        printf("%s\n", "OpenAL context creation failed. Application will close.");
        return 1;
    }
    
    memset(&sound, 0, sizeof sound);
    sound.bufferID = -1;
    sound.sourceID = -1;
    
    if (argc > 1) {
        soundClose(&sound); // if a sound was playing, stop it before opening another sound file
        if (soundOpen(argv[1], &sound)) {
            if (window) {
                SDL_SetWindowTitle(window, sound.name);
            }
        } else {
            printf("Failed to open: %s\n", argv[1]);
        }
    }
    sourcex = DEFAULT_WIDTH / 2;
    sourcey = DEFAULT_HEIGHT / 2;
    base_source_x = sourcex;
    base_source_y = sourcey;
    volume = 1.0f;

    while (keeprunning) {
        if (sdlok >= 0) {
            // using SDL
            while (SDL_PollEvent(&ev)) {
                switch (ev.type) {
                    case SDL_QUIT:
                        keeprunning = 0;
                        break;
                    case SDL_WINDOWEVENT:
                        onWindowEvent(ev.window);
                        break;
                    case SDL_MOUSEMOTION:
                        onMouseMotion(ev.motion);
                        break;
                    case SDL_MOUSEWHEEL:
                        onMouseScroll(ev.wheel);
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        onMouseButton(ev.button);
                }
            }
        }
        
        char readbuf[256];
#ifndef _WIN32
        int bytesread = read(stdinfd, readbuf, sizeof readbuf);
#else
        int bytesread = ReadFile(stdinfd, readbuf, sizeof readbuf);
#endif
        if (bytesread > 0) {
            onConsoleInput(readbuf, bytesread);
        }

        // set listener position
        alListener3f(AL_POSITION, listenx, listeny, 0);
        ALint state;
        alGetSourcei(sound.sourceID, AL_SOURCE_STATE, &state);

        if (state == AL_STOPPED || state == AL_INITIAL) {
            // set sound position and volume
            //alSourcei(sound.sourceID, AL_SOURCE_RELATIVE, AL_FALSE); // seems false by default
            alSource3i(sound.sourceID, AL_POSITION, sourcex, sourcey, 0);
            alSourcef(sound.sourceID, AL_GAIN, volume);
            // start playback
            alSourcePlay(sound.sourceID);
        }
    }

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    if (window) { 
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}


void onWindowEvent(SDL_WindowEvent winevent)
{
    switch (winevent.event) {
        case SDL_WINDOWEVENT_RESIZED:
            width = winevent.data1;
            height = winevent.data2;
            sourcex = (base_source_x * width) / base_window_x;
            sourcey = (base_source_y * height) / base_window_y;
            //printf("%d = (%d * %d) / %d\n", sourcex, base_source_x, width, base_window_x);
            //printf("%d = (%d * %d) / %d\n", sourcey, base_source_y, height, base_window_y);
            alSourceStop(sound.sourceID);
            printf("window resized to %dx%d\n", width, height);
            printf("window source  %dx%d\n", sourcex, sourcey);
            break;
        default:
            break;
    }
}


void onMouseMotion(SDL_MouseMotionEvent motion)
{
    // update listener coordinates
    listenx = motion.x;
    listeny = motion.y;
}


void onMouseScroll(SDL_MouseWheelEvent whevent)
{
    // volume control
    volume += whevent.y;
}


void onMouseButton(SDL_MouseButtonEvent btnevent)
{
    if (btnevent.state == SDL_PRESSED) {
        switch (btnevent.button) {
            case SDL_BUTTON_LEFT: // set sound source to mouse position
                SDL_GetMouseState(&sourcex, &sourcey);
                base_source_x = sourcex;
                base_source_y = sourcey;
                base_window_x = width;
                base_window_y = height;
                alSourceStop(sound.sourceID);
                break;
        }
    }
}


void onConsoleInput(const char* buffer, int len)
{
    int argc = 0;
    char* argv[3]; // max 2 params

    char* ptr = strtok(buffer, " ");
    while (ptr) {
        argv[argc++] = ptr;
        ptr = strtok(NULL, " ");
    }

    argc++;

    switch (argv[0][0]) {
        case 'o':   // open sound
            if (argc > 1) {
                soundClose(&sound); // if a sound was playing, stop it before opening another sound file
                if (soundOpen(argv[1], &sound)) {
                    if (window) {
                        SDL_SetWindowTitle(window, sound.name);
                    }
                } else {
                    printf("Failed to open: %s\n", argv[1]);
                }   
            }
            break;
        case 'c':   // close sound
            soundClose(&sound);
            break;
        case 'p':   // pause/play sound
        {
            ALint state;
            alGetSourcei(sound.sourceID, AL_SOURCE_STATE, &state);
            if (state == AL_PLAYING) {
                alSourcePause(sound.sourceID);
            } else if (state == AL_PAUSED || state == AL_STOPPED) {
                alSourcePlay(sound.sourceID);
            }
            
            break;
        }
        case 'r':   // reset
            alSourceStop(sound.sourceID);
            break;
        case 'g':   // get sound info
        {
            float sec;
            alGetSourcef(sound.sourceID, AL_SEC_OFFSET, &sec);
            printf("listener: x=%d  y=%d\nsource: x=%d  y=%d\nvolume: %f\ntime: %f\n", listenx, listeny, sourcex, sourcey, volume, sec);
            break;
        }
        case 's':   // set source position
            if (argc > 1) {
                sourcex = atoi(argv[1]);
                if (argc > 2) {
                    sourcey = atoi(argv[2]);
                }
            }
            break;
        case 'l':   // set listener position
            if (argc > 1) {
                listenx = atoi(argv[1]);
                if (argc > 2) {
                    listeny = atoi(argv[2]);
                }
            }
            break;
        case 'v':   // set volume
            if (argc > 1) {
                volume = atof(argv[1]);
            }
            break;
        case 'h':   // help
            showHelp();
            break;
        case 'q':   // quit
            keeprunning = 0;
            break;
        default:
            printf("%s\n", "unknown command; type 'h' to print help.");
    }
}


void showHelp()
{
    puts("Commands:\n\n> h\n\tShow help.\n> o\n\tOpen an OGG file and play it.\n> c\n\tClose the current sound.\n> p\n\tPause/play the current sound.\n> r\n\tRestart the playback.\n> g\n\tPrint informations about the sound.\n> s <x> <y>\n\tSet the sound emitter position.\n> l <x> <y>\n\tSet the listener position.\n> v <value>\n\tSet the volume.\n> q\n\tQuit.\n");
}