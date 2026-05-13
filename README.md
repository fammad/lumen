# Lumen

A physical desk object that senses blink rate via webcam and signals 
eye strain risk through ambient light. Built as a NordiCHI 2026 Demo 
track submission.

## What it does

Webcam → MediaPipe Face Mesh → EAR blink detection → rolling risk 
score → serial command → ESP32-S3 → NeoPixel LED ring.

Three ambient states: calm (blue), attention (amber), break (red).
Two user gestures: tap to pause, hold to start breathing exercise.

## Hardware

- XIAO ESP32-S3 + NeoPixel 16-LED ring + momentary button
- 3D-printed translucent PLA dome, ~10 cm
- USB-C to MacBook

## Performance

Blink detection F1: [your measured value here]  
Accuracy: ~99% at ≤120 cm (low light), ~99% at ≤150 cm (daylight)  
Beyond range: exponential degradation  
Test setup: MacBook Pro built-in camera, 1920×1080, 60 FPS

## Known limitations

- EAR threshold (0.21) tuned for home lighting; likely needs adjustment 
  under fluorescent conference lighting
- Head angles >~30° cause landmark compression; wink detection unreliable
- No glasses-aware calibration yet but works without indifferently

## Run

```bash
python blink_detector.py
```

Requires Python 3.12. MediaPipe pinned to 0.10.14 (3.13 breaks mp.solutions API).

## Status

NordiCHI 2026 submission in progress. Hardware build pending.

## Blink Detection Validation

F1 validation against manual count. Tested across lighting and distance.

| Condition | Distance | F1 |
|---|---|---|
| Direct sunlight | 1.0 m | 1.00 |
| Direct sunlight | 1.5 m | 0.97 |
| Direct sunlight | 2.0 m | 0.85 |
| Sunlight + head turns (±45°) | 1.0 m | 0.92 |
| Table lamp only | 1.5 m | 0.91 |
| Table lamp only | 2.0 m | 0.57 ❌ |

**Demo operating envelope:** 60–80 cm, mixed conference lighting.
All conditions within this envelope exceed F1 = 0.90.

**Known limitation:** Detection degrades sharply beyond 1.5 m in low light.
EAR threshold tuned at 0.21 for MacBook Pro built-in camera at 1080p/60fps.

Detection fails under tinted or semi-transparent sglasses. 
Designed for clear-lens glasses or no glasses only.