#ifndef __HPP_SOUND__
#define __HPP_SOUND__

#include "miniaudio.h"
#include <string>
#include <unordered_map>

enum class SoundID {
    PEST_SPAWN,
    PEST_DAMAGE,
    CURSOR_MOVE,
    PLAYER_INPUT,
    BUY_ITEM,
    WATER_PLANT,
    HARVEST,
    QUOTA_COMPLETE
};

class Sound {
public:
    static Sound& instance() {
        static Sound inst;
        return inst;
    }

    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;

    bool init();
    void shutdown();

    bool loadSound(SoundID id, const std::string& filepath);

    void play(SoundID id, float volume = 1.0f);

    bool loadMusic(const std::string& filepath);
    void playMusic(bool loop = true);
    void stopMusic();
    void setMusicVolume(float volume); // 0.0 – 1.0

private:
    Sound() = default;
    ~Sound() { shutdown(); }

    ma_engine        engine{};
    bool             engineReady = false;

    std::unordered_map<int, std::string>  soundPaths;

    ma_sound musicSound{};
    bool     musicLoaded = false;
};

#endif // __HPP_SOUND__
