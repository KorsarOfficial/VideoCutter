# VideoCutter

Video trimming via FFmpeg stream copy — no re-encoding.

## Model

```
V: File → AVFormatContext → AVCodecContext → AVFrame (YUV420P)
R: AVFrame → sws_scale → SDL_UpdateYUVTexture → SDL_RenderCopy
T: [t_in, t_out] ⊂ [0, duration], t_in < t_out
C: ffmpeg -ss t_in -to t_out -c copy → F ∈ {mp4,avi,mov,mkv,webm,ogg,flv,wmv}
```

Pipeline: `File → libavformat → libavcodec → sws_scale → SDL2 texture → display`

## Build

```
pacman -S mingw-w64-x86_64-{gcc,cmake,SDL2,ffmpeg}
mkdir build && cd build && cmake .. -G "MinGW Makefiles" && cmake --build .
```

## Controls

| Key | Action |
|-----|--------|
| O | Open file dialog |
| Space | Play/Pause |
| I | Set trim start |
| ] | Set trim end |
| R | Reset trim points |
| F / ← → | Cycle format |
| C | Cut (save dialog) |

## Requirements

- Windows 10+
- MSYS2 MinGW64
- FFmpeg in PATH (for cutting)
