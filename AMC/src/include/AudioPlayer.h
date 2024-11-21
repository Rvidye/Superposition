#pragma once

#include<AL/al.h>
#include<AL/alc.h>
#include<string>

namespace AMC {

class AudioPlayer 
{
    public:

        AudioPlayer();
        ~AudioPlayer();

        // Public Methods
        bool initializeAudio(const std::string& audio_file);
        void play();
        void pause();
        void resume();
        void stop();
        void togglePlayback();
        void toggleMute();
        void seek(float seconds);
        void skipForward(float seconds);
        void rewind(float seconds);
        float getCurrentPosition();

    private:
        // Private Methods
        char* loadWAV(const std::string& filename, int* channels, int* samplerate, int* bitsPerSample, int* size);
        int convertToInt(char* buffer, int length);
        bool isBigEndian();

        // Private Members
        ALCdevice* device;
        ALCcontext* context;
        ALuint sourceID;
        bool isPlaying;
        bool isMuted;
};
}