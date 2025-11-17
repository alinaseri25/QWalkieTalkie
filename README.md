
# QWalkieTalkie — UDP Real-Time Audio with Qt

Experimental walkie-talkie style real-time voice streaming between Android and Windows using Qt `QUdpSocket` and `QAudioSource/QAudioSink`.

## Overview
- Goal: Low-latency bidirectional audio over UDP.
- Status: Working prototype with adaptive buffering.
- Platform: Android ↔ Windows.

## Features
- Send/receive raw PCM via UDP with adjustable packet size.
- Adaptive jitter buffer to smooth network variability.
- Cross-platform audio I/O using Qt Multimedia.

## Architecture
- AudioCapture: pulls PCM from `QAudioSource` (Android/Windows).
- NetTransport: sends/receives packets via `QUdpSocket`.
- JitterBuffer: time-aligns incoming audio, adaptive depth.
- AudioPlayback: feeds `QAudioSink`.

Suggested classes:
- AudioCapture.h/.cpp
- NetTransport.h/.cpp
- JitterBuffer.h/.cpp
- AudioPlayback.h/.cpp
- MainWindow/UI for controls

## Build Requirements
- Qt: 6.8.1 (Multimedia module enabled)
- Android SDK/NDK for mobile target, MSVC/MinGW for Windows
- JDK 17 for Android builds via Qt Creator

AndroidManifest permissions:
- RECORD_AUDIO
- MODIFY_AUDIO_SETTINGS
- INTERNET
- ACCESS_NETWORK_STATE
- ACCESS_WIFI_STATE

Multicast/Broadcast reception on Android:
- Acquire `WifiManager.createMulticastLock()` via JNI if you use multicast.

## Configuration
- Sample rate: 16000 or 48000 Hz
- Channel: mono (1)
- Sample format: signed 16-bit (PCM S16LE)
- Packet window: start with 20 ms (e.g., 320 bytes @ 16 kHz, mono, 16-bit)

## Usage
- Start capture, select target IP:port, press “Push-to-Talk” or continuous mode.
- Receiver plays audio via `QAudioSink` with adaptive buffer.

## Benchmark (TODO)
- Plot Packet Size (bytes) vs End-to-End Delay (ms).
- Measure jitter, packet loss rate, and buffer under/over-run counts.
- Compare raw PCM vs Opus.

## Opus Integration (Optional)
- Use libopus to encode/decode with 10–20 ms frames.
- Benefits: lower bandwidth, improved quality under loss.

## Roadmap
- Add echo cancellation and AGC (SpeexDSP/WebRTC AEC).
- NAT traversal (STUN) for cross-network scenarios.
- Packet authentication (SRTP-like lightweight HMAC).

## License
MIT/Apache-2.0 suggested for education and reuse.
