#pragma GCC optimize("O2")
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <string>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
using namespace std;
#define rep(i,a,b) for(int i=(a);i<(b);i++)

constexpr int INIT_W = 1280, INIT_H = 720;
constexpr SDL_Color BG      = {26, 26, 46, 255};
constexpr SDL_Color TOP_BAR = {22, 33, 62, 255};
constexpr SDL_Color BOT_BAR = {15, 52, 96, 255};
constexpr SDL_Color BOT_BAR_LOADED = {26, 71, 42, 255};
constexpr SDL_Color TEXT_COLOR = {200, 200, 220, 255};
constexpr SDL_Color DIM_TEXT   = {120, 120, 140, 255};
constexpr SDL_Color ACCENT     = {233, 69, 96, 255};
constexpr SDL_Color CTRL_BG    = {16, 33, 62, 255};
constexpr SDL_Color TRIM_START_COL = {233, 69, 96, 255};
constexpr SDL_Color TRIM_END_COL   = {0, 210, 211, 255};
constexpr SDL_Color TRIM_FILL_COL  = {233, 69, 96, 60};
constexpr int TOP_H = 40, BOT_H = 32, CTRL_H = 50, TRIM_H = 100, FONT_SCALE = 2;
constexpr int THUMB_W = 160, THUMB_H = 90;

// output formats
static const char* FORMATS[] = {"mp4","avi","mov","mkv","webm","ogg","flv","wmv"};
static const char* FORMAT_LABELS[] = {"MP4","AVI","MOV","MKV","WebM","OGG","FLV","WMV"};
constexpr int NUM_FORMATS = 8;

// cut state (shared between main thread and ffmpeg thread)
static atomic<int> cut_state{0}; // 0=idle, 1=cutting, 2=done, 3=error
static atomic<double> cut_progress{0.0};
static mutex cut_error_mtx;
static string cut_error_msg;
static string cut_done_path;
static HANDLE cut_process_handle = nullptr;
static thread cut_thread;

// embedded 8x8 bitmap font for ASCII 32..126
static constexpr unsigned char FONT_DATA[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 space
    {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00}, // 33 !
    {0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 "
    {0x6C,0xFE,0x6C,0x6C,0xFE,0x6C,0x00,0x00}, // 35 #
    {0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00}, // 36 $
    {0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00,0x00}, // 37 %
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // 38 &
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 39 '
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 (
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 *
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 +
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ,
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 -
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 .
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x00,0x00}, // 47 /
    {0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00}, // 48 0
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 1
    {0x7C,0xC6,0x06,0x1C,0x70,0xC6,0xFE,0x00}, // 50 2
    {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00}, // 51 3
    {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00}, // 52 4
    {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00}, // 53 5
    {0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, // 54 6
    {0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00}, // 55 7
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, // 56 8
    {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00}, // 57 9
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // 58 :
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // 59 ;
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 <
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 61 =
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00}, // 62 >
    {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00}, // 63 ?
    {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0x00}, // 64 @
    {0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00}, // 65 A
    {0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00}, // 66 B
    {0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00}, // 67 C
    {0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00}, // 68 D
    {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE,0x00}, // 69 E
    {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0,0x00}, // 70 F
    {0x7C,0xC6,0xC0,0xCE,0xC6,0xC6,0x7E,0x00}, // 71 G
    {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, // 72 H
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, // 73 I
    {0x1E,0x06,0x06,0x06,0xC6,0xC6,0x7C,0x00}, // 74 J
    {0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00}, // 75 K
    {0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00}, // 76 L
    {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00}, // 77 M
    {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, // 78 N
    {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, // 79 O
    {0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00}, // 80 P
    {0x7C,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x06}, // 81 Q
    {0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00}, // 82 R
    {0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00}, // 83 S
    {0xFE,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 T
    {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, // 85 U
    {0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00}, // 86 V
    {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00}, // 87 W
    {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, // 88 X
    {0xC6,0xC6,0x6C,0x38,0x18,0x18,0x18,0x00}, // 89 Y
    {0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE,0x00}, // 90 Z
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 91 [
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x00,0x00}, // 92 backslash
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 93 ]
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, // 94 ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00}, // 95 _
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // 96 `
    {0x00,0x00,0x7C,0x06,0x7E,0xC6,0x7E,0x00}, // 97 a
    {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xFC,0x00}, // 98 b
    {0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00}, // 99 c
    {0x06,0x06,0x7E,0xC6,0xC6,0xC6,0x7E,0x00}, // 100 d
    {0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00}, // 101 e
    {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00}, // 102 f
    {0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x7C}, // 103 g
    {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x00}, // 104 h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 i
    {0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0xCC,0x78}, // 106 j
    {0xC0,0xC0,0xCC,0xD8,0xF0,0xD8,0xCC,0x00}, // 107 k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 l
    {0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00}, // 109 m
    {0x00,0x00,0xFC,0xC6,0xC6,0xC6,0xC6,0x00}, // 110 n
    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00}, // 111 o
    {0x00,0x00,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0}, // 112 p
    {0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x06}, // 113 q
    {0x00,0x00,0xDC,0xE6,0xC0,0xC0,0xC0,0x00}, // 114 r
    {0x00,0x00,0x7E,0xC0,0x7C,0x06,0xFC,0x00}, // 115 s
    {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00}, // 116 t
    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x00}, // 117 u
    {0x00,0x00,0xC6,0xC6,0x6C,0x38,0x10,0x00}, // 118 v
    {0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00}, // 119 w
    {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00}, // 120 x
    {0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0x7C}, // 121 y
    {0x00,0x00,0xFE,0x0C,0x38,0x60,0xFE,0x00}, // 122 z
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // 123 {
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 124 |
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // 125 }
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}, // 126 ~
};

struct VideoInfo {
    string filename, filepath;
    int width=0, height=0;
    double duration=0, fps=0;
    bool loaded=false;
    string error;
};

// playback state
struct Playback {
    AVFormatContext* fmt = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwsContext* sws = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* yuv_frame = nullptr;
    AVPacket* pkt = nullptr;
    SDL_Texture* tex = nullptr;
    int stream_idx = -1;
    AVRational time_base = {0,1};
    bool playing = false;
    bool has_frame = false;
    bool eof = false;
    double current_time = 0.0;
    double duration = 0.0;
    Uint64 play_start_ticks = 0;
    double play_start_time = 0.0;
    bool dragging_timeline = false;

    // trim state
    double trim_start = -1, trim_end = -1;
    SDL_Texture* thumb_start = nullptr;
    SDL_Texture* thumb_end = nullptr;
    int dragging_trim = 0; // 0=none, 1=start, 2=end

    void close() {
        if(tex) { SDL_DestroyTexture(tex); tex = nullptr; }
        if(thumb_start) { SDL_DestroyTexture(thumb_start); thumb_start = nullptr; }
        if(thumb_end) { SDL_DestroyTexture(thumb_end); thumb_end = nullptr; }
        if(yuv_frame) { av_frame_free(&yuv_frame); }
        if(frame) { av_frame_free(&frame); }
        if(pkt) { av_packet_free(&pkt); }
        if(sws) { sws_freeContext(sws); sws = nullptr; }
        if(codec_ctx) { avcodec_free_context(&codec_ctx); }
        if(fmt) { avformat_close_input(&fmt); }
        stream_idx = -1;
        playing = false;
        has_frame = false;
        eof = false;
        current_time = 0.0;
        duration = 0.0;
        dragging_timeline = false;
        trim_start = trim_end = -1;
        dragging_trim = 0;
    }
};

static SDL_Renderer* ren = nullptr;

void draw_char(int x, int y, char c, SDL_Color col, int scale=FONT_SCALE) {
    if(c < 32 || c > 126) c = '?';
    const auto& g = FONT_DATA[c - 32];
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
    rep(row,0,8) rep(bit,0,8)
        if(g[row] & (0x80 >> bit)) {
            SDL_Rect r = {x + bit*scale, y + row*scale, scale, scale};
            SDL_RenderFillRect(ren, &r);
        }
}

void draw_text(int x, int y, const char* s, SDL_Color col, int scale=FONT_SCALE) {
    while(*s) {
        draw_char(x, y, *s, col, scale);
        x += 8 * scale;
        s++;
    }
}

int text_width(const char* s, int scale=FONT_SCALE) {
    return (int)strlen(s) * 8 * scale;
}

void draw_text_centered(int cx, int y, const char* s, SDL_Color col, int scale=FONT_SCALE) {
    draw_text(cx - text_width(s, scale)/2, y, s, col, scale);
}

string format_time(double sec) {
    if(sec < 0) sec = 0;
    int h = (int)sec / 3600;
    int m = ((int)sec % 3600) / 60;
    int s = (int)sec % 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    return buf;
}

string wchar_to_utf8(const wchar_t* ws) {
    int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if(len <= 0) return "";
    string s(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, s.data(), len, nullptr, nullptr);
    return s;
}

string basename_of(const string& path) {
    auto p = path.find_last_of("/\\");
    return p == string::npos ? path : path.substr(p + 1);
}

string stem_of(const string& filename) {
    auto p = filename.find_last_of('.');
    return p == string::npos ? filename : filename.substr(0, p);
}

// decode one frame, returns true if got a frame
bool decode_one_frame(Playback& pb) {
    while(true) {
        int ret = avcodec_receive_frame(pb.codec_ctx, pb.frame);
        if(ret == 0) {
            // convert to YUV420P
            sws_scale(pb.sws, pb.frame->data, pb.frame->linesize,
                      0, pb.codec_ctx->height,
                      pb.yuv_frame->data, pb.yuv_frame->linesize);
            // update texture
            SDL_UpdateYUVTexture(pb.tex, nullptr,
                pb.yuv_frame->data[0], pb.yuv_frame->linesize[0],
                pb.yuv_frame->data[1], pb.yuv_frame->linesize[1],
                pb.yuv_frame->data[2], pb.yuv_frame->linesize[2]);
            // update current time from pts
            if(pb.frame->pts != AV_NOPTS_VALUE)
                pb.current_time = pb.frame->pts * av_q2d(pb.time_base);
            pb.has_frame = true;
            return true;
        }
        if(ret == AVERROR_EOF) { pb.eof = true; return false; }
        // need more data
        while(true) {
            int rr = av_read_frame(pb.fmt, pb.pkt);
            if(rr < 0) {
                avcodec_send_packet(pb.codec_ctx, nullptr); // flush
                break;
            }
            if(pb.pkt->stream_index == pb.stream_idx) {
                avcodec_send_packet(pb.codec_ctx, pb.pkt);
                av_packet_unref(pb.pkt);
                break;
            }
            av_packet_unref(pb.pkt);
        }
    }
}

void seek_to(Playback& pb, double target_sec) {
    if(!pb.fmt) return;
    int64_t ts = (int64_t)(target_sec / av_q2d(pb.time_base));
    av_seek_frame(pb.fmt, pb.stream_idx, ts, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(pb.codec_ctx);
    pb.eof = false;
    // decode forward to reach target or nearest keyframe
    decode_one_frame(pb);
    pb.current_time = max(0.0, min(pb.current_time, pb.duration));
    if(pb.playing) {
        pb.play_start_ticks = SDL_GetTicks64();
        pb.play_start_time = pb.current_time;
    }
}

// generate a thumbnail texture at given timestamp
SDL_Texture* generate_thumbnail(Playback& pb, double timestamp) {
    if(!pb.fmt || !pb.codec_ctx) return nullptr;
    double saved_time = pb.current_time;
    bool was_playing = pb.playing;
    pb.playing = false;

    // seek to thumbnail timestamp
    int64_t ts = (int64_t)(timestamp / av_q2d(pb.time_base));
    av_seek_frame(pb.fmt, pb.stream_idx, ts, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(pb.codec_ctx);
    pb.eof = false;

    AVFrame* tmp = av_frame_alloc();
    bool got = false;
    // decode until we get a frame
    while(!got) {
        int ret = avcodec_receive_frame(pb.codec_ctx, tmp);
        if(ret == 0) { got = true; break; }
        if(ret == AVERROR_EOF) break;
        while(true) {
            int rr = av_read_frame(pb.fmt, pb.pkt);
            if(rr < 0) { avcodec_send_packet(pb.codec_ctx, nullptr); break; }
            if(pb.pkt->stream_index == pb.stream_idx) {
                avcodec_send_packet(pb.codec_ctx, pb.pkt);
                av_packet_unref(pb.pkt);
                break;
            }
            av_packet_unref(pb.pkt);
        }
    }

    SDL_Texture* thumb = nullptr;
    if(got) {
        // scale to thumbnail size
        int tw = THUMB_W, th = THUMB_H;
        // maintain aspect ratio
        double aspect = (double)pb.codec_ctx->width / pb.codec_ctx->height;
        if(aspect > (double)THUMB_W / THUMB_H) {
            th = (int)(THUMB_W / aspect);
        } else {
            tw = (int)(THUMB_H * aspect);
        }

        SwsContext* thumb_sws = sws_getContext(
            pb.codec_ctx->width, pb.codec_ctx->height, pb.codec_ctx->pix_fmt,
            tw, th, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if(thumb_sws) {
            uint8_t* rgb_data[1];
            int rgb_linesize[1];
            rgb_linesize[0] = tw * 3;
            rgb_data[0] = new uint8_t[th * rgb_linesize[0]];
            sws_scale(thumb_sws, tmp->data, tmp->linesize, 0,
                      pb.codec_ctx->height, rgb_data, rgb_linesize);

            thumb = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                      SDL_TEXTUREACCESS_STATIC, tw, th);
            if(thumb) {
                SDL_UpdateTexture(thumb, nullptr, rgb_data[0], rgb_linesize[0]);
            }
            delete[] rgb_data[0];
            sws_freeContext(thumb_sws);
        }
    }
    av_frame_free(&tmp);

    // seek back to original position
    seek_to(pb, saved_time);
    pb.playing = was_playing;
    if(was_playing) {
        pb.play_start_ticks = SDL_GetTicks64();
        pb.play_start_time = pb.current_time;
    }
    return thumb;
}

bool open_video(Playback& pb, VideoInfo& vi, const string& path, SDL_Renderer* renderer) {
    pb.close();
    vi = {};
    vi.filename = basename_of(path);
    vi.filepath = path;

    if(avformat_open_input(&pb.fmt, path.c_str(), nullptr, nullptr) < 0) {
        vi.error = "Failed to open: " + vi.filename;
        return false;
    }
    if(avformat_find_stream_info(pb.fmt, nullptr) < 0) {
        vi.error = "Cannot read stream info";
        pb.close();
        return false;
    }

    pb.stream_idx = av_find_best_stream(pb.fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(pb.stream_idx < 0) {
        vi.error = "No video stream found";
        pb.close();
        return false;
    }

    auto* par = pb.fmt->streams[pb.stream_idx]->codecpar;
    pb.time_base = pb.fmt->streams[pb.stream_idx]->time_base;

    // codec
    const AVCodec* codec = avcodec_find_decoder(par->codec_id);
    if(!codec) {
        vi.error = "Unsupported codec";
        pb.close();
        return false;
    }
    pb.codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(pb.codec_ctx, par);
    if(avcodec_open2(pb.codec_ctx, codec, nullptr) < 0) {
        vi.error = "Failed to open codec";
        pb.close();
        return false;
    }

    int vw = pb.codec_ctx->width, vh = pb.codec_ctx->height;

    // sws for converting to YUV420P
    pb.sws = sws_getContext(vw, vh, pb.codec_ctx->pix_fmt,
                            vw, vh, AV_PIX_FMT_YUV420P,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    if(!pb.sws) {
        vi.error = "Failed to create scaler";
        pb.close();
        return false;
    }

    // alloc frames
    pb.frame = av_frame_alloc();
    pb.yuv_frame = av_frame_alloc();
    pb.yuv_frame->format = AV_PIX_FMT_YUV420P;
    pb.yuv_frame->width = vw;
    pb.yuv_frame->height = vh;
    av_frame_get_buffer(pb.yuv_frame, 32);
    pb.pkt = av_packet_alloc();

    // SDL texture
    pb.tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                               SDL_TEXTUREACCESS_STREAMING, vw, vh);
    if(!pb.tex) {
        vi.error = "Failed to create texture";
        pb.close();
        return false;
    }

    // fill VideoInfo
    vi.width = vw;
    vi.height = vh;
    auto fr = pb.fmt->streams[pb.stream_idx]->avg_frame_rate;
    if(fr.num && fr.den) vi.fps = (double)fr.num / fr.den;
    else {
        fr = pb.fmt->streams[pb.stream_idx]->r_frame_rate;
        if(fr.num && fr.den) vi.fps = (double)fr.num / fr.den;
    }
    if(pb.fmt->duration > 0)
        vi.duration = (double)pb.fmt->duration / AV_TIME_BASE;
    else {
        auto dur = pb.fmt->streams[pb.stream_idx]->duration;
        if(dur > 0 && pb.time_base.den)
            vi.duration = dur * av_q2d(pb.time_base);
    }
    pb.duration = vi.duration;
    vi.loaded = true;

    // decode first frame (start paused)
    decode_one_frame(pb);

    return true;
}

string open_file_dialog(HWND hwnd) {
    wchar_t path[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Video Files\0*.mp4;*.avi;*.mov;*.mkv;*.webm;*.ogg;*.flv;*.wmv\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if(GetOpenFileNameW(&ofn)) return wchar_to_utf8(path);
    return "";
}

wstring utf8_to_wchar(const string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if(len <= 0) return L"";
    wstring ws(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

string save_file_dialog(HWND hwnd, const string& default_name, int fmt_idx) {
    wchar_t path[MAX_PATH] = {};
    wstring wname = utf8_to_wchar(default_name);
    wcsncpy(path, wname.c_str(), MAX_PATH - 1);

    // build filter string for selected format
    string ext = FORMATS[fmt_idx];
    string label = string(FORMAT_LABELS[fmt_idx]) + " Files";
    string pattern = "*." + ext;
    // filter needs double null termination; build as wstring
    wstring wfilter = utf8_to_wchar(label);
    wfilter.push_back(L'\0');
    wfilter += utf8_to_wchar(pattern);
    wfilter.push_back(L'\0');
    wfilter += L"All Files";
    wfilter.push_back(L'\0');
    wfilter += L"*.*";
    wfilter.push_back(L'\0');

    wstring wext = utf8_to_wchar(ext);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = wfilter.c_str();
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = wext.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if(GetSaveFileNameW(&ofn)) return wchar_to_utf8(path);
    return "";
}

// parse "time=HH:MM:SS.xx" from ffmpeg stderr line
double parse_ffmpeg_time(const char* line) {
    const char* p = strstr(line, "time=");
    if(!p) return -1;
    p += 5;
    int h = 0, m = 0;
    double s = 0;
    if(sscanf(p, "%d:%d:%lf", &h, &m, &s) == 3) {
        return h * 3600.0 + m * 60.0 + s;
    }
    return -1;
}

void run_ffmpeg_cut(string input_path, double start, double end, string output_path) {
    double trim_duration = end - start;
    cut_progress.store(0.0);
    cut_state.store(1);

    // build command line
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -ss %.3f -to %.3f -i \"%s\" -c copy \"%s\"",
        start, end, input_path.c_str(), output_path.c_str());

    // convert to wide string for CreateProcessW
    wstring wcmd = utf8_to_wchar(string(cmd));

    // create pipe for stderr
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;
    si.hStdInput = nullptr;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(nullptr, wcmd.data(), nullptr, nullptr, TRUE,
                              CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);

    if(!ok) {
        CloseHandle(hRead);
        lock_guard<mutex> lk(cut_error_mtx);
        cut_error_msg = "FFmpeg not found in PATH";
        cut_state.store(3);
        return;
    }

    cut_process_handle = pi.hProcess;

    // read stderr for progress
    char buf[4096];
    string line_buf;
    DWORD n;
    while(ReadFile(hRead, buf, sizeof(buf) - 1, &n, nullptr) && n > 0) {
        buf[n] = 0;
        line_buf += buf;
        // process complete lines
        size_t pos;
        while((pos = line_buf.find('\r')) != string::npos ||
              (pos = line_buf.find('\n')) != string::npos) {
            string line = line_buf.substr(0, pos);
            line_buf.erase(0, pos + 1);
            double t = parse_ffmpeg_time(line.c_str());
            if(t >= 0 && trim_duration > 0) {
                double p = min(1.0, t / trim_duration);
                cut_progress.store(p);
            }
        }
    }
    CloseHandle(hRead);

    DWORD exit_code = 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    cut_process_handle = nullptr;

    if(exit_code == 0) {
        cut_progress.store(1.0);
        cut_done_path = output_path;
        cut_state.store(2);
    } else {
        lock_guard<mutex> lk(cut_error_mtx);
        cut_error_msg = "FFmpeg exited with code " + to_string(exit_code);
        cut_state.store(3);
    }
}

int main(int, char*[]) {
    SDL_Init(SDL_INIT_VIDEO);
    auto* win = SDL_CreateWindow("VideoCutter",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        INIT_W, INIT_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(win, &wminfo);
    HWND hwnd = wminfo.info.win.window;
    DragAcceptFiles(hwnd, TRUE);

    int ww = INIT_W, wh = INIT_H;
    bool running = true;
    VideoInfo vi;
    Playback pb;
    int selected_format = 0; // index into FORMATS[]
    Uint64 status_msg_ticks = 0;
    string status_msg;

    auto handle_file = [&](const string& path) {
        if(path.empty()) return;
        if(cut_state.load() == 1) {
            status_msg = "Cannot load while cutting";
            status_msg_ticks = SDL_GetTicks64();
            return;
        }
        open_video(pb, vi, path, ren);
        if(vi.loaded) {
            string title = "VideoCutter - " + vi.filename;
            SDL_SetWindowTitle(win, title.c_str());
            cut_state.store(0); // reset cut status on new file
        }
    };

    while(running) {
        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            switch(ev.type) {
                case SDL_QUIT: running = false; break;
                case SDL_KEYDOWN:
                    if(ev.key.keysym.sym == SDLK_o && !(ev.key.keysym.mod & KMOD_CTRL))
                        handle_file(open_file_dialog(hwnd));
                    if(ev.key.keysym.sym == SDLK_ESCAPE)
                        running = false;
                    if(ev.key.keysym.sym == SDLK_SPACE && vi.loaded) {
                        pb.playing = !pb.playing;
                        if(pb.playing) {
                            if(pb.eof) {
                                seek_to(pb, 0);
                                pb.eof = false;
                            }
                            pb.play_start_ticks = SDL_GetTicks64();
                            pb.play_start_time = pb.current_time;
                        }
                    }
                    // trim: I = set start, O (with ctrl) or [ = set start alt
                    if((ev.key.keysym.sym == SDLK_i ||
                        ev.key.keysym.sym == SDLK_LEFTBRACKET) && vi.loaded) {
                        pb.trim_start = pb.current_time;
                        if(pb.thumb_start) SDL_DestroyTexture(pb.thumb_start);
                        pb.thumb_start = generate_thumbnail(pb, pb.trim_start);
                    }
                    // trim: O (with ctrl) or ] = set end -- plain O is open file
                    if((ev.key.keysym.sym == SDLK_o && (ev.key.keysym.mod & KMOD_CTRL)) ||
                       ev.key.keysym.sym == SDLK_RIGHTBRACKET) {
                        if(vi.loaded) {
                            pb.trim_end = pb.current_time;
                            if(pb.thumb_end) SDL_DestroyTexture(pb.thumb_end);
                            pb.thumb_end = generate_thumbnail(pb, pb.trim_end);
                        }
                    }
                    // R = reset trim points
                    if(ev.key.keysym.sym == SDLK_r && vi.loaded) {
                        pb.trim_start = pb.trim_end = -1;
                        if(pb.thumb_start) { SDL_DestroyTexture(pb.thumb_start); pb.thumb_start = nullptr; }
                        if(pb.thumb_end) { SDL_DestroyTexture(pb.thumb_end); pb.thumb_end = nullptr; }
                    }
                    // F = cycle format forward
                    if(ev.key.keysym.sym == SDLK_f) {
                        selected_format = (selected_format + 1) % NUM_FORMATS;
                    }
                    // Left/Right arrows cycle format
                    if(ev.key.keysym.sym == SDLK_LEFT && vi.loaded) {
                        selected_format = (selected_format + NUM_FORMATS - 1) % NUM_FORMATS;
                    }
                    if(ev.key.keysym.sym == SDLK_RIGHT && vi.loaded) {
                        selected_format = (selected_format + 1) % NUM_FORMATS;
                    }
                    // C = cut
                    if(ev.key.keysym.sym == SDLK_c && vi.loaded) {
                        if(cut_state.load() == 1) {
                            status_msg = "Cut already in progress...";
                            status_msg_ticks = SDL_GetTicks64();
                        } else if(pb.trim_start < 0 || pb.trim_end < 0) {
                            status_msg = "Set trim points first (I and ])";
                            status_msg_ticks = SDL_GetTicks64();
                        } else if(pb.trim_end <= pb.trim_start) {
                            status_msg = "Invalid: end <= start";
                            status_msg_ticks = SDL_GetTicks64();
                        } else {
                            string def_name = stem_of(vi.filename) + "_cut." + FORMATS[selected_format];
                            string out = save_file_dialog(hwnd, def_name, selected_format);
                            if(!out.empty()) {
                                cut_state.store(0);
                                cut_progress.store(0.0);
                                if(cut_thread.joinable()) cut_thread.join();
                                cut_thread = thread(run_ffmpeg_cut,
                                    vi.filepath, pb.trim_start, pb.trim_end, out);
                                cut_thread.detach();
                            }
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    int mx = ev.button.x, my = ev.button.y;
                    int ctrl_y = wh - BOT_H - CTRL_H;
                    // play/pause button area: left side of control bar
                    int btn_x = 10, btn_y = ctrl_y + 5, btn_sz = CTRL_H - 10;
                    if(vi.loaded && mx >= btn_x && mx <= btn_x + btn_sz &&
                       my >= btn_y && my <= btn_y + btn_sz) {
                        pb.playing = !pb.playing;
                        if(pb.playing) {
                            if(pb.eof) { seek_to(pb, 0); pb.eof = false; }
                            pb.play_start_ticks = SDL_GetTicks64();
                            pb.play_start_time = pb.current_time;
                        }
                    }
                    // format button clicks
                    if(vi.loaded) {
                        int trim_panel_y_ = wh - BOT_H - CTRL_H - TRIM_H;
                        int fmt_y_ = trim_panel_y_ + TRIM_H - 28;
                        int fmt_btn_w_ = 48, fmt_btn_h_ = 20, fmt_gap_ = 4;
                        int thumb2_x_ = 10 + THUMB_W + 20;
                        int info_x_ = thumb2_x_ + THUMB_W + 20;
                        rep(fi, 0, NUM_FORMATS) {
                            int bx = info_x_ + fi * (fmt_btn_w_ + fmt_gap_);
                            if(mx >= bx && mx <= bx + fmt_btn_w_ &&
                               my >= fmt_y_ && my <= fmt_y_ + fmt_btn_h_) {
                                selected_format = fi;
                            }
                        }
                    }
                    // timeline bar click -- check for trim marker drag first
                    int tl_x = 60 + btn_sz;
                    int tl_w = ww - tl_x - 160;
                    int tl_y = ctrl_y + CTRL_H/2 - 6;
                    int tl_h = 12;
                    if(vi.loaded && tl_w > 0 && my >= tl_y - 8 && my <= tl_y + tl_h + 8) {
                        // check if near a trim marker (within 8 pixels)
                        bool grabbed_trim = false;
                        if(pb.trim_start >= 0 && pb.duration > 0) {
                            int sx = tl_x + (int)(pb.trim_start / pb.duration * tl_w);
                            if(abs(mx - sx) < 10) {
                                pb.dragging_trim = 1;
                                grabbed_trim = true;
                            }
                        }
                        if(!grabbed_trim && pb.trim_end >= 0 && pb.duration > 0) {
                            int ex = tl_x + (int)(pb.trim_end / pb.duration * tl_w);
                            if(abs(mx - ex) < 10) {
                                pb.dragging_trim = 2;
                                grabbed_trim = true;
                            }
                        }
                        if(!grabbed_trim && mx >= tl_x && mx <= tl_x + tl_w) {
                            pb.dragging_timeline = true;
                            double frac = (double)(mx - tl_x) / tl_w;
                            frac = max(0.0, min(1.0, frac));
                            seek_to(pb, frac * pb.duration);
                        }
                    }
                    break;
                }
                case SDL_MOUSEMOTION:
                    if(vi.loaded) {
                        int mx = ev.motion.x;
                        int ctrl_y = wh - BOT_H - CTRL_H;
                        int btn_sz = CTRL_H - 10;
                        int tl_x = 60 + btn_sz;
                        int tl_w = ww - tl_x - 160;
                        if(tl_w > 0) {
                            double frac = (double)(mx - tl_x) / tl_w;
                            frac = max(0.0, min(1.0, frac));
                            if(pb.dragging_timeline) {
                                seek_to(pb, frac * pb.duration);
                            } else if(pb.dragging_trim == 1) {
                                pb.trim_start = frac * pb.duration;
                            } else if(pb.dragging_trim == 2) {
                                pb.trim_end = frac * pb.duration;
                            }
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    // regenerate thumbnail on trim marker release
                    if(pb.dragging_trim == 1 && pb.trim_start >= 0) {
                        if(pb.thumb_start) SDL_DestroyTexture(pb.thumb_start);
                        pb.thumb_start = generate_thumbnail(pb, pb.trim_start);
                    }
                    if(pb.dragging_trim == 2 && pb.trim_end >= 0) {
                        if(pb.thumb_end) SDL_DestroyTexture(pb.thumb_end);
                        pb.thumb_end = generate_thumbnail(pb, pb.trim_end);
                    }
                    pb.dragging_timeline = false;
                    pb.dragging_trim = 0;
                    break;
                case SDL_DROPFILE:
                    handle_file(string(ev.drop.file));
                    SDL_free(ev.drop.file);
                    break;
                case SDL_WINDOWEVENT:
                    if(ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                        ww = ev.window.data1;
                        wh = ev.window.data2;
                    }
                    break;
            }
        }

        // advance playback if playing
        if(pb.playing && vi.loaded && !pb.eof) {
            Uint64 now = SDL_GetTicks64();
            double elapsed = (now - pb.play_start_ticks) / 1000.0;
            double target = pb.play_start_time + elapsed;
            // decode frames until we catch up to wall clock
            while(pb.current_time < target && !pb.eof) {
                if(!decode_one_frame(pb)) break;
            }
            if(pb.eof) pb.playing = false;
        }

        // -- render --
        SDL_SetRenderDrawColor(ren, BG.r, BG.g, BG.b, 255);
        SDL_RenderClear(ren);

        // top bar
        SDL_SetRenderDrawColor(ren, TOP_BAR.r, TOP_BAR.g, TOP_BAR.b, 255);
        SDL_Rect top = {0, 0, ww, TOP_H};
        SDL_RenderFillRect(ren, &top);
        draw_text(12, (TOP_H - 8*FONT_SCALE)/2, "VideoCutter", TEXT_COLOR);

        // trim info panel (between video and control bar)
        int trim_panel_y = wh - BOT_H - CTRL_H - TRIM_H;
        int ctrl_y = wh - BOT_H - CTRL_H;

        SDL_SetRenderDrawColor(ren, CTRL_BG.r, CTRL_BG.g, CTRL_BG.b + 10, 255);
        SDL_Rect trim_bg = {0, trim_panel_y, ww, TRIM_H};
        SDL_RenderFillRect(ren, &trim_bg);

        // control bar
        SDL_SetRenderDrawColor(ren, CTRL_BG.r, CTRL_BG.g, CTRL_BG.b, 255);
        SDL_Rect ctrl = {0, ctrl_y, ww, CTRL_H};
        SDL_RenderFillRect(ren, &ctrl);

        // bottom status bar
        auto& bc = vi.loaded ? BOT_BAR_LOADED : BOT_BAR;
        SDL_SetRenderDrawColor(ren, bc.r, bc.g, bc.b, 255);
        SDL_Rect bot = {0, wh - BOT_H, ww, BOT_H};
        SDL_RenderFillRect(ren, &bot);

        // status message (auto-hide after 3 seconds)
        bool show_status = !status_msg.empty() && (SDL_GetTicks64() - status_msg_ticks < 3000);
        if(show_status) {
            draw_text(12, wh - BOT_H + (BOT_H - 8*FONT_SCALE)/2,
                status_msg.c_str(), {255, 200, 100, 255});
        } else if(vi.loaded) {
            draw_text(12, wh - BOT_H + (BOT_H - 8*FONT_SCALE)/2,
                vi.filename.c_str(), TEXT_COLOR);
        } else {
            draw_text(12, wh - BOT_H + (BOT_H - 8*FONT_SCALE)/2,
                "No file loaded", DIM_TEXT);
        }

        // content area: video or placeholder
        int content_y = TOP_H;
        int content_h = trim_panel_y - TOP_H;
        int cx = ww / 2;
        int cy = content_y + content_h / 2;

        if(vi.loaded && pb.has_frame) {
            // calculate aspect-ratio-correct dest rect
            double vid_aspect = (double)vi.width / vi.height;
            double area_aspect = (double)ww / content_h;
            int dw, dh;
            if(vid_aspect > area_aspect) {
                dw = ww;
                dh = (int)(ww / vid_aspect);
            } else {
                dh = content_h;
                dw = (int)(content_h * vid_aspect);
            }
            SDL_Rect dst = {(ww - dw)/2, content_y + (content_h - dh)/2, dw, dh};
            SDL_RenderCopy(ren, pb.tex, nullptr, &dst);
        } else if(vi.loaded) {
            draw_text_centered(cx, cy, "Decoding...", DIM_TEXT);
        } else if(!vi.error.empty()) {
            draw_text_centered(cx, cy - 8*FONT_SCALE, vi.error.c_str(), {255,100,100,255});
            draw_text_centered(cx, cy + 8, "Press O to open a file", DIM_TEXT);
        } else {
            draw_text_centered(cx, cy - 8*FONT_SCALE,
                "Drop video file or press O to open", DIM_TEXT);
        }

        // draw trim panel content
        if(vi.loaded) {
            int pad = 10;
            int thumb_x = pad;
            int thumb_y = trim_panel_y + 5;

            // start thumbnail
            if(pb.thumb_start) {
                int tw, th;
                SDL_QueryTexture(pb.thumb_start, nullptr, nullptr, &tw, &th);
                SDL_Rect dst = {thumb_x, thumb_y, tw, th};
                SDL_RenderCopy(ren, pb.thumb_start, nullptr, &dst);
                // label
                string lbl = "IN: " + format_time(pb.trim_start);
                draw_text(thumb_x, thumb_y + th + 2, lbl.c_str(), TRIM_START_COL, 1);
            } else {
                SDL_SetRenderDrawColor(ren, 40, 40, 60, 255);
                SDL_Rect r = {thumb_x, thumb_y, THUMB_W, THUMB_H};
                SDL_RenderFillRect(ren, &r);
                draw_text_centered(thumb_x + THUMB_W/2, thumb_y + THUMB_H/2 - 4,
                    "Press I", DIM_TEXT, 1);
            }

            // end thumbnail
            int thumb2_x = thumb_x + THUMB_W + 20;
            if(pb.thumb_end) {
                int tw, th;
                SDL_QueryTexture(pb.thumb_end, nullptr, nullptr, &tw, &th);
                SDL_Rect dst = {thumb2_x, thumb_y, tw, th};
                SDL_RenderCopy(ren, pb.thumb_end, nullptr, &dst);
                string lbl = "OUT: " + format_time(pb.trim_end);
                draw_text(thumb2_x, thumb_y + th + 2, lbl.c_str(), TRIM_END_COL, 1);
            } else {
                SDL_SetRenderDrawColor(ren, 40, 40, 60, 255);
                SDL_Rect r = {thumb2_x, thumb_y, THUMB_W, THUMB_H};
                SDL_RenderFillRect(ren, &r);
                draw_text_centered(thumb2_x + THUMB_W/2, thumb_y + THUMB_H/2 - 4,
                    "Press ]", DIM_TEXT, 1);
            }

            // trim range info
            int info_x = thumb2_x + THUMB_W + 20;
            if(pb.trim_start >= 0 && pb.trim_end >= 0 && pb.trim_end <= pb.trim_start) {
                draw_text(info_x, thumb_y + 10,
                    "Warning: END <= START", {255, 100, 100, 255}, 1);
            } else if(pb.trim_start >= 0 && pb.trim_end >= 0) {
                double dur = pb.trim_end - pb.trim_start;
                string info = "Duration: " + format_time(dur);
                draw_text(info_x, thumb_y + 5, info.c_str(), TEXT_COLOR, 1);
                draw_text(info_x, thumb_y + 17,
                    "R = Reset  C = Cut", DIM_TEXT, 1);
            }

            // format selector buttons (row of clickable rects)
            int fmt_y = trim_panel_y + TRIM_H - 28;
            int fmt_btn_w = 48, fmt_btn_h = 20, fmt_gap = 4;
            int fmt_total_w = NUM_FORMATS * (fmt_btn_w + fmt_gap) - fmt_gap;
            int fmt_x = info_x;
            rep(fi, 0, NUM_FORMATS) {
                int bx = fmt_x + fi * (fmt_btn_w + fmt_gap);
                int by = fmt_y;
                bool sel = (fi == selected_format);
                if(sel) {
                    SDL_SetRenderDrawColor(ren, ACCENT.r, ACCENT.g, ACCENT.b, 255);
                } else {
                    SDL_SetRenderDrawColor(ren, 50, 50, 70, 255);
                }
                SDL_Rect r = {bx, by, fmt_btn_w, fmt_btn_h};
                SDL_RenderFillRect(ren, &r);
                SDL_Color tc = sel ? SDL_Color{255,255,255,255} : DIM_TEXT;
                int tw = text_width(FORMAT_LABELS[fi], 1);
                draw_text(bx + (fmt_btn_w - tw)/2, by + 6, FORMAT_LABELS[fi], tc, 1);
            }

            // cut progress / status
            int cs = cut_state.load();
            if(cs == 1) {
                // progress bar
                double p = cut_progress.load();
                int bar_x = info_x, bar_y = thumb_y + 32;
                int bar_w = 200, bar_h = 14;
                SDL_SetRenderDrawColor(ren, 40, 40, 60, 255);
                SDL_Rect bg_r = {bar_x, bar_y, bar_w, bar_h};
                SDL_RenderFillRect(ren, &bg_r);
                SDL_SetRenderDrawColor(ren, 40, 200, 80, 255);
                SDL_Rect fg_r = {bar_x, bar_y, (int)(p * bar_w), bar_h};
                SDL_RenderFillRect(ren, &fg_r);
                char pct[32];
                snprintf(pct, sizeof(pct), "Cutting... %d%%", (int)(p * 100));
                draw_text(bar_x + bar_w + 8, bar_y + 3, pct, TEXT_COLOR, 1);
            } else if(cs == 2) {
                draw_text(info_x, thumb_y + 32, "Done!", {40, 200, 80, 255}, 1);
                // show path (truncated)
                string msg = cut_done_path;
                if(msg.size() > 40) msg = "..." + msg.substr(msg.size() - 37);
                draw_text(info_x, thumb_y + 44, msg.c_str(), DIM_TEXT, 1);
            } else if(cs == 3) {
                lock_guard<mutex> lk(cut_error_mtx);
                draw_text(info_x, thumb_y + 32, cut_error_msg.c_str(), {255, 100, 100, 255}, 1);
            }
        }

        // draw controls if video loaded
        if(vi.loaded) {
            int btn_sz = CTRL_H - 10;
            int btn_x = 10, btn_y = ctrl_y + 5;

            // play/pause button
            SDL_SetRenderDrawColor(ren, ACCENT.r, ACCENT.g, ACCENT.b, 255);
            if(pb.playing) {
                // pause icon: two vertical bars
                int bar_w = btn_sz / 5;
                int gap = btn_sz / 5;
                int lx = btn_x + btn_sz/2 - gap/2 - bar_w;
                SDL_Rect b1 = {lx, btn_y + 4, bar_w, btn_sz - 8};
                SDL_Rect b2 = {lx + bar_w + gap, btn_y + 4, bar_w, btn_sz - 8};
                SDL_RenderFillRect(ren, &b1);
                SDL_RenderFillRect(ren, &b2);
            } else {
                // play icon: triangle using horizontal lines
                int tx = btn_x + 6, ty = btn_y + 4;
                int th = btn_sz - 8;
                rep(i, 0, th) {
                    if(i < th/2) {
                        SDL_RenderDrawLine(ren, tx, ty + i, tx + i, ty + i);
                    } else {
                        int ri = th - 1 - i;
                        SDL_RenderDrawLine(ren, tx, ty + i, tx + ri, ty + i);
                    }
                }
            }

            // timeline bar
            int tl_x = 60 + btn_sz;
            int tl_w = ww - tl_x - 160;
            int tl_y = ctrl_y + CTRL_H/2 - 6;
            int tl_h = 12;
            if(tl_w > 10) {
                // background
                SDL_SetRenderDrawColor(ren, CTRL_BG.r + 20, CTRL_BG.g + 20, CTRL_BG.b + 20, 255);
                SDL_Rect bg_bar = {tl_x, tl_y, tl_w, tl_h};
                SDL_RenderFillRect(ren, &bg_bar);

                // trim region highlight (semi-transparent fill between markers)
                if(pb.trim_start >= 0 && pb.trim_end >= 0 && pb.duration > 0) {
                    double s_frac = pb.trim_start / pb.duration;
                    double e_frac = pb.trim_end / pb.duration;
                    s_frac = max(0.0, min(1.0, s_frac));
                    e_frac = max(0.0, min(1.0, e_frac));
                    int sx = tl_x + (int)(s_frac * tl_w);
                    int ex = tl_x + (int)(e_frac * tl_w);
                    if(ex > sx) {
                        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(ren, TRIM_FILL_COL.r, TRIM_FILL_COL.g, TRIM_FILL_COL.b, TRIM_FILL_COL.a);
                        SDL_Rect fill = {sx, tl_y, ex - sx, tl_h};
                        SDL_RenderFillRect(ren, &fill);
                        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
                    }
                }

                // progress
                double frac = pb.duration > 0 ? pb.current_time / pb.duration : 0;
                frac = max(0.0, min(1.0, frac));
                int prog_w = (int)(frac * tl_w);
                SDL_SetRenderDrawColor(ren, ACCENT.r, ACCENT.g, ACCENT.b, 255);
                SDL_Rect prog = {tl_x, tl_y, prog_w, tl_h};
                SDL_RenderFillRect(ren, &prog);

                // trim markers (vertical lines)
                if(pb.trim_start >= 0 && pb.duration > 0) {
                    int sx = tl_x + (int)(pb.trim_start / pb.duration * tl_w);
                    SDL_SetRenderDrawColor(ren, TRIM_START_COL.r, TRIM_START_COL.g, TRIM_START_COL.b, 255);
                    SDL_Rect marker = {sx - 2, tl_y - 6, 4, tl_h + 12};
                    SDL_RenderFillRect(ren, &marker);
                }
                if(pb.trim_end >= 0 && pb.duration > 0) {
                    int ex = tl_x + (int)(pb.trim_end / pb.duration * tl_w);
                    SDL_SetRenderDrawColor(ren, TRIM_END_COL.r, TRIM_END_COL.g, TRIM_END_COL.b, 255);
                    SDL_Rect marker = {ex - 2, tl_y - 6, 4, tl_h + 12};
                    SDL_RenderFillRect(ren, &marker);
                }

                // handle
                int hx = tl_x + prog_w - 6;
                SDL_Rect handle = {hx, tl_y - 3, 12, tl_h + 6};
                SDL_SetRenderDrawColor(ren, 240, 240, 240, 255);
                SDL_RenderFillRect(ren, &handle);
            }

            // time display
            char time_buf[64];
            string cur_t = format_time(pb.current_time);
            string dur_t = format_time(pb.duration);
            snprintf(time_buf, sizeof(time_buf), "%s/%s", cur_t.c_str(), dur_t.c_str());
            int time_x = ww - 150;
            int time_y = ctrl_y + (CTRL_H - 8*FONT_SCALE)/2;
            draw_text(time_x, time_y, time_buf, TEXT_COLOR);
        }

        SDL_RenderPresent(ren);
    }

    // terminate FFmpeg process if still running
    if(cut_state.load() == 1 && cut_process_handle) {
        TerminateProcess(cut_process_handle, 1);
    }

    pb.close();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
