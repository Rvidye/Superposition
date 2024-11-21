#include<AudioPlayer.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <Log.h>

namespace AMC {

    AudioPlayer::AudioPlayer() : device(nullptr), context(nullptr), sourceID(0), isPlaying(false), isMuted(false) 
    {
        // Initialize OpenAL Device and Context
        device = alcOpenDevice(nullptr);
        if (!device) {
            LOG_ERROR(L"Failed to open audio device.");
        }
        context = alcCreateContext(device, nullptr);
        if (!alcMakeContextCurrent(context)) {
            LOG_ERROR(L"Failed to make audio context current.");
        }
    }

    AudioPlayer::~AudioPlayer() 
    {
        // Clean up OpenAL resources
        if (sourceID) {
            alDeleteSources(1, &sourceID);
        }
        if (context) {
            alcDestroyContext(context);
        }
        if (device) {
            alcCloseDevice(device);
        }
    }

    bool AudioPlayer::initializeAudio(const std::string& audio_file) 
    {
        // Load WAV file data
        int channels, sampleRate, bitsPerSample, size;
        char* data = loadWAV(audio_file, &channels, &sampleRate, &bitsPerSample, &size);
        if (!data) {
            LOG_ERROR(L"Failed to load WAV file: ");
            return false;
        }

        // Generate OpenAL Buffer
        ALuint bufferID;
        alGenBuffers(1, &bufferID);

        ALenum format;
        if (channels == 1) {
            format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        }
        else {
            format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        }

        alBufferData(bufferID, format, data, size, sampleRate);

        // Generate OpenAL Source
        alGenSources(1, &sourceID);
        alSourcei(sourceID, AL_BUFFER, bufferID);

        // Clean up buffer data
        alDeleteBuffers(1, &bufferID);
        free(data);
        return true;
    }

    void AudioPlayer::play() 
    {
        alSourcePlay(sourceID);
        isPlaying = true;
    }

    void AudioPlayer::pause() 
    {
        if (isPlaying) {
            alSourcePause(sourceID);
            isPlaying = false;
        }
    }

    void AudioPlayer::resume() 
    {
        if (!isPlaying) {
            alSourcePlay(sourceID);
            isPlaying = true;
        }
    }

    void AudioPlayer::stop() 
    {
        alSourceStop(sourceID);
        isPlaying = false;
    }

    void AudioPlayer::togglePlayback() 
    {
        if (isPlaying) {
            pause();
        }
        else {
            resume();
        }
    }

    void AudioPlayer::toggleMute() 
    {
        if (isMuted) {
            alSourcef(sourceID, AL_GAIN, 1.0f);
            isMuted = false;
        }
        else {
            alSourcef(sourceID, AL_GAIN, 0.0f);
            isMuted = true;
        }
    }

    void AudioPlayer::seek(float seconds) 
    {
        alSourcef(sourceID, AL_SEC_OFFSET, seconds);
    }

    void AudioPlayer::skipForward(float seconds) 
    {
        float newPosition = getCurrentPosition() + seconds;
        // Optionally, clamp newPosition to the duration of the audio
        seek(newPosition);
    }

    void AudioPlayer::rewind(float seconds) 
    {
        float newPosition = getCurrentPosition() - seconds;
        if (newPosition < 0.0f) newPosition = 0.0f;
        seek(newPosition);
    }

    float AudioPlayer::getCurrentPosition() 
    {
        float position = 0.0f;
        alGetSourcef(sourceID, AL_SEC_OFFSET, &position);
        return position;
    }

    char* AudioPlayer::loadWAV(const std::string& filename, int* channels, int* samplerate, int* bitsPerSample, int* size) 
    {
        // Open the file
        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile) {
            LOG_ERROR(L"Unable to open WAV file:");
            return nullptr;
        }

        // Read the header
        char buffer[4];
        inFile.read(buffer, 4);
        if (std::strncmp(buffer, "RIFF", 4) != 0) {
            LOG_ERROR(L"Invalid WAV file format.");
            return nullptr;
        }

        inFile.seekg(22);
        inFile.read(buffer, 2);
        *channels = convertToInt(buffer, 2);

        inFile.read(buffer, 4);
        *samplerate = convertToInt(buffer, 4);

        inFile.seekg(34);
        inFile.read(buffer, 2);
        *bitsPerSample = convertToInt(buffer, 2);

        // Find the data chunk
        inFile.seekg(40);
        inFile.read(buffer, 4);
        *size = convertToInt(buffer, 4);

        // Read the data
        char* data = (char*)malloc(*size);
        inFile.read(data, *size);
        inFile.close();

        return data;
    }

    int AudioPlayer::convertToInt(char* buffer, int length) 
    {
        int value = 0;
        if (isBigEndian()) {
            for (int i = 0; i < length; i++) {
                ((char*)&value)[3 - i] = buffer[i];
            }
        }
        else {
            std::memcpy(&value, buffer, length);
        }
        return value;
    }

    bool AudioPlayer::isBigEndian() 
    {
        int num = 1;
        return (*(char*)&num) == 0;
    }
}