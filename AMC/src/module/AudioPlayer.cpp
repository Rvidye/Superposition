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
        int frameSize = channels * (bitsPerSample / 8);
        size -= size % frameSize;
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
        //if (sourceID) {
        //    ALint bufferID;
        //    alGetSourcei(sourceID, AL_BUFFER, &bufferID);
        //    alSourcei(sourceID, AL_BUFFER, 0); // Detach the buffer
        //    alDeleteBuffers(1, &bufferID); // Now it's safe to delete
        //    alDeleteSources(1, &sourceID);
        //}
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
        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile) {
            LOG_ERROR(L"Unable to open WAV file.");
            return nullptr;
        }

        char buffer[4];

        // Read RIFF header
        inFile.read(buffer, 4);
        if (std::strncmp(buffer, "RIFF", 4) != 0) {
            LOG_ERROR(L"Invalid RIFF header.");
            return nullptr;
        }

        // Skip "WAVE" header
        inFile.seekg(8);
        inFile.read(buffer, 4);
        if (std::strncmp(buffer, "WAVE", 4) != 0) {
            LOG_ERROR(L"Invalid WAVE format.");
            return nullptr;
        }

        // Read chunks
        while (inFile.read(buffer, 4)) {
            int chunkSize = 0;
            inFile.read(reinterpret_cast<char*>(&chunkSize), 4);

            if (std::strncmp(buffer, "fmt ", 4) == 0) {
                // Read format chunk
                char fmtBuffer[16];
                inFile.read(fmtBuffer, chunkSize);

                int audioFormat = convertToInt(fmtBuffer, 2);
                if (audioFormat != 1) { // PCM format
                    LOG_ERROR(L"Unsupported WAV format (only PCM is supported).");
                    return nullptr;
                }

                *channels = convertToInt(fmtBuffer + 2, 2);
                *samplerate = convertToInt(fmtBuffer + 4, 4);
                *bitsPerSample = convertToInt(fmtBuffer + 14, 2);

            }
            else if (std::strncmp(buffer, "data", 4) == 0) {
                // Read data chunk
                *size = chunkSize;
                char* data = (char*)malloc(*size);
                inFile.read(data, *size);
                return data;

            }
            else {
                // Skip other chunks
                inFile.seekg(chunkSize, std::ios::cur);
            }
        }

        LOG_ERROR(L"Missing 'data' chunk in WAV file.");
        return nullptr;
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