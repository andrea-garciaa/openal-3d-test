#include <stdlib.h>
#include <string.h>
#include "sound.h"

int soundOpen(const char* oggfilename, Sound* dest)
{
    if (!oggfilename) {
        return NULL;
    }

    if (0 > ov_fopen(oggfilename, &dest->vf)) {
        return NULL;
    }

    alGenBuffers(1, &dest->bufferID);
    alGenSources(1, &dest->sourceID);

    size_t datalen = ov_pcm_total(&dest->vf, -1) * dest->vf.vi->channels * 2;

    void* pcmout = malloc(datalen);
    if (!pcmout) {
        soundClose(dest);
        return NULL;
    }

    for (int size=0,offset=0,sel=0; (size = ov_read(&dest->vf, pcmout + offset, 4096, 0, 2, 1, &sel)) != 0; offset += size);

    alBufferData(dest->bufferID, dest->vf.vi->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcmout, datalen, dest->vf.vi->rate);
    alSourcei(dest->sourceID, AL_BUFFER, dest->bufferID);

    char* tmp = strrchr(oggfilename, '/');
    if (tmp) {
        tmp++;
    } else {
        tmp = oggfilename;
    }

    dest->name = tmp;

    ov_clear(&dest->vf);
    return 1;
}

int soundClose(Sound* sndHandle)
{
    if (sndHandle) {
        if (sndHandle->sourceID != -1) {
            alSourceStop(sndHandle->sourceID);
            alDeleteSources(1, &sndHandle->sourceID);
        }

        if (sndHandle->bufferID != -1) {
            alDeleteBuffers(1, &sndHandle->bufferID);
        }

        return 1;
    }

    sndHandle->sourceID = -1;
    sndHandle->bufferID = -1;
    sndHandle->name = NULL;

    return NULL;
}