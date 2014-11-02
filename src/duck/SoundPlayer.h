
#ifndef H_DUCK_SOUND_PLAYER_H
#define H_DUCK_SOUND_PLAYER_H

#include "rob/audio/AudioSystem.h"
#include "rob/resource/MasterCache.h"

namespace duck
{

    using namespace rob;

    constexpr float PositionScale = 0.125f;

    class SoundPlayer
    {
    public:
        SoundPlayer()
            : m_audio(nullptr)
//            , m_cache(nullptr)
            , m_currentTime(0)
            , m_burning(InvalidSound)
            , m_water(InvalidSound)
        { }

        void Init(AudioSystem &audio, MasterCache &cache)
        {
            m_audio = &audio;
            m_burning = cache.GetSound("Explosion32.wav");
            m_water = cache.GetSound("Randomize55.wav");
        }

        void UpdateTime(const GameTime &gameTime)
        {
            m_currentTime = gameTime.GetTotalMicroseconds();
        }

        void PlayBurningSound(const vec2f &pos)
        {
            PlaySound(m_burning, pos.x, pos.y);
        }

        void PlayWaterSound(const vec2f &pos)
        {
            PlaySound(m_water, pos.x, pos.y);
        }

    private:
        void PlaySound(SoundHandle sound, float x, float y)
        {
            x *= PositionScale; y *= PositionScale;
            m_audio->PlaySound(sound, 0.5f, x, y, m_currentTime);
        }

    private:
        AudioSystem *m_audio;
//        MasterCache *m_cache;
        Time_t m_currentTime;

        SoundHandle m_burning;
        SoundHandle m_water;
    };

} // duck

#endif // H_DUCK_SOUND_PLAYER_H
