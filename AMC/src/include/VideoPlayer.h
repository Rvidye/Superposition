#pragma once
#include<GL/glew.h>
#include<string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

namespace AMC {

    class VideoPlayer
    {
    public:

        VideoPlayer(const std::string& filename);
        ~VideoPlayer();

        GLuint getTexture();
        float getDuration();
        void update(double deltaTime);
        void play();
        void pause();
        void stop();
    private:
        bool initializeFFmpeg(const std::string& filename);
        bool initializeTexture();
        void cleanup();
        void flipImageVertically(uint8_t* data, int width, int height, int channels);

        AVFormatContext* av_format_ctx;
        AVCodecContext* av_codec_ctx;
        AVFrame* av_frame;
        AVFrame* av_rgb_frame;
        AVPacket* av_packet;
        SwsContext* sws_scaler_ctx;
        int video_stream_index;

        // OpenGL texture
        GLuint textureID;
        
        // Video Properties
        int videoWidth;
        int videoHeight;

        // Playback control
        double videoTime; // Current playback time in seconds
        double frameDuration; // Duration of a single frame in seconds
        bool isPlaying;
        double totalDuration;

        // Buffer for RGB data
        uint8_t* rgbBuffer;
    };
}