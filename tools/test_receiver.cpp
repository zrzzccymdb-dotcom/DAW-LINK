#include "audio_transport.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

int main()
{
    constexpr std::uint32_t FRAMES_PER_BLOCK = 480;
    constexpr std::uint32_t CHANNELS = 2;

    dawlink::AudioTransport transport;
    std::string error;

    std::cout << "DAW LINK test receiver starting...\n";

    if (!transport.Open(
            L"Local\\DAW_LINK_Phase1",
            &error))
    {
        std::cerr << "Could not connect to DAW LINK sender: "
                  << error << '\n';

        std::cerr << "Start test_sender.exe first, then try again.\n";
        return 1;
    }

    const auto config = transport.GetConfig();

    std::cout << "Connected to DAW LINK!\n";
    std::cout << "Sample rate: " << config.sampleRate << " Hz\n";
    std::cout << "Channels: " << config.channels << '\n';
    std::cout << "Buffer capacity: "
              << config.bufferFrames << " frames\n\n";

    std::array<float, FRAMES_PER_BLOCK * CHANNELS> buffer{};

    double sumSquares = 0.0;
    float peak = 0.0f;

    std::uint64_t measuredSamples = 0;
    auto lastReport = std::chrono::steady_clock::now();

    while (true)
    {
        if (transport.GetStats().queuedFrames == 0)
        {
            transport.Wait(20);
            continue;
        }

        const std::uint32_t framesRead = transport.Read(
            buffer.data(),
            FRAMES_PER_BLOCK
        );

        if (framesRead == 0)
            continue;

        for (std::uint32_t i = 0;
             i < framesRead * config.channels;
             ++i)
        {
            const float sample = buffer[i];

            peak = std::max(peak, std::abs(sample));
            sumSquares += static_cast<double>(sample) * sample;
            ++measuredSamples;
        }

        const auto now = std::chrono::steady_clock::now();

        if (now - lastReport >= std::chrono::seconds(1))
        {
            const double rms = measuredSamples > 0
                ? std::sqrt(sumSquares / measuredSamples)
                : 0.0;

            const auto stats = transport.GetStats();

            std::cout << "Audio received"
                      << " | Peak: " << peak
                      << " | RMS: " << rms
                      << " | Queued: " << stats.queuedFrames
                      << " | Dropped: " << stats.droppedFrames
                      << " | Frames read: " << stats.framesRead
                      << '\n';

            peak = 0.0f;
            sumSquares = 0.0;
            measuredSamples = 0;
            lastReport = now;
        }
    }

    return 0;
}
