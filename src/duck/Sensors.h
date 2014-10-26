
#ifndef H_DUCK_SENSORS_H
#define H_DUCK_SENSORS_H

#include "Sensor.h"

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

} // duck

#endif // H_DUCK_SENSORS_H
