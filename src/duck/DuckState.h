
#ifndef H_DUCK_DUCK_STATE_H
#define H_DUCK_DUCK_STATE_H

#include "rob/application/GameState.h"
#include "rob/application/GameTime.h"
#include "rob/renderer/Renderer.h"
#include "rob/memory/Pool.h"

#include "GameData.h"
#include "GameObject.h"
#include "Sensors.h"
#include "FadeEffect.h"
#include "SoundPlayer.h"

namespace duck
{

    using rob::Time_t;
    using rob::GameTime;

    class DebugDraw;

    class DuckState : public rob::GameState
    {
    public:
        DuckState(GameData &gameData);
        ~DuckState();

        bool Initialize() override;
        void CreateWorld();

        GameObject* CreateObject(GameObject *prevLink = nullptr);
        GameObject* CreateStaticBox(const vec2f &position, float angle, float w, float h);
        GameObject* CreateWheel(const vec2f &position);
        GameObject* CreateWaterContainer(const vec2f &position, float w, float h);
        GameObject* CreateBird(const vec2f &position);
        void CreateOven();
        void CreateSpawnArea(const vec2f &position);
        void CreateSlide();

        void DestroyObject(GameObject *object);
        void DestroyLinkedObjects(GameObject *object);

        void DestroyObjectList(GameObject *object, GameObject *last);
        void DestroySingleObject(GameObject *object);
        void DestroyAllObjects();

        void BirdBurned(GameObject *bird);
        void BirdSaved(GameObject *bird);
        void BirdDied(GameObject *bird);

        bool IsGameOver() const;

        void RecalcProj();
        void OnResize(int w, int h) override;

        void RealtimeUpdate(const Time_t deltaMicroseconds) override;

        void NewBird();

        void DestroyMouseJoint();

        void Update(const GameTime &gameTime) override;
        void RenderGameOver();
        void Render() override;

        void OnKeyPress(rob::Keyboard::Key key, rob::Keyboard::Scancode scancode, rob::uint32_t mods) override;
        void OnMouseDown(rob::MouseButton button, int x, int y) override;
        void OnMouseUp(rob::MouseButton button, int x, int y) override;
        void OnMouseMove(int x, int y) override;

    private:
        GameData &m_gameData;
        rob::View m_view;
        b2World *m_world;
        DebugDraw *m_debugDraw;
        bool m_drawBox2D;

        bool m_inUpdate;

        b2Body *m_worldBody;

        b2MouseJoint *m_mouseJoint;
        float m_originalAngle;
        vec2f m_mouseWorld;

        rob::Pool<GameObject> m_objectPool;
        GameObject **m_objects;
        size_t m_objectCount;

        SensorListener m_sensorListener;

        OvenSensor m_ovenSensor;
        SpawnSensor m_spawnSensor;
        SlideSensor m_slideSensor;
        KillSensor m_killSensor;
        WaterSensor m_waterSensor;

        FadeEffect m_fadeEffect;

        float m_scoreTimer;
        int m_lastPoints;

        SoundPlayer m_sounds;
    };

} // duck

#endif // H_DUCK_DUCK_STATE_H

