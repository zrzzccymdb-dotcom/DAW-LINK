#pragma once

#ifndef DAW_LINK_AUDIO_TRANSPORT_HPP
#define DAW_LINK_AUDIO_TRANSPORT_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace dawlink
{
    constexpr std::uint32_t MAX_CHANNELS = 8;
    constexpr std::uint32_t DEFAULT_SAMPLE_RATE = 48000;
    constexpr std::uint32_t DEFAULT_BUFFER_FRAMES = 65536;

    struct AudioConfig
    {
        std::uint32_t sampleRate = DEFAULT_SAMPLE_RATE;
        std::uint32_t channels = 2;
        std::uint32_t bufferFrames = DEFAULT_BUFFER_FRAMES;
    };

    struct AudioStats
    {
        std::uint64_t framesWritten = 0;
        std::uint64_t framesRead = 0;
        std::uint64_t droppedFrames = 0;
        std::uint64_t queuedFrames = 0;
    };

    class AudioTransport
    {
    public:
        AudioTransport();
        ~AudioTransport();

        AudioTransport(const AudioTransport&) = delete;
        AudioTransport& operator=(const AudioTransport&) = delete;

        AudioTransport(AudioTransport&& other) noexcept;
        AudioTransport& operator=(AudioTransport&& other) noexcept;

        // Creates a shared-memory audio transport.
        bool Create(
            const std::wstring& name,
            const AudioConfig& config,
            std::string* errorMessage = nullptr
        );

        // Opens an existing shared-memory audio transport.
        bool Open(
            const std::wstring& name,
            std::string* errorMessage = nullptr
        );

        // Releases the shared-memory resources.
        void Close();

        // Writes interleaved floating-point audio frames.
        // Returns the number of frames successfully written.
        std::uint32_t Write(
            const float* samples,
            std::uint32_t frameCount
        );

        // Reads interleaved floating-point audio frames.
        // Returns the number of frames successfully read.
        std::uint32_t Read(
            float* output,
            std::uint32_t frameCount
        );

        // Signals the receiver that new audio may be available.
        void Signal();

        // Waits for new audio, with a timeout in milliseconds.
        bool Wait(std::uint32_t timeoutMilliseconds);

        // Returns current transport statistics.
        AudioStats GetStats() const;

        // Returns the transport configuration.
        AudioConfig GetConfig() const;

        // Returns true when the transport is ready.
        bool IsOpen() const;

        // Returns the shared-memory transport name.
        std::wstring GetName() const;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}

#endif // DAW_LINK_AUDIO_TRANSPORT_HPP
