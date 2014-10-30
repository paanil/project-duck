
#ifndef H_DUCK_GAME_OBJECT_H
#define H_DUCK_GAME_OBJECT_H

#include "Physics.h"

#include "rob/application/GameTime.h"
#include "rob/graphics/GraphicsTypes.h"
#include "rob/renderer/Color.h"

namespace rob
{
    class Renderer;
} // rob

namespace duck
{

    using rob::vec2f;
    using rob::GameTime;
    using rob::Color;

    class Logic;

    class GameObject
    {
    public:
        GameObject();

        void SetPosition(const vec2f &pos);
        vec2f GetPosition() const;

        vec2f GetDimensions() const;

        void SetBody(b2Body *body) { m_body = body; }
        b2Body* GetBody() { return m_body; }

        void SetLogic(Logic *logic) { m_logic = logic; }
        Logic* GetLogic() { return m_logic; }

        void SetColor(const Color &color);
        Color GetColor() const;

        void SetTexture(rob::TextureHandle texture);
        rob::TextureHandle GetTexture() const;

        void SetLayer(int layer);
        int GetLayer() const;

        void SetAlive(bool alive)
        { m_alive = alive; }
        bool IsAlive() const
        { return m_alive; }

        void SetSaved(bool saved)
        { m_saved = saved; }
        bool IsSaved() const
        { return m_saved; }

        void SetBurned()
        { m_burned = true; }
        bool IsBurned() const
        { return m_burned; }

        void Update(const GameTime &gameTime);
        void Render(rob::Renderer *renderer);

        void SetNext(GameObject *object);
        GameObject *GetNext();

    private:
        b2Body *m_body;
        Logic *m_logic;

        Color m_color;
        rob::TextureHandle m_texture;
        int m_renderLayer;

        bool m_alive;
        bool m_saved;
        bool m_burned;

        GameObject *m_next;
    };

} // duck

#endif // H_DUCK_GAME_OBJECT_H

