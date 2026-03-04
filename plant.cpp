#define MINIAUDIO_IMPLEMENTATION
#include "sound.hpp"
#include <SDL3/SDL.h>

bool Sound::init() {
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        SDL_Log("Failed to initialize miniaudio engine.");
        return false;
    }
    engineReady = true;
    SDL_Log("Sound engine initialized.");
    return true;
}

void Sound::shutdown() {
    if (musicLoaded) {
        ma_sound_uninit(&musicSound);
        musicLoaded = false;
    }
    if (engineReady) {
        ma_engine_uninit(&engine);
        engineReady = false;
    }
}

bool Sound::loadSound(SoundID id, const std::string& filepath) {
    soundPaths[(int)id] = filepath;
    return true;
}

void Sound::play(SoundID id, float volume) {
    if (!engineReady) return;
    auto it = soundPaths.find((int)id);
    if (it == soundPaths.end()) return;

    ma_sound* s = new ma_sound();
    ma_result r = ma_sound_init_from_file(&engine, it->second.c_str(),
                      MA_SOUND_FLAG_ASYNC, NULL, NULL, s);
    if (r != MA_SUCCESS) { delete s; return; }
    ma_sound_set_volume(s, volume);
    ma_sound_set_looping(s, MA_FALSE);
    ma_sound_start(s);
}

bool Sound::loadMusic(const std::string& filepath) {
    if (!engineReady) return false;
    if (musicLoaded) {
        ma_sound_uninit(&musicSound);
        musicLoaded = false;
    }
    ma_result result = ma_sound_init_from_file(&engine, filepath.c_str(),
                            MA_SOUND_FLAG_STREAM, NULL, NULL, &musicSound);
    if (result != MA_SUCCESS) {
        SDL_Log("Failed to load music: %s", filepath.c_str());
        return false;
    }
    musicLoaded = true;
    return true;
}

void Sound::playMusic(bool loop) {
    if (!musicLoaded) return;
    ma_sound_set_looping(&musicSound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(&musicSound);
}

void Sound::stopMusic() {
    if (!musicLoaded) return;
    ma_sound_stop(&musicSound);
}

void Sound::setMusicVolume(float volume) {
    if (!musicLoaded) return;
    ma_sound_set_volume(&musicSound, volume);
}
