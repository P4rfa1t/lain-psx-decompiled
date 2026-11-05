#include "ui_font.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "settings.h"

static bool file_exists(const char *path) {
    if (!path || !*path) {
        return false;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}

static ImFont *add_font(const char *path, float size) {
    if (!file_exists(path)) {
        return nullptr;
    }
    ImFont *font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path, size);
    if (font) {
        SDL_Log("ui font: %s", path);
    }
    return font;
}

extern "C" ImFont *ui_font_load(float size_px) {
    ImGuiIO &io = ImGui::GetIO();
    ImFont *font = nullptr;
    const char *env = SDL_getenv("LAIN_UI_FONT");
    if (env && *env) {
        font = add_font(env, size_px);
    }

    char path[1024];
    if (!font) {
        const char *candidates[] = {"fonts/ui.ttf", "fonts/ui.otf", "fonts/ui.ttc", "font.ttf"};
        for (const char *name : candidates) {
            snprintf(path, sizeof path, "%s%s", settings_data_dir(), name);
            if ((font = add_font(path, size_px))) {
                break;
            }
        }
    }
    if (!font) {
        const char *base = SDL_GetBasePath();
        if (base) {
            snprintf(path, sizeof path, "%sfonts/ui.ttf", base);
            font = add_font(path, size_px);
        }
    }
    if (!font) {
        /* System fonts with Simplified Chinese coverage. */
        static const char *const kSystemFonts[] = {
#if defined(_WIN32)
            "C:/Windows/Fonts/msyh.ttc", "C:/Windows/Fonts/msyhbd.ttc",
            "C:/Windows/Fonts/simhei.ttf", "C:/Windows/Fonts/simsun.ttc",
            "C:/Windows/Fonts/Deng.ttf",
#elif defined(__APPLE__)
            "/System/Library/Fonts/PingFang.ttc",
            "/System/Library/Fonts/Hiragino Sans GB.ttc",
            "/System/Library/Fonts/STHeiti Medium.ttc",
#else
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
            "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
#endif
            nullptr,
        };
        for (int i = 0; kSystemFonts[i] && !font; i++) {
            font = add_font(kSystemFonts[i], size_px);
        }
    }
    if (!font) {
        SDL_Log("ui font: no CJK font found; Chinese text will not render "
                "(put a TTF at %sfonts/ui.ttf)", settings_data_dir());
        font = io.Fonts->AddFontDefault();
    }
    io.FontDefault = font;
    return font;
}
