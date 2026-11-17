
#include "audio_transport.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <cstring>
#include <limits>
#include <utility>

namespace dawlink
{
    namespace
    {
        constexpr std::uint32_t SHARED_MAGIC = 0x4441574C; // DAWL
        constexpr std::uint32_t SHARED_VERSION = 1;

        struct SharedHeader
        {
            std::uint32_t magic;
            std::uint32_t version;
            std::uint32_t sampleRate;
            std::uint32_t channels;
            std::uint32_t bufferFrames;
            std::uint32_t reserved;

            alignas(8) volatile LONG64 writeFrame;
            alignas(8) volatile LONG64 readFrame;
            alignas(8) volatile LONG64 droppedFrames;
        };

        LONG64 AtomicLoad(volatile LONG64* value)
        {
            return InterlockedCompareExchange64(value, 0, 0);
        }

        bool ValidConfig(const AudioConfig& config)
        {
            return config.sampleRate >= 8000 &&
                   config.sampleRate <= 384000 &&
                   config.channels >= 1 &&
                   config.channels <= MAX_CHANNELS &&
                   config.bufferFrames >= 256 &&
                   config.bufferFrames <= 1048576;
        }

        bool SetError(std::string* destination, const char* message)
        {
            if (destination)
                *destination = message;

            return false;
        }
    }

    struct AudioTransport::Implementation
    {
        HANDLE mapping = nullptr;
        HANDLE event = nullptr;
        void* view = nullptr;
        std::size_t mappedBytes = 0;

        SharedHeader* header = nullptr;
        float* samples = nullptr;

        AudioConfig config{};
        std::wstring name;
        bool open = false;

        void Release()
        {
            open = false;
            header = nullptr;
            samples = nullptr;

            if (view)
            {
                UnmapViewOfFile(view);
                view = nullptr;
            }

            if (event)
            {
                CloseHandle(event);
                event = nullptr;
            }

            if (mapping)
            {
                CloseHandle(mapping);
                mapping = nullptr;
            }

            mappedBytes = 0;
            name.clear();
        }

        std::size_t RequiredBytes(const AudioConfig& value) const
        {
            return sizeof(SharedHeader) +
                   static_cast<std::size_t>(value.bufferFrames) *
                   value.channels * sizeof(float);
        }
    };

    AudioTransport::AudioTransport()
        : implementation_(std::make_unique<Implementation>())
    {
    }

    AudioTransport::~AudioTransport()
    {
        Close();
    }

    AudioTransport::AudioTransport(AudioTransport&& other) noexcept
        : implementation_(std::move(other.implementation_))
    {
    }

    AudioTransport& AudioTransport::operator=(AudioTransport&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            implementation_ = std::move(other.implementation_);
        }

        return *this;
    }

    bool AudioTransport::Create(
        const std::wstring& name,
        const AudioConfig& config,
        std::string* errorMessage)
    {
        Close();

        if (!ValidConfig(config))
            return SetError(errorMessage, "Invalid audio configuration.");

        if (name.empty())
            return SetError(errorMessage, "Transport name cannot be empty.");

        const std::size_t bytes =
            sizeof(SharedHeader) +
            static_cast<std::size_t>(config.bufferFrames) *
            config.channels * sizeof(float);

        if (bytes > static_cast<std::size_t>(
                std::numeric_limits<DWORD>::max()) * 2)
        {
            return SetError(errorMessage, "Shared memory size is too large.");
        }

        const std::wstring mappingName = name + L"_Memory";
        const std::wstring eventName = name + L"_Event";

        const DWORD high = static_cast<DWORD>(
            static_cast<unsigned long long>(bytes) >> 32);

        const DWORD low = static_cast<DWORD>(bytes & 0xFFFFFFFFULL);

        HANDLE mapping = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            high,
            low,
            mappingName.c_str()
        );

        if (!mapping)
            return SetError(errorMessage, "Could not create shared memory.");

        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(mapping);
            return SetError(errorMessage,
                "A transport with this name already exists.");
        }

        void* view = MapViewOfFile(
            mapping,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            bytes
        );

        if (!view)
        {
            CloseHandle(mapping);
            return SetError(errorMessage, "Could not map shared memory.");
        }

        HANDLE event = CreateEventW(
            nullptr,
            FALSE,
            FALSE,
            eventName.c_str()
        );

        if (!event)
        {
            UnmapViewOfFile(view);
            CloseHandle(mapping);
            return SetError(errorMessage, "Could not create audio event.");
        }

        auto* header = static_cast<SharedHeader*>(view);

        std::memset(view, 0, bytes);

        header->sampleRate = config.sampleRate;
        header->channels = config.channels;
        header->bufferFrames = config.bufferFrames;
        header->version = SHARED_VERSION;

        MemoryBarrier();
        header->magic = SHARED_MAGIC;

        implementation_->mapping = mapping;
        implementation_->event = event;
        implementation_->view = view;
        implementation_->mappedBytes = bytes;
        implementation_->header = header;
        implementation_->samples =
            reinterpret_cast<float*>(
                static_cast<unsigned char*>(view) +
                sizeof(SharedHeader)
            );

        implementation_->config = config;
        implementation_->name = name;
        implementation_->open = true;

        if (errorMessage)
            errorMessage->clear();

        return true;
    }

    bool AudioTransport::Open(
        const std::wstring& name,
        std::string* errorMessage)
    {
        Close();

        if (name.empty())
            return SetError(errorMessage, "Transport name cannot be empty.");

        const std::wstring mappingName = name + L"_Memory";
        const std::wstring eventName = name + L"_Event";

        HANDLE mapping = OpenFileMappingW(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            mappingName.c_str()
        );

        if (!mapping)
            return SetError(errorMessage, "Could not open shared memory.");

        void* view = MapViewOfFile(
            mapping,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            0
        );

        if (!view)
        {
            CloseHandle(mapping);
            return SetError(errorMessage, "Could not map shared memory.");
        }

        auto* header = static_cast<SharedHeader*>(view);

        MemoryBarrier();

        if (header->magic != SHARED_MAGIC ||
            header->version != SHARED_VERSION)
        {
            UnmapViewOfFile(view);
            CloseHandle(mapping);
            return SetError(errorMessage,
                "Shared memory has an invalid header or version.");
        }

        AudioConfig config;
        config.sampleRate = header->sampleRate;
        config.channels = header->channels;
        config.bufferFrames = header->bufferFrames;

        if (!ValidConfig(config))
        {
            UnmapViewOfFile(view);
            CloseHandle(mapping);
            return SetError(errorMessage,
                "Shared memory contains an invalid audio configuration.");
        }

        const std::size_t bytes =
            sizeof(SharedHeader) +
            static_cast<std::size_t>(config.bufferFrames) *
            config.channels * sizeof(float);

        HANDLE event = OpenEventW(
            SYNCHRONIZE | EVENT_MODIFY_STATE,
            FALSE,
            eventName.c_str()
        );

        if (!event)
        {
            UnmapViewOfFile(view);
            CloseHandle(mapping);
            return SetError(errorMessage, "Could not open audio event.");
        }

        implementation_->mapping = mapping;
        implementation_->event = event;
        implementation_->view = view;
        implementation_->mappedBytes = bytes;
        implementation_->header = header;
        implementation_->samples =
            reinterpret_cast<float*>(
                static_cast<unsigned char*>(view) +
                sizeof(SharedHeader)
            );

        implementation_->config = config;
        implementation_->name = name;
        implementation_->open = true;

        if (errorMessage)
            errorMessage->clear();

        return true;
    }

    void AudioTransport::Close()
    {
        if (implementation_)
            implementation_->Release();
    }

    std::uint32_t AudioTransport::Write(
        const float* samples,
        std::uint32_t frameCount)
    {
        if (!IsOpen() || !samples || frameCount == 0)
            return 0;

        auto* header = implementation_->header;
        const std::uint32_t channels = implementation_->config.channels;
        const std::uint64_t capacity =
            implementation_->config.bufferFrames;

        const std::uint64_t writeFrame =
            static_cast<std::uint64_t>(AtomicLoad(&header->writeFrame));

        const std::uint64_t readFrame =
            static_cast<std::uint64_t>(AtomicLoad(&header->readFrame));

        const std::uint64_t queued = writeFrame - readFrame;
        const std::uint64_t available =
            queued >= capacity ? 0 : capacity - queued;

        const std::uint32_t framesToWrite =
            static_cast<std::uint32_t>(
                available < frameCount ? available : frameCount
            );

        const std::uint32_t dropped = frameCount - framesToWrite;

        if (dropped > 0)
            InterlockedExchangeAdd64(
                &header->droppedFrames,
                static_cast<LONG64>(dropped)
            );

        if (framesToWrite == 0)
            return 0;

        const std::uint32_t start =
            static_cast<std::uint32_t>(writeFrame % capacity);

        for (std::uint32_t frame = 0; frame < framesToWrite; ++frame)
        {
            const std::uint32_t destination =
                (start + frame) %
                static_cast<std::uint32_t>(capacity);

            std::memcpy(
                implementation_->samples +
                    static_cast<std::size_t>(destination) * channels,
                samples +
                    static_cast<std::size_t>(frame) * channels,
                channels * sizeof(float)
            );
        }

        MemoryBarrier();

        InterlockedExchange64(
            &header->writeFrame,
            static_cast<LONG64>(writeFrame + framesToWrite)
        );

        Signal();

        return framesToWrite;
    }

    std::uint32_t AudioTransport::Read(
        float* output,
        std::uint32_t frameCount)
    {
        if (!IsOpen() || !output || frameCount == 0)
            return 0;

        auto* header = implementation_->header;
        const std::uint32_t channels = implementation_->config.channels;
        const std::uint64_t capacity =
            implementation_->config.bufferFrames;

        const std::uint64_t readFrame =
            static_cast<std::uint64_t>(AtomicLoad(&header->readFrame));

        const std::uint64_t writeFrame =
            static_cast<std::uint64_t>(AtomicLoad(&header->writeFrame));

        const std::uint64_t queued = writeFrame - readFrame;

        const std::uint32_t framesToRead =
            static_cast<std::uint32_t>(
                queued < frameCount ? queued : frameCount
            );

        if (framesToRead == 0)
            return 0;

        const std::uint32_t start =
            static_cast<std::uint32_t>(readFrame % capacity);

        for (std::uint32_t frame = 0; frame < framesToRead; ++frame)
        {
            const std::uint32_t source =
                (start + frame) %
                static_cast<std::uint32_t>(capacity);

            std::memcpy(
                output +
                    static_cast<std::size_t>(frame) * channels,
                implementation_->samples +
                    static_cast<std::size_t>(source) * channels,
                channels * sizeof(float)
            );
        }

        MemoryBarrier();

        InterlockedExchange64(
            &header->readFrame,
            static_cast<LONG64>(readFrame + framesToRead)
        );

        return framesToRead;
    }

    void AudioTransport::Signal()
    {
        if (IsOpen())
            SetEvent(implementation_->event);
    }

    bool AudioTransport::Wait(std::uint32_t timeoutMilliseconds)
    {
        if (!IsOpen())
            return false;

        return WaitForSingleObject(
            implementation_->event,
            timeoutMilliseconds
        ) == WAIT_OBJECT_0;
    }

    AudioStats AudioTransport::GetStats() const
    {
        AudioStats stats;

        if (!IsOpen())
            return stats;

        const auto* header = implementation_->header;

        const std::uint64_t written =
            static_cast<std::uint64_t>(
                AtomicLoad(&header->writeFrame)
            );

        const std::uint64_t read =
            static_cast<std::uint64_t>(
                AtomicLoad(&header->readFrame)
            );

        stats.framesWritten = written;
        stats.framesRead = read;
        stats.droppedFrames = static_cast<std::uint64_t>(
            AtomicLoad(&header->droppedFrames)
        );

        stats.queuedFrames = written >= read ? written - read : 0;

        return stats;
    }

    AudioConfig AudioTransport::GetConfig() const
    {
        if (!IsOpen())
            return AudioConfig{};

        return implementation_->config;
    }

    bool AudioTransport::IsOpen() const
    {
        return implementation_ &&
               implementation_->open &&
               implementation_->header != nullptr &&
               implementation_->samples != nullptr;
    }

    std::wstring AudioTransport::GetName() const
    {
        if (!IsOpen())
            return {};

        return implementation_->name;
    }
}
