
#ifndef H_DUCK_SENSORS_H
#define H_DUCK_SENSORS_H

#include "Sensor.h"
#include "GameObject.h"

#include "rob/Assert.h"

namespace duck
{

    class OvenSensor : public Sensor
    {
    public:
        OvenSensor()
            : Sensor(BirdBits)
        { }

        void BeginContact(void *userData) override;
    };

    class SpawnSensor : public Sensor
    {
    public:
        SpawnSensor()
            : Sensor(BirdBits)
            , m_count(0)
        { }

        bool CanSpawn() const { return m_count == 0; }

        void BeginContact(void *userData) override
        {
            m_count++;
        }
        void EndContact(void *userData) override
        {
            ROB_ASSERT(m_count > 0);
            m_count--;
        }
    private:
        int m_count;
    };

    class SlideSensor : public Sensor
    {
    public:
        SlideSensor()
            : Sensor(BirdBits)
        { }

        void BeginContact(void *userData) override
        {
            GameObject *bird = (GameObject*)userData;
            ROB_ASSERT(bird != nullptr);
            if (!bird->IsBurned())
            {
                bird->SetSaved(true);
            }
            else
            {
                bird->SetAlive(false);
            }
        }
    };

    class KillSensor : public Sensor
    {
    public:
        KillSensor();
        void BeginContact(void *userData) override;
    };

} // duck

#endif // H_DUCK_SENSORS_H
