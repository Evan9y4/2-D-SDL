#include "text_renderer.hpp"
#include <SDL3/SDL.h>

bool TextRenderer::init(const std::string& path) {
    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        return false;
    }
    const char* base = SDL_GetBasePath();
    fontPath = base ? (std::string(base) + path) : path;
    ready = true;
    SDL_Log("TextRenderer initialized, font: %s", fontPath.c_str());
    return true;
}

void TextRenderer::shutdown() {
    for (auto& [size, font] : fontCache) {
        if (font) TTF_CloseFont(font);
    }
    fontCache.clear();
    TTF_Quit();
    ready = false;
}

TTF_Font* TextRenderer::getFont(int size) {
    if (!ready) return nullptr;
    auto it = fontCache.find(size);
    if (it != fontCache.end()) return it->second;

    TTF_Font* font = TTF_OpenFont(fontPath.c_str(), (float)size);
    if (!font) {
        SDL_Log("TTF_OpenFont failed (%s, size %d): %s",
                fontPath.c_str(), size, SDL_GetError());
        return nullptr;
    }
    fontCache[size] = font;
    return font;
}

void TextRenderer::draw(SDL_Renderer* r, const std::string& text,
                        float x, float y, int size, SDL_Color color) {
    if (text.empty()) return;
    TTF_Font* font = getFont(size);
    if (!font) return;

    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_DestroySurface(surf);
    if (!tex) return;

    float w, h;
    SDL_GetTextureSize(tex, &w, &h);
    SDL_FRect dst = { x, y, w, h };
    SDL_RenderTexture(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

void TextRenderer::drawCentered(SDL_Renderer* r, const std::string& text,
                                 float x, float y, float w, int size, SDL_Color color) {
    if (text.empty()) return;
    TTF_Font* font = getFont(size);
    if (!font) return;

    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_DestroySurface(surf);
    if (!tex) return;

    float tw, th;
    SDL_GetTextureSize(tex, &tw, &th);
    SDL_FRect dst = { x + (w - tw) / 2.f, y, tw, th };
    SDL_RenderTexture(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

float TextRenderer::measureWidth(const std::string& text, int size) {
    TTF_Font* font = getFont(size);
    if (!font) return 0.f;
    int w = 0, h = 0;
    TTF_GetStringSize(font, text.c_str(), text.size(), &w, &h);
    return (float)w;
}
