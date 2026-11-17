
# DAW LINK — System Architecture

## 1. Overview

DAW LINK is a free, open-source Windows audio bridge designed to route audio from ASIO-compatible Digital Audio Workstations (DAWs) to OBS Studio.

The primary goal is to allow musicians and streamers to use their normal DAW and audio interface monitoring while providing OBS with an independent, low-latency audio feed.

## 2. Design Goals

- Work with multiple ASIO-compatible audio interfaces.
- Support multiple DAW applications.
- Provide a dedicated audio source for OBS Studio.
- Preserve the user's normal hardware monitoring.
- Minimize latency and audio dropouts.
- Support stereo and multichannel audio.
- Provide connection monitoring and diagnostics.
- Recover gracefully from device interruptions.
- Remain free and open source.

## 3. High-Level Architecture

The planned system consists of four major components.

### Component A: DAW LINK ASIO Bridge

The ASIO bridge will connect to the audio interface's existing ASIO driver.

Its responsibilities will include:

- Forwarding audio to the physical audio interface.
- Capturing a copy of the DAW's output.
- Maintaining sample-rate and channel information.
- Handling audio buffer callbacks.
- Reporting driver errors and interruptions.

The bridge must avoid disrupting normal DAW playback.

### Component B: Audio Transport

The audio transport provides communication between DAW LINK components.

Phase 1 implements a Windows shared-memory transport.

Its responsibilities include:

- Transferring floating-point audio samples.
- Maintaining read and write positions.
- Tracking queued audio frames.
- Counting dropped frames.
- Signaling the receiving component when audio is available.

The initial implementation supports up to eight channels.

### Component C: OBS Audio Source

A native OBS plugin is planned.

Its responsibilities will include:

- Discovering the DAW LINK audio transport.
- Reading audio from shared memory.
- Providing audio to OBS.
- Supporting stereo and multichannel configurations.
- Reporting connection status.
- Handling interruptions without crashing OBS.

The plugin should appear as a dedicated DAW LINK audio source in OBS Studio.

### Component D: DAW LINK Control Center

A Windows desktop application is planned.

Its responsibilities will include:

- Selecting available audio devices.
- Displaying connection status.
- Showing audio input and output meters.
- Displaying sample rate and channel configuration.
- Reporting dropped frames and buffer statistics.
- Providing troubleshooting information.
- Managing DAW LINK components.

## 4. Intended Audio Signal Flow

The intended signal flow is:

1. Guitar or instrument enters the audio interface.
2. The DAW receives audio through the ASIO driver.
3. Studio One processes the audio using plugins such as NAM.
4. The DAW LINK bridge forwards audio to the physical audio interface.
5. A copy of the DAW output is sent to the shared-memory transport.
6. The OBS component receives the copied audio.
7. OBS mixes the DAW LINK source into the stream or recording.

The goal is to preserve the normal audio interface monitoring path while providing OBS with its own audio feed.

## 5. Audio Format

Initial development configuration:

| Setting | Initial Value |
|---|---|
| Operating system | Windows 10/11 x64 |
| Sample rate | 48,000 Hz |
| Audio format | Float32 |
| Channels | Stereo |
| Maximum channels | 8 |
| Buffer capacity | 65,536 frames |
| Language | C++ |
| Language standard | C++20 |

Future sample-rate targets:

- 44,100 Hz
- 48,000 Hz
- 96,000 Hz

The system should use the sample rate selected by the DAW and audio interface wherever possible.

## 6. Performance and Reliability

Real-time audio processing requires careful attention to timing.

The planned implementation should:

- Avoid unnecessary memory allocation inside audio callbacks.
- Avoid blocking operations inside real-time callbacks.
- Use bounded buffers.
- Track dropped frames and buffer underruns.
- Handle device disconnections.
- Avoid interfering with the DAW's audio engine.
- Provide useful diagnostics when audio stops.

No universal zero-dropout guarantee is assumed. Performance will depend on the audio interface, driver, computer and buffer configuration.

## 7. Development Phases

### Phase 1 — Shared-Memory Transport

Status: Prototype source created.

- Shared-memory transport.
- Audio sender test.
- Audio receiver test.
- Basic audio statistics.

### Phase 2 — Automated Windows Builds

- Configure GitHub Actions.
- Compile the Windows x64 executables.
- Package downloadable build artifacts.
- Fix compiler errors.

### Phase 3 — ASIO Integration

- Investigate ASIO SDK licensing and redistribution requirements.
- Develop the ASIO bridge.
- Connect to supported hardware drivers.
- Verify normal monitoring and independent audio capture.

### Phase 4 — OBS Integration

- Develop a native OBS audio source.
- Connect it to the shared-memory transport.
- Add connection status and audio meters.
- Test audio in OBS recordings and live streams.

### Phase 5 — Control Center

- Build the Windows desktop interface.
- Add device selection.
- Add audio meters and diagnostics.
- Add configuration and recovery options.

### Phase 6 — Release

- Package the application.
- Create a Windows installer.
- Document installation and troubleshooting.
- Publish a free release.

## 8. Intended Test Environment

Initial development and testing target:

- Windows 10 or Windows 11, 64-bit
- Studio One
- Focusrite Scarlett 2i2 3rd Generation
- OBS Studio
- NAM guitar processing

The architecture is intended to support other compatible DAWs and audio interfaces as development progresses.

## 9. Licensing

The project is intended to use the GNU General Public License version 3.0.

Third-party SDKs and libraries must be reviewed for license compatibility before inclusion or distribution.

## 10. Current Limitations

The Phase 1 prototype is not a finished audio bridge.

It currently does not:

- Capture audio directly from an ASIO driver.
- Integrate with Studio One.
- Provide an OBS plugin.
- Provide a desktop control application.
- Guarantee real-time performance under all conditions.

These capabilities are planned development goals, not completed features.
