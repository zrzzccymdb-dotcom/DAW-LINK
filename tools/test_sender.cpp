#include "audio_transport.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <thread>
#include <chrono>
#include <string>

int main()
{
    constexpr double PI = std::numbers::pi_v<double>;
    constexpr double FREQUENCY = 440.0;
    constexpr std::uint32_t SAMPLE_RATE = 48000;
    constexpr std::uint32_t CHANNELS = 2;
    constexpr std::uint32_t FRAMES_PER_BLOCK = 480;

    dawlink::AudioTransport transport;

    dawlink::AudioConfig config;
    config.sampleRate = SAMPLE_RATE;
    config.channels = CHANNELS;
    config.bufferFrames = 65536;

    std::string error;

    if (!transport.Create(
            L"Local\\DAW_LINK_Phase1",
            config,
            &error))
    {
        std::cerr << "Could not start DAW LINK sender: "
                  << error << '\n';
        return 1;
    }

    std::cout << "DAW LINK test sender is running.\n";
    std::cout << "Signal: 440 Hz stereo sine wave\n";
    std::cout << "Sample rate: " << SAMPLE_RATE << " Hz\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    std::array<float, FRAMES_PER_BLOCK * CHANNELS> buffer{};

    double phase = 0.0;
    const double phaseStep =
        2.0 * PI * FREQUENCY / SAMPLE_RATE;

    std::uint64_t totalFrames = 0;

    while (true)
    {
        for (std::uint32_t frame = 0;
             frame < FRAMES_PER_BLOCK;
             ++frame)
        {
            const float sample =
                static_cast<float>(0.25 * std::sin(phase));

            buffer[frame * CHANNELS] = sample;
            buffer[frame * CHANNELS + 1] = sample;

            phase += phaseStep;

            if (phase >= 2.0 * PI)
                phase -= 2.0 * PI;
        }

        const std::uint32_t written = transport.Write(
            buffer.data(),
            FRAMES_PER_BLOCK
        );

        totalFrames += written;

        if (totalFrames >= SAMPLE_RATE)
        {
            const auto stats = transport.GetStats();

            std::cout << "Frames sent: "
                      << stats.framesWritten
                      << " | Dropped: "
                      << stats.droppedFrames
                      << " | Queued: "
                      << stats.queuedFrames
                      << '\n';

            totalFrames = 0;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    return 0;
}
