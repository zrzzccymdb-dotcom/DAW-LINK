
# DAW LINK

**A free, open-source audio bridge for connecting ASIO-capable DAWs to OBS Studio.**

DAW LINK is an ongoing Windows audio project designed to make it easier to send audio directly from a Digital Audio Workstation (DAW) to OBS Studio while preserving normal audio monitoring through the audio interface.

## Project goals

- Connect ASIO-capable DAWs to OBS Studio.
- Preserve normal monitoring through the audio interface.
- Provide a separate audio feed for streaming and recording.
- Support common sample rates, including 44.1 kHz, 48 kHz and 96 kHz.
- Support stereo and multichannel audio.
- Provide audio meters and connection diagnostics.
- Recover gracefully from audio interruptions where possible.
- Remain free and open source.

## Development status

**Current milestone: Phase 1 — Shared-memory audio transport prototype**

The current prototype includes:

- A Windows shared-memory audio transport.
- A test sender that generates a 440 Hz stereo sine wave.
- A test receiver that measures incoming audio levels.
- Basic frame, queue and dropped-audio statistics.

The current prototype does not yet include:

- ASIO driver integration.
- A DAW LINK control application.
- An OBS Studio plugin.
- Direct Studio One integration.
- An installer.

These features are planned for future development.

## Intended architecture

1. **DAW LINK ASIO component:** Connects to an ASIO audio interface and forwards the DAW audio.
2. **Audio transport:** Transfers audio between components using shared memory.
3. **OBS component:** Makes the audio available as a dedicated OBS audio source.
4. **Control Center:** Provides device selection, monitoring and diagnostics.

## Intended test setup

- Windows 10 or Windows 11, 64-bit
- ASIO-compatible audio interface
- ASIO-capable DAW
- OBS Studio

## Building

The project uses C++20 and CMake.

The initial prototype is intended to be built on Windows using a supported Visual Studio C++ toolchain.

## License

DAW LINK is intended to be distributed under the GNU General Public License v3.0. See `LICENSE` for details.

## Important

DAW LINK is experimental software under active development. The current prototype is not a finished audio driver or production-ready streaming solution.
