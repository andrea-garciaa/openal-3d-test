#ifndef _SOUND_H
#define _SOUND_H

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <AL/al.h>

typedef struct
{
    OggVorbis_File vf;
    ALuint bufferID;
    ALuint sourceID;
    char* name;
} Sound;

int soundOpen(const char* oggfilename, Sound* dest);
int soundClose(Sound* sndHandle);

#endif