
#ifndef H_DUCK_SENSOR_H
#define H_DUCK_SENSOR_H

#include "Physics.h"

#define BIT(x) (1 << x)

namespace duck
{

    class DuckState;

    enum
    {
        BirdBits = BIT(5),
        SensorBits = BIT(6),
        StaticBits = BIT(7),
        WheelBits = BIT(8)
    };

    class Sensor
    {
    public:
        Sensor(uint16 maskBits)
            : m_maskBits(maskBits)
        { }

        virtual ~Sensor()
        { }

        void SetDuckState(DuckState *duckState)
        { m_duckState = duckState; }

        void SetBody(b2Body *body)
        { m_body = body; }
        b2Body* GetBody(b2Body *body) const
        { return m_body; }

        void SetShape(const b2Shape *shape)
        {
            b2FixtureDef fixDef;
            fixDef.shape = shape;
            fixDef.userData = this;
            fixDef.isSensor = true;
            fixDef.filter.categoryBits = SensorBits;
            fixDef.filter.maskBits = m_maskBits;
            m_body->CreateFixture(&fixDef);
        }

        virtual void BeginContact(void *userData) { }
        virtual void EndContact(void *userData) { }

    protected:
        DuckState *m_duckState;

    private:
        uint16 m_maskBits;
        b2Body *m_body;
    };

    class SensorListener : public b2ContactListener
    {
    public:
        void BeginContact(b2Contact* contact) override
        {
            Sensor *sensor = nullptr;
            void *userData = GetUserDataAndSensor(&sensor, contact);
            if (sensor)
                sensor->BeginContact(userData);
        }

        void EndContact(b2Contact *contact) override
        {
            Sensor *sensor = nullptr;
            void *userData = GetUserDataAndSensor(&sensor, contact);
            if (sensor)
                sensor->EndContact(userData);
        }

    private:
        void* GetUserDataAndSensor(Sensor **sensor, b2Contact *contact)
        {
            if (( *sensor = GetSensor(contact->GetFixtureA()) ))
                return contact->GetFixtureB()->GetUserData();
            if (( *sensor = GetSensor(contact->GetFixtureB()) ))
                return contact->GetFixtureA()->GetUserData();
            return nullptr;
        }

        Sensor *GetSensor(const b2Fixture *fixture)
        {
            if (fixture->GetFilterData().categoryBits == SensorBits)
                return (Sensor*)fixture->GetUserData();
            return nullptr;
        }
    };

} // duck

#endif // H_DUCK_SENSOR_H
