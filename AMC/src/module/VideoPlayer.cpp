#include<VideoPlayer.h>
#include<common.h>

namespace AMC {

    VideoPlayer::VideoPlayer(const std::string& filename) :
        av_format_ctx(nullptr),
        av_codec_ctx(nullptr),
        av_frame(nullptr),
        av_rgb_frame(nullptr),
        av_packet(nullptr),
        sws_scaler_ctx(nullptr),
        video_stream_index(-1),
        videoWidth(0),
        videoHeight(0),
        textureID(0),
        videoTime(0.0),
        frameDuration(0.0),
        isPlaying(false),
        rgbBuffer(nullptr),
        s_time(0),
        e_time(0),
        d_time(0),
        vedioFPS(25.0) {
        avformat_network_init();

        if (!initializeFFmpeg(filename)) {
            LOG_ERROR(L"FFMPEG Failed To Initialize");
            return;
        }

        if (!initializeTexture()) {
            LOG_ERROR(L"Video Texture Failed To Initialize");
            return;
        }

        AVRational frame_rate = av_guess_frame_rate(av_format_ctx, av_format_ctx->streams[video_stream_index], nullptr);
        if (frame_rate.num != 0 && frame_rate.den != 0) {
            frameDuration = static_cast<double>(frame_rate.den) / static_cast<double>(frame_rate.num);
        }
        else {
            frameDuration = 1.0 / 25.0; // Default to ~25 FPS if frame rate is unknown
        }

        if (av_format_ctx->duration != AV_NOPTS_VALUE) {
            totalDuration = static_cast<double>(av_format_ctx->duration) / AV_TIME_BASE;
        }
        else {
            totalDuration = 0.0;
            std::cout << "Warning: Could not determine video duration." << std::endl;
        }
    }

    VideoPlayer::~VideoPlayer() {
        cleanup();
    }

    GLuint VideoPlayer::getTexture() {
        return textureID;
    }

    float VideoPlayer::getDuration() {
        return static_cast<float>(totalDuration);
    }

    void VideoPlayer::update(double deltaTime) {
        //if (!isPlaying || totalDuration <= 0.0) return;

        //// Increment videoTime based on deltaTime
        //videoTime += deltaTime;

        //// Calculate the target playback time
        //double targetTime = videoTime;

        //// Read and decode frames until the frame time exceeds targetTime
        //while (av_read_frame(av_format_ctx, av_packet) >= 0) {
        //    if (av_packet->stream_index == video_stream_index) {
        //        int response = avcodec_send_packet(av_codec_ctx, av_packet);
        //        if (response < 0) {
        //            char errBuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        //            av_strerror(response, errBuf, sizeof(errBuf));
        //            std::cerr << "Error sending packet to decoder: " << errBuf << std::endl;
        //            av_packet_unref(av_packet);
        //            isPlaying = false;
        //            break;
        //        }

        //        response = avcodec_receive_frame(av_codec_ctx, av_frame);
        //        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
        //            av_packet_unref(av_packet);
        //            continue;
        //        }
        //        else if (response < 0) {
        //            char errBuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        //            av_strerror(response, errBuf, sizeof(errBuf));
        //            std::cerr << "Error receiving frame from decoder: " << errBuf << std::endl;
        //            av_packet_unref(av_packet);
        //            isPlaying = false;
        //            break;
        //        }

        //        // Calculate frame time
        //        double frameTime = 0.0;
        //        if (av_frame->pts != AV_NOPTS_VALUE) {
        //            frameTime = av_frame->pts * av_q2d(av_format_ctx->streams[video_stream_index]->time_base);
        //        }

        //        // If the frame time is within the current playback window, render it
        //        if (frameTime >= targetTime) {
        //            // Convert the frame to RGBA
        //            sws_scale(
        //                sws_scaler_ctx,
        //                av_frame->data,
        //                av_frame->linesize,
        //                0,
        //                av_codec_ctx->height,
        //                av_rgb_frame->data,
        //                av_rgb_frame->linesize
        //            );

        //            // Flip the image vertically
        //            flipImageVertically(av_rgb_frame->data[0], videoWidth, videoHeight, 4);

        //            // Update the OpenGL texture using traditional binding method
        //            glBindTexture(GL_TEXTURE_2D, textureID);
        //            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure proper alignment
        //            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoWidth, videoHeight, GL_RGBA, GL_UNSIGNED_BYTE, av_rgb_frame->data[0]);
        //            glBindTexture(GL_TEXTURE_2D, 0);

        //            av_packet_unref(av_packet);
        //            return;
        //        }
        //    }
        //    av_packet_unref(av_packet);
        //}
        //// If no more frames are available, stop playback
        //isPlaying = false;

        int response;

        while (av_read_frame(av_format_ctx, av_packet) >= 0) {

            response = avcodec_send_packet(av_codec_ctx, av_packet);

            response = avcodec_receive_frame(av_codec_ctx, av_frame);
            av_packet_unref(av_packet);
            break;
        }

        sws_scale(
            sws_scaler_ctx,
            av_frame->data,
            av_frame->linesize,
            0,
            av_codec_ctx->height,
            av_rgb_frame->data,
            av_rgb_frame->linesize
        );

        e_time = clock();
        d_time = (e_time - s_time) / (double)CLOCKS_PER_SEC;
        std::cout << d_time << std::endl;
        if (d_time >= 1.0 / vedioFPS) {
            s_time = e_time;
            glTextureSubImage2D(textureID, 0, 0, 0, videoWidth, videoHeight, GL_RGBA, GL_UNSIGNED_BYTE, av_rgb_frame->data[0]);
        }
    }

    void VideoPlayer::play() {
        isPlaying = true;
    }

    void VideoPlayer::pause() {
        isPlaying = false;
    }

    void VideoPlayer::stop() {
        isPlaying = false;
        videoTime = 0.0f;
    }

    bool VideoPlayer::initializeFFmpeg(const std::string& filename) {
        // Open video file
        if (avformat_open_input(&av_format_ctx, filename.c_str(), nullptr, nullptr) != 0) {
            std::cout << "Could not open video file: " << filename << std::endl;
            return false;
        }

        // Retrieve stream information
        if (avformat_find_stream_info(av_format_ctx, nullptr) < 0) {
            std::cout << "Could not find stream information." << std::endl;
            return false;
        }

        // Find the first video stream
        for (unsigned int i = 0; i < av_format_ctx->nb_streams; ++i) {
            if (av_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = static_cast<int>(i);
                break;
            }
        }

        if (video_stream_index == -1) {
            std::cout << "Could not find a video stream in the file." << std::endl;
            return false;
        }

        // Get codec parameters
        AVCodecParameters* codec_par = av_format_ctx->streams[video_stream_index]->codecpar;
        AVCodec* codec = const_cast<AVCodec*>(avcodec_find_decoder(codec_par->codec_id));
        if (!codec) {
            std::cout << "Unsupported codec!" << std::endl;
            return false;
        }

        // Allocate codec context
        av_codec_ctx = avcodec_alloc_context3(codec);
        if (!av_codec_ctx) {
            std::cout << "Failed to allocate codec context." << std::endl;
            return false;
        }

        // Copy codec parameters to codec context
        if (avcodec_parameters_to_context(av_codec_ctx, codec_par) < 0) {
            std::cout << "Failed to copy codec parameters to decoder context." << std::endl;
            return false;
        }

        // Open codec
        if (avcodec_open2(av_codec_ctx, codec, nullptr) < 0) {
            std::cout << "Could not open codec." << std::endl;
            return false;
        }

        // Allocate video frame
        av_frame = av_frame_alloc();
        if (!av_frame) {
            std::cout << "Could not allocate video frame." << std::endl;
            return false;
        }

        // Allocate RGB frame
        av_rgb_frame = av_frame_alloc();
        if (!av_rgb_frame) {
            std::cout << "Could not allocate RGB frame." << std::endl;
            return false;
        }

        // Determine required buffer size and allocate buffer
        videoWidth = av_codec_ctx->width;
        videoHeight = av_codec_ctx->height;
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
        rgbBuffer = static_cast<uint8_t*>(av_malloc(numBytes * sizeof(uint8_t)));
        if (!rgbBuffer) {
            std::cout << "Could not allocate RGB buffer." << std::endl;
            return false;
        }

        // Assign buffer to RGB frame
        if (av_image_fill_arrays(av_rgb_frame->data, av_rgb_frame->linesize, rgbBuffer, AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1) < 0) {
            std::cout << "Could not fill RGB frame arrays." << std::endl;
            return false;
        }

        // Initialize SwsContext for frame conversion (from codec's pix_fmt to RGBA)
        sws_scaler_ctx = sws_getContext(
            videoWidth,
            videoHeight,
            av_codec_ctx->pix_fmt,
            videoWidth,
            videoHeight,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );

        if (!sws_scaler_ctx) {
            std::cout << "Could not initialize the conversion context." << std::endl;
            return false;
        }

        // Allocate packet
        av_packet = av_packet_alloc();
        if (!av_packet) {
            std::cout << "Could not allocate AVPacket." << std::endl;
            return false;
        }

        return true;
    }

    bool VideoPlayer::initializeTexture() {
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureStorage2D(textureID, 1, GL_RGBA8, videoWidth, videoHeight);
        return true;
    }

    void VideoPlayer::cleanup() {
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }

        if (rgbBuffer) {
            av_free(rgbBuffer);
            rgbBuffer = nullptr;
        }

        if (av_rgb_frame) {
            av_frame_free(&av_rgb_frame);
            av_rgb_frame = nullptr;
        }

        if (av_frame) {
            av_frame_free(&av_frame);
            av_frame = nullptr;
        }

        if (av_packet) {
            av_packet_free(&av_packet);
            av_packet = nullptr;
        }

        if (sws_scaler_ctx) {
            sws_freeContext(sws_scaler_ctx);
            sws_scaler_ctx = nullptr;
        }

        if (av_codec_ctx) {
            avcodec_free_context(&av_codec_ctx);
            av_codec_ctx = nullptr;
        }

        if (av_format_ctx) {
            avformat_close_input(&av_format_ctx);
            av_format_ctx = nullptr;
        }

        avformat_network_deinit();
    }

};
