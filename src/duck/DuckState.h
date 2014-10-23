
#ifndef H_DUCK_DUCK_STATE_H
#define H_DUCK_DUCK_STATE_H

#include "rob/application/GameState.h"
#include "rob/application/GameTime.h"
#include "rob/renderer/Renderer.h"
#include "rob/memory/Pool.h"

#include "GameObject.h"

#include "rob/Log.h"
#define BIT(x) (1 << x)

namespace duck
{

    using rob::Time_t;
    using rob::GameTime;

    class DebugDraw;

    enum
    {
        DuckBits = BIT(5),
        SensorBits = BIT(6)
    };

    class Sensor
    {
    public:
        Sensor(uint16 maskBits)
            : m_maskBits(maskBits)
        { }

        virtual ~Sensor()
        { }

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

        virtual void BeginContact(const b2Body *body) { }
//        virtual void EndContact(const b2Body *body) { }

    private:
        uint16 m_maskBits;
        b2Body *m_body;
    };

    class SensorListener : public b2ContactListener
    {
    public:
        void BeginContact(b2Contact* contact)
        {
            const b2Fixture *fixA = contact->GetFixtureA();
            const b2Fixture *fixB = contact->GetFixtureA();
            if (fixA->GetFilterData().categoryBits == SensorBits)
            {
                Sensor *sensor = (Sensor*)fixA->GetUserData();
                sensor->BeginContact(fixB->GetBody());
            }
            else if (fixB->GetFilterData().categoryBits == SensorBits)
            {
                Sensor *sensor = (Sensor*)fixB->GetUserData();
                sensor->BeginContact(fixA->GetBody());
            }
        }
    };

    class DuckSensor : public Sensor
    {
    public:
        DuckSensor()
            : Sensor(DuckBits)
        { }

        virtual void BeginContact(const b2Body *body) override
        {
            rob::log::Debug("Duck sensor: begin contact!");
        }
    };

    class DuckState : public rob::GameState
    {
    public:
        DuckState();
        ~DuckState();

        bool Initialize() override;
        void CreateWorld();

        GameObject* CreateObject();
        GameObject* CreateStaticBox(const vec2f &position, float angle, float w, float h);
        GameObject* CreateWheel(const vec2f &position);
        GameObject* CreateWaterContainer(const vec2f &position, float w, float h);
        GameObject* CreateBird(const vec2f &position);

        void DestroyObject(GameObject *object);
        void DestroyAllObjects();

        void OnResize(int w, int h) override;

        void RealtimeUpdate(const Time_t deltaMicroseconds) override;
        void Update(const GameTime &gameTime) override;
        void Render() override;

        void OnKeyPress(rob::Keyboard::Key key, rob::Keyboard::Scancode scancode, rob::uint32_t mods) override;
        void OnMouseDown(rob::MouseButton button, int x, int y) override;
        void OnMouseUp(rob::MouseButton button, int x, int y) override;
        void OnMouseMove(int x, int y) override;

    private:
        rob::View m_view;
        b2World *m_world;
        DebugDraw *m_debugDraw;
        bool m_drawBox2D;

        b2Body *m_worldBody;

        b2MouseJoint *m_mouseJoint;
        vec2f m_mouseWorld;

        rob::Pool<GameObject> m_objectPool;
        GameObject **m_objects;
        size_t m_objectCount;

        SensorListener m_sensorListener;
        DuckSensor m_duckSensor;
    };

} // duck

#endif // H_DUCK_DUCK_STATE_H

