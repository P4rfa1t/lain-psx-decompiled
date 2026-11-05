#include "setup_ui.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <thread>
#include <string>
#include <vector>

#include "discs.h"
#include "packs.h"
#include "sub_download.h"
#include "mods_ui.h"
#include "unused_gallery.h"
#include "app_icon.h"
#include "i18n.h"
#include "ui_font.h"
#include "touch.h"
#include "PsyX/PsyX_logger.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#ifndef __SWITCH__
#include "tinyfiledialogs.h"
#else
#include <dirent.h>
#endif

namespace {

const char *disc_status(int disc) {
    char path[1024];
    return discs_imported(settings_data_dir(), disc, path, sizeof path) ? "ready" : "not imported";
}

struct WindowSize {
    int w, h;
};
const WindowSize kSizes[] = {{960, 720}, {1280, 960}, {1440, 1080}, {1600, 1200}, {1920, 1440}, {2560, 1920}};

}  // namespace

namespace {

// "Unused" tab: decoded from the player's discs when first opened
struct Gallery {
    std::atomic<int> state{0};  // 0 not loaded, 1 loading, 2 ready, 3 failed
    std::vector<GallerySection> sections;
    std::string error;
    std::vector<std::vector<GLuint>> textures;  // per item (flattened over sections), per frame
    bool uploaded = false;
};

void gallery_start(Gallery &g) {
    char d1[1024], d2[1024];
    bool have1 = discs_imported(settings_data_dir(), DISC_1, d1, sizeof d1);
    bool have2 = discs_imported(settings_data_dir(), DISC_2, d2, sizeof d2);
    if (!have1) {
        g.error = "Import disc 1 first (Setup tab).";
        g.state = 3;
        return;
    }
    g.state = 1;
    std::string p1 = d1, p2 = have2 ? d2 : "";
    std::thread([&g, p1, p2] {
        bool ok = gallery_load(p1.c_str(), p2.empty() ? nullptr : p2.c_str(), g.sections, g.error);
        g.state = ok ? 2 : 3;
    }).detach();
}

void gallery_upload(Gallery &g) {
    for (auto &sec : g.sections) {
        for (auto &it : sec.items) {
            std::vector<GLuint> tex(it.frames.size());
            glGenTextures((GLsizei)tex.size(), tex.data());
            for (size_t f = 0; f < tex.size(); f++) {
                glBindTexture(GL_TEXTURE_2D, tex[f]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, it.w, it.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, it.frames[f].data());
            }
            g.textures.push_back(std::move(tex));
            it.frames.clear();  // on the GPU now
            it.frames.shrink_to_fit();
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    g.uploaded = true;
}

// Playing the gallery's sounds (a plain SDL audio device, launcher only)
SDL_AudioDeviceID g_audio;
const GalleryItem *g_playing;

void sound_stop() {
    if (g_audio) {
        SDL_ClearQueuedAudio(g_audio);
    }
    g_playing = nullptr;
}

void sound_play(const GalleryItem &it) {
    if (!g_audio) {
        if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return;
        SDL_AudioSpec want = {}, have;
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        g_audio = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (!g_audio) return;
        SDL_PauseAudioDevice(g_audio, 0);
    }
    // Linear resampling to 44.1 kHz stereo.
    const size_t in_frames = it.pcm.size() / it.channels;
    const size_t out_frames = (size_t)((double)in_frames * 44100 / it.rate);
    std::vector<int16_t> out(out_frames * 2);
    for (size_t i = 0; i < out_frames; i++) {
        double pos = (double)i * it.rate / 44100;
        size_t a = (size_t)pos, b = std::min(a + 1, in_frames - 1);
        double f = pos - a;
        for (int c = 0; c < 2; c++) {
            int ch = it.channels == 2 ? c : 0;
            double v = it.pcm[a * it.channels + ch] * (1 - f) + it.pcm[b * it.channels + ch] * f;
            out[i * 2 + c] = (int16_t)v;
        }
    }
    SDL_ClearQueuedAudio(g_audio);
    SDL_QueueAudio(g_audio, out.data(), (Uint32)(out.size() * sizeof(int16_t)));
    g_playing = &it;
}

void sound_close() {
    if (g_audio) {
        SDL_CloseAudioDevice(g_audio);
        g_audio = 0;
    }
    g_playing = nullptr;
}

void gallery_draw(Gallery &g) {
    if (g.state == 0) {
        gallery_start(g);
    }
    if (g.state == 1) {
        ImGui::TextDisabled("%s", tr("Reading your discs..."));
        return;
    }
    if (g.state == 3) {
        ImGui::TextWrapped("%s", g.error.c_str());
        if (ImGui::Button(tr("Try again"))) {
            g.state = 0; // e.g. after importing disc 1
        }
        return;
    }
    if (!g.uploaded) {
        gallery_upload(g);
    }
    ImGui::TextWrapped("%s", tr("Things on the Serial Experiments Lain discs that the game never shows, "
                                "decoded from your own discs. Found while decompiling the game."));
    const double now = ImGui::GetTime();
    size_t n = 0;
    for (auto &sec : g.sections) {
        ImGui::SeparatorText(tr(sec.title.c_str()));
        ImGui::TextWrapped("%s", tr(sec.intro.c_str()));
        const float avail = ImGui::GetContentRegionAvail().x;
        float x = 0;
        for (size_t i = 0; i < sec.items.size(); i++, n++) {
            const GalleryItem &it = sec.items[i];
            const std::vector<GLuint> &tex = g.textures[n];
            if (tex.empty()) {
                if (it.pcm.empty()) continue;
                // A sound: Play/Stop, its length, what it is.
                if (g_playing == &it && g_audio && SDL_GetQueuedAudioSize(g_audio) == 0) g_playing = nullptr;
                ImGui::PushID((int)n);
                if (ImGui::Button(g_playing == &it ? tr("Stop") : tr("Play sound"), ImVec2(70, 0))) {
                    if (g_playing == &it) sound_stop();
                    else sound_play(it);
                }
                ImGui::PopID();
                ImGui::SameLine();
                ImGui::Text("%s", tr(it.title.c_str()));
                ImGui::SameLine();
                ImGui::TextDisabled(tr("(%.1f s)"), it.pcm.size() / (double)it.channels / it.rate);
                ImGui::TextWrapped("%s", tr(it.caption.c_str()));
                x = 0;
                continue;
            }
            // Small pictures are enlarged (whole pixels), big ones fit a 256-pixel box.
            float scale = std::max(1.0f, std::floor(160.0f / std::max(it.w, it.h)));
            if (it.w * scale > 256 || it.h * scale > 256) scale = 256.0f / std::max(it.w, it.h);
            const ImVec2 size(it.w * scale, it.h * scale);
            const float cell = std::max(size.x, 170.0f);
            if (i > 0 && x + cell <= avail) {
                ImGui::SameLine();
            } else {
                x = 0;
            }
            x += cell + ImGui::GetStyle().ItemSpacing.x;
            size_t frame = it.fps > 0 ? (size_t)(now * it.fps) % tex.size() : 0;
            ImGui::BeginGroup();
            ImGui::Image((ImTextureID)(intptr_t)tex[frame], size);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cell);
            ImGui::TextUnformatted(tr(it.title.c_str()));
            if (it.fps > 0) {
                ImGui::SameLine();
                ImGui::TextDisabled(tr("(%zu frames)"), tex.size());
            }
            ImGui::TextDisabled("%s", tr(it.caption.c_str()));
            ImGui::PopTextWrapPos();
            ImGui::EndGroup();
        }
    }
}

}  // namespace

#if defined(__SWITCH__)
/* The launcher artwork is a plain RGBA file next to settings.ini (written by
 * the package); 512x512, GDI+-style BGRA byte order. */
static GLuint load_logo_texture(void) {
    char path[1100];
    snprintf(path, sizeof path, "%slain_logo.rgba", settings_data_dir());
    FILE *f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    const int W = 512, H = 512;
    std::vector<unsigned char> px((size_t)W * H * 4);
    const size_t n = fread(px.data(), 1, px.size(), f);
    fclose(f);
    if (n != px.size()) {
        return 0;
    }
    for (size_t i = 0; i + 3 < px.size(); i += 4) {
        const unsigned char t = px[i];
        px[i] = px[i + 2];
        px[i + 2] = t;
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
#endif

extern "C" int setup_ui_run(Settings *s) {
#ifdef __SWITCH__
    /* No display controls on the console: fullscreen, 2x internal resolution. */
    s->fullscreen = 1;
    s->width = 1280;
    s->height = 720;
    if (s->render_scale == 0) {
        s->render_scale = 2;
    }
#endif
#if defined(__SWITCH__)
    const char *glsl = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_Window *win = SDL_CreateWindow(tr("Serial Experiments Lain - Setup"), SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, 1280, 720,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
#else
    const char *glsl = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_Window *win = SDL_CreateWindow(tr("Serial Experiments Lain - Setup"), SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, 720, 640,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
    if (!win) {
        fprintf(stderr, "setup window: %s\n", SDL_GetError());
        return 0;
    }
    app_icon_apply(win);
    SDL_GLContext gl = SDL_GL_CreateContext(win);
    if (!gl) {
        fprintf(stderr, "setup window: no GL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        return 0;
    }
    SDL_GL_MakeCurrent(win, gl);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
#if defined(__SWITCH__)
    // No mouse or keyboard on the console: navigate with the pad.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    /* Light, rounded and roomy, like the console's own settings screens. */
    ImGui::StyleColorsLight();
    {
        ImGuiStyle &st = ImGui::GetStyle();
        /* Scale the stock sizes first, then set ours in real pixels. */
        st.ScaleAllSizes(30.0f / 18.0f);
        st.WindowRounding = 0;
        st.FrameRounding = 14;
        st.GrabRounding = 14;
        st.PopupRounding = 14;
        st.WindowPadding = ImVec2(56, 30);
        st.FramePadding = ImVec2(22, 14);
        st.ItemSpacing = ImVec2(18, 16);
        st.ItemInnerSpacing = ImVec2(12, 10);
        st.TouchExtraPadding = ImVec2(10, 10);
        st.Colors[ImGuiCol_WindowBg] = ImVec4(1, 1, 1, 1);
        st.Colors[ImGuiCol_Text] = ImVec4(0.09f, 0.09f, 0.10f, 1);
        st.Colors[ImGuiCol_TextDisabled] = ImVec4(0.46f, 0.46f, 0.49f, 1);
        st.Colors[ImGuiCol_PopupBg] = ImVec4(1, 1, 1, 1);
        st.Colors[ImGuiCol_Border] = ImVec4(0.90f, 0.90f, 0.92f, 1);
        st.Colors[ImGuiCol_Separator] = ImVec4(0.86f, 0.86f, 0.88f, 1);
        st.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.62f, 0.89f, 1);
        st.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.70f, 0.95f, 1);
        st.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.55f, 0.82f, 1);
        st.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.62f, 0.89f, 1);
        st.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.70f, 0.95f, 1);
        st.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.55f, 0.82f, 1);
        /* No ImGui focus rectangle: the cards and buttons show focus themselves. */
        st.Colors[ImGuiCol_NavCursor] = ImVec4(0, 0, 0, 0);
    }
#else
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0;
#endif
    ImGui_ImplSDL2_InitForOpenGL(win, gl);
    ImGui_ImplOpenGL3_Init(glsl);
#if defined(__SWITCH__)
    // 720p on a 6.2" screen: 18 px is unreadable at arm's length.
    ui_font_load(30.0f);
    static GLuint logo_tex = load_logo_texture();
#else
    ui_font_load(18.0f);
#endif

    std::vector<PackOption> text_langs = find_packs("lang", tr("Japanese (original)"));
    std::vector<PackOption> voice_langs = find_packs("dub", tr("Japanese (original)"));
    // Static: their worker threads write into them and may still be running when the
    // window closes (Play, Quit), so they must outlive this function.
    static ImportJob job;
    static SubJob subjob;
    job = ImportJob{};
    sub_cleanup_leftovers();
    static Gallery gallery; // static: its loader thread may outlive a quick close
    std::string sub_message;
    std::vector<std::string> queue;  // discs waiting to import
    std::string last_message;
    int result = -1;

    while (result < 0) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            /* On the Switch the pad is fed below from libnx with Nintendo
             * semantics (A = confirm). SDL would add the Xbox-positional
             * mapping on top of it (its B button is the bottom one, so it
             * would flip confirm/cancel), so its pad events are dropped. */
#ifdef __SWITCH__
            if (ev.type != SDL_CONTROLLERBUTTONDOWN && ev.type != SDL_CONTROLLERBUTTONUP &&
                ev.type != SDL_CONTROLLERAXISMOTION)
#endif
            ImGui_ImplSDL2_ProcessEvent(&ev);
#ifndef __SWITCH__
            if (ev.type == SDL_FINGERDOWN || ev.type == SDL_FINGERMOTION || ev.type == SDL_FINGERUP) {
                lain_touch_to_imgui(&ev, io.DisplaySize.x, io.DisplaySize.y);
            }
#endif
            if (ev.type == SDL_QUIT) {
                result = 0;
            } else if (ev.type == SDL_DROPFILE) {
                queue.push_back(ev.drop.file);
                SDL_free(ev.drop.file);
            }
        }
        if (job.state != IMPORT_RUNNING && !queue.empty()) {
            discs_import_start(&job, queue.front().c_str(), settings_data_dir());
            queue.erase(queue.begin());
        }
        if (job.state == IMPORT_DONE || job.state == IMPORT_FAILED) {
            last_message = job.message;
            job.state = IMPORT_IDLE;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        lain_switch_input_feed(io.DisplaySize.x, io.DisplaySize.y);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("setup", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

#if defined(__SWITCH__)
        ImGui::PushFont(nullptr, 46.0f);
        ImGui::TextUnformatted("Serial Experiments Lain");
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 6));
#else
        ImGui::TextUnformatted("Serial Experiments Lain");
        ImGui::TextDisabled("%s", tr("Native client. Game data comes from your own discs."));
        ImGui::Separator();
#endif

        const bool busy = job.state == IMPORT_RUNNING;
        // A finished subtitle download/import (polled here, whatever tab is open).
        if (subjob.state == SubJob::DONE || subjob.state == SubJob::FAILED) {
            sub_message = subjob.message;
            if (subjob.state == SubJob::DONE) {
                text_langs = find_packs("lang", tr("Japanese (original)"));
                snprintf(s->text_lang, sizeof s->text_lang, "%s", subjob.code.c_str());
            }
            subjob.state = SubJob::IDLE;
        }
        const bool sub_busy = subjob.state == SubJob::RUNNING;
        // Tabs; Play/Quit stay below them. The console only needs language.
#ifdef __SWITCH__
        const float footer = 176.0f;
#else
        const float footer = 64.0f;
#endif
#ifdef __SWITCH__
        const bool setup_tab = true;
        ImGui::BeginChild("setup_page", ImVec2(0, -footer));
#else
        ImGui::BeginTabBar("launcher");
        const bool setup_tab = ImGui::BeginTabItem(tr("Setup"));
        if (setup_tab) {
            ImGui::BeginChild("setup_page", ImVec2(0, -footer));
        }
#endif
        if (setup_tab) {
#ifndef __SWITCH__
        // Discs
        ImGui::SeparatorText(tr("Discs"));
        ImGui::Text(tr("Disc 1 (Site A): %s"), tr(disc_status(DISC_1)));
        ImGui::Text(tr("Disc 2 (Site B): %s"), tr(disc_status(DISC_2)));
        ImGui::BeginDisabled(busy);
        if (ImGui::Button(tr("Add disc..."))) {
            const char *filters[] = {"*.cue", "*.bin", "*.img"};
            const char *picked = tinyfd_openFileDialog(tr("Choose a Serial Experiments Lain disc"), "", 3, filters,
                                                       tr("Disc image (.cue, .bin, .img)"), 1);
            if (picked) {
                // Multiple selections come back separated by '|'.
                std::string all = picked;
                size_t start = 0, bar;
                while ((bar = all.find('|', start)) != std::string::npos) {
                    queue.push_back(all.substr(start, bar - start));
                    start = bar + 1;
                }
                queue.push_back(all.substr(start));
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", tr("or drop .cue, .bin or .img files on this window. Either disc, any order."));
        if (busy) {
            ImGui::ProgressBar(job.progress, ImVec2(-1, 0), tr("Importing and verifying..."));
        } else if (!last_message.empty()) {
            ImGui::TextWrapped("%s", last_message.c_str());
        }
#endif

#ifndef __SWITCH__
        // Display
        ImGui::SeparatorText(tr("Display"));
        ImGui::RadioButton(tr("Windowed"), &s->fullscreen, 0);
        ImGui::SameLine();
        ImGui::RadioButton(tr("Fullscreen"), &s->fullscreen, 1);
        ImGui::BeginDisabled(s->fullscreen);
        char cur[32];
        snprintf(cur, sizeof cur, "%d x %d", s->width, s->height);
        if (ImGui::BeginCombo(tr("Window size"), cur)) {
            for (const WindowSize &ws : kSizes) {
                char label[32];
                snprintf(label, sizeof label, "%d x %d", ws.w, ws.h);
                if (ImGui::Selectable(label, ws.w == s->width && ws.h == s->height)) {
                    s->width = ws.w;
                    s->height = ws.h;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        ImGui::SliderInt(tr("Render scale"), &s->render_scale, 0, 8, s->render_scale ? "%dx" : tr("Auto"));
        ImGui::SameLine();
        if (s->render_scale) {
            ImGui::TextDisabled(tr("(%d x %d internal)"), 320 * s->render_scale, 240 * s->render_scale);
        } else {
            ImGui::TextDisabled("%s", tr("(window resolution; 1x = original pixels)"));
        }
        bool vsync = s->vsync != 0;
        if (ImGui::Checkbox(tr("VSync"), &vsync)) {
            s->vsync = vsync;
        }
        ImGui::SameLine();
        bool smooth = s->smooth != 0;
        if (ImGui::Checkbox(tr("Smooth motion (60 fps)"), &smooth)) {
            s->smooth = smooth;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", tr("12/30 fps screens get in-between frames; game speed unchanged"));
#endif

        // Language
        ImGui::SeparatorText(tr("Language"));
#ifdef __SWITCH__
        {
            /* One card per setting: label left, value right. Tap the card or
             * press A on it to switch to the next value - no drop-down, so a
             * change is one press and the option list is always readable. */
            auto switch_row = [](const char *id, const char *label, const std::vector<PackOption> &opts,
                                 const char *current, char *code, size_t cap, float width) {
                ImGui::PushID(id);
                const float row_h = 88.0f;
                const ImVec2 p = ImGui::GetCursorScreenPos();
                const bool pressed = ImGui::InvisibleButton("##row", ImVec2(width, row_h), ImGuiButtonFlags_EnableNav);
                const bool focused = ImGui::IsItemFocused();
                const bool hovered = ImGui::IsItemHovered();
                ImDrawList *dl = ImGui::GetWindowDrawList();
                bool changed = false;
                const char *shown = current;
                int cur = 0;
                for (size_t i = 0; i < opts.size(); i++) {
                    if (opts[i].code == current) {
                        cur = (int)i;
                        shown = opts[i].label.c_str();
                    }
                }
                if (pressed && !opts.empty()) {
                    const PackOption &next = opts[(size_t)(cur + 1) % opts.size()];
                    snprintf(code, cap, "%s", next.code.c_str());
                    shown = next.label.c_str();
                    changed = true;
                }
                dl->AddRectFilled(p, ImVec2(p.x + width, p.y + row_h),
                                  (focused || hovered) ? IM_COL32(0, 158, 227, 44) : IM_COL32(243, 243, 245, 255),
                                  14.0f);
                const float ty = p.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f;
                dl->AddText(ImVec2(p.x + 28, ty), IM_COL32(28, 28, 30, 255), label);
                const ImVec2 ts = ImGui::CalcTextSize(shown);
                dl->AddText(ImVec2(p.x + width - ts.x - 52, ty), IM_COL32(0, 110, 178, 255), shown);
                /* A small "next" chevron hints that a press cycles the value. */
                const float cx = p.x + width - 26, cy = p.y + row_h * 0.5f;
                dl->AddTriangleFilled(ImVec2(cx - 7, cy - 4), ImVec2(cx + 7, cy - 4), ImVec2(cx, cy + 5),
                                      IM_COL32(0, 110, 178, 255));
                ImGui::PopID();
                return changed;
            };

            /* Leave the right-hand column for the artwork. */
            const float avail = ImGui::GetContentRegionAvail().x;
            const float art = avail > 900.0f ? 360.0f : 0.0f;
            const float row_w = avail - art;

            std::vector<PackOption> ui_opts = {
                {"auto", tr("Auto (system)")}, {"zh", "简体中文"}, {"en", "English"}};
            if (switch_row("ui", tr("Interface language"), ui_opts, s->ui_lang, s->ui_lang,
                           sizeof s->ui_lang, row_w)) {
                i18n_set_language(s->ui_lang);
                lain_log("launcher: interface language %s", s->ui_lang);
                text_langs = find_packs("lang", tr("Japanese (original)"));
                voice_langs = find_packs("dub", tr("Japanese (original)"));
            }
            ImGui::Dummy(ImVec2(0, 4));
            if (switch_row("text", tr("Text / subtitles"), text_langs, s->text_lang, s->text_lang,
                           sizeof s->text_lang, row_w)) {
                lain_log("launcher: text language %s", s->text_lang);
            }
            ImGui::Dummy(ImVec2(0, 4));
            if (switch_row("voice", tr("Voices"), voice_langs, s->voice_lang, s->voice_lang,
                           sizeof s->voice_lang, row_w)) {
                lain_log("launcher: voice language %s", s->voice_lang);
            }

            /* Lain in the lower right of this page, clear of the cards. Drawn
             * into the page's own draw list so the child's scroll area clips it. */
            if (logo_tex) {
                const ImVec2 wp = ImGui::GetWindowPos();
                const ImVec2 ws = ImGui::GetWindowSize();
                const float art = 300.0f;
                const float ax = wp.x + ws.x - art - 32.0f;
                const float ay = wp.y + ws.y - art - 16.0f;
                ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)logo_tex, ImVec2(ax, ay),
                                                     ImVec2(ax + art, ay + art));
            }
        }
#else
        {
            const char *ui_codes[] = {"auto", "zh", "en"};
            const char *ui_labels[] = {tr("Auto (system)"), "简体中文", "English"};
            int cur_ui = 0;
            for (int i = 0; i < 3; i++) {
                if (strcmp(s->ui_lang, ui_codes[i]) == 0) cur_ui = i;
            }
            if (ImGui::BeginCombo(tr("Interface language"), ui_labels[cur_ui])) {
                for (int i = 0; i < 3; i++) {
                    if (ImGui::Selectable(ui_labels[i], cur_ui == i)) {
                        snprintf(s->ui_lang, sizeof s->ui_lang, "%s", ui_codes[i]);
                        i18n_set_language(s->ui_lang);
                        lain_log("launcher: interface language %s", ui_codes[i]);
                        text_langs = find_packs("lang", tr("Japanese (original)"));
                        voice_langs = find_packs("dub", tr("Japanese (original)"));
                    }
                }
                ImGui::EndCombo();
            }
        }
        auto lang_combo = [](const char *label, std::vector<PackOption> &opts, char *code, size_t cap) {
            const char *shown = opts[0].label.c_str();
            for (auto &o : opts) {
                if (o.code == code) shown = o.label.c_str();
            }
            if (ImGui::BeginCombo(label, shown)) {
                for (auto &o : opts) {
                    if (ImGui::Selectable(o.label.c_str(), o.code == code)) {
                        snprintf(code, cap, "%s", o.code.c_str());
                    }
                }
                ImGui::EndCombo();
            }
        };
        lang_combo(tr("Text / subtitles"), text_langs, s->text_lang, sizeof s->text_lang);
        lang_combo(tr("Voices"), voice_langs, s->voice_lang, sizeof s->voice_lang);
#endif

#ifndef __SWITCH__
        // Subtitles are not part of this program: players fetch a published fan set
        // with one click, or add a folder of .ass/.srt files they already have.
        ImGui::BeginDisabled(sub_busy);
        for (int i = 0; i < kSubSourceCount; i++) {
            ImGui::PushID(i);
            if (ImGui::Button(kSubSources[i].label)) {
                sub_download_start(&subjob, kSubSources[i]);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", kSubSources[i].credit);
            ImGui::PopID();
        }
        if (ImGui::Button(tr("Add subtitle folder..."))) {
            const char *dir = tinyfd_selectFolderDialog(tr("Folder with .ass or .srt subtitle files"), "");
            if (dir) {
                sub_import_folder_start(&subjob, dir);
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
#else
        // On the console, packs are copied to the SD card by hand (see the README).
#endif
        if (sub_busy) {
            ImGui::TextDisabled("%s", tr("Working..."));
        } else if (!sub_message.empty()) {
            ImGui::TextWrapped("%s", sub_message.c_str());
        } else {
            ImGui::TextDisabled(tr("Packs live in: %slang"), settings_data_dir());
        }

        }
        if (setup_tab) {
            ImGui::EndChild();
#ifndef __SWITCH__
            ImGui::EndTabItem();
#endif
        }
#ifndef __SWITCH__
        // Tests/docs: LAIN_SETUP_TAB=mods / unused opens the launcher on that tab.
        static bool first_frame = true;
        const char *want_tab = first_frame ? getenv("LAIN_SETUP_TAB") : nullptr;
        const bool open_unused = want_tab && !strcmp(want_tab, "unused");
        const bool open_mods = want_tab && !strcmp(want_tab, "mods");
        first_frame = false;
        if (ImGui::BeginTabItem(tr("Mods"), nullptr, open_mods ? ImGuiTabItemFlags_SetSelected : 0)) {
            ImGui::BeginChild("mods_page", ImVec2(0, -footer));
            mods_tab_draw(s);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(tr("Unused"), nullptr, open_unused ? ImGuiTabItemFlags_SetSelected : 0)) {
            ImGui::BeginChild("unused_page", ImVec2(0, -footer));
            gallery_draw(gallery);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
#endif

        // Play
        ImGui::Separator();
        char p1[1024];
        bool have_disc1 = discs_imported(settings_data_dir(), DISC_1, p1, sizeof p1);
        ImGui::BeginDisabled(busy || sub_busy);
#ifdef __SWITCH__
        const ImVec2 play_size(280, 64), quit_size(160, 64);
#else
        const ImVec2 play_size(160, 40), quit_size(100, 40);
#endif
        if (ImGui::Button(tr("Play game"), play_size)) {
#ifdef __SWITCH__
            if (!have_disc1) {
                // The console has no disc browser: scan the folder again so a disc
                // dropped in while the launcher was open is found on this press.
                discs_auto_setup(settings_data_dir());
                have_disc1 = discs_imported(settings_data_dir(), DISC_1, p1, sizeof p1);
            }
#endif
            if (!have_disc1) {
                char miss[512];
#ifdef __SWITCH__
                snprintf(miss, sizeof miss, tr("Disc 1 not found: put your .img in %sdiscs/."),
                         settings_data_dir());
#else
                snprintf(miss, sizeof miss, tr("Disc 1 not found: put .cue/.bin/.img in %sdiscs/ and press Rescan discs."),
                         settings_data_dir());
#endif
                last_message = miss;
                lain_log("launcher: play with no disc 1");
            } else {
                lain_log("launcher: play (text %s, voice %s)", s->text_lang, s->voice_lang);
                s->setup_done = 1;
                settings_save(s);
                result = 1;
            }
        }
#ifdef __SWITCH__
        if (ImGui::IsItemFocused()) {
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                IM_COL32(0, 110, 178, 255), 14.0f, 0, 3.0f);
        }
#endif
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button(tr("Quit"), quit_size)) {
            result = 0;
        }
#ifdef __SWITCH__
        if (ImGui::IsItemFocused()) {
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                IM_COL32(0, 110, 178, 255), 14.0f, 0, 3.0f);
        }
#endif
        if (!have_disc1) {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", tr("Import disc 1 to play. Disc 2 is needed for Site B."));
        }
#ifdef __SWITCH__
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::PushFont(nullptr, 22.0f);
        ImGui::TextDisabled("%s", tr("A Confirm    B Back    D-pad Select    Touch to tap"));
        ImGui::TextDisabled("%s", tr("A ○    B ×    X △    Y □        + Start        - Select"));
        ImGui::PopFont();
#endif

        ImGui::End();
        ImGui::Render();
        int fbw, fbh;
        SDL_GL_GetDrawableSize(win, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
#if defined(__SWITCH__)
        glClearColor(1.0f, 1.0f, 1.0f, 1);
#else
        glClearColor(0.05f, 0.05f, 0.07f, 1);
#endif
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Tests/docs: LAIN_SETUP_SCREENSHOT=<file.bmp> saves the launcher after a few frames and quits.
        static int frames = 0;
        const char *shot = getenv("LAIN_SETUP_SCREENSHOT");
        if (shot && ++frames == 90) {
            std::vector<unsigned char> px((size_t)fbw * fbh * 4);
            glReadPixels(0, 0, fbw, fbh, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, fbw, fbh, 32, SDL_PIXELFORMAT_RGBA32);
            for (int y = 0; y < fbh; y++) {
                memcpy((unsigned char *)surf->pixels + (size_t)y * surf->pitch, &px[(size_t)(fbh - 1 - y) * fbw * 4],
                       (size_t)fbw * 4);
            }
            SDL_SaveBMP(surf, shot);
            SDL_FreeSurface(surf);
            result = 0;
        }
        SDL_GL_SwapWindow(win);
    }

    settings_save(s);
    sound_close(); // the game opens its own audio
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    return result;
}
