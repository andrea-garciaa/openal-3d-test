# openal-3d-test

An OGG vorbis file player, with sound spatialization capabitilies for mono sounds (3D on stereo not supported)

# Install dependencies:
    sudo apt install libsdl2-2.0.0 libsdl2-dev libogg-dev libvorbis-dev

# Compile:
    gcc main.c sound.c -lSDL2 -lopenal -logg -lvorbis -lvorbisfile -o openal-3d-test

# Usage:
    openal-3d-test <oggfilename>

    If a window shows up, hover it with your cursor to move the listener position.

    Click to set the sound emitter position to the cursor position.

    By default, the sound position is 0,0.

    Type help to see the available commands you can use without any window manager.