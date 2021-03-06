
#ifndef H_DUCK_GAME_OBJECT_H
#define H_DUCK_GAME_OBJECT_H

#include "Physics.h"

#include "rob/application/GameTime.h"
#include "rob/graphics/GraphicsTypes.h"
#include "rob/renderer/Color.h"
#include "rob/Assert.h"

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
        ~GameObject();

        void SetPosition(const vec2f &pos);
        vec2f GetPosition() const;

        vec2f GetDimensions() const;

        void SetBody(b2Body *body) { m_body = body; }
        b2Body* GetBody() { return m_body; }

        void SetLogic(Logic *logic);
        Logic* GetLogic();

        void SetColor(const Color &color);
        Color GetColor() const;

        void SetTexture(rob::TextureHandle texture);
        rob::TextureHandle GetTexture() const;

        void SetFlameTexture(rob::TextureHandle texture)
        { m_flameTexture = texture; }
        void SetFlameGlowTexture(rob::TextureHandle texture)
        { m_flameGlowTexture = texture; }

        void SetTextureScale(float scale)
        { m_textureScale = scale; }

        void SetLayer(int layer);
        int GetLayer() const;

        void SetDestroyed(bool destroyed)
        { m_destroyed = destroyed; }
        bool IsDestroyed() const
        { return m_destroyed; }

        void SetSaved(bool saved)
        { m_saved = saved; }
        bool IsSaved() const
        { return m_saved; }

        void SetBurned()
        { m_burned = true; m_burnTimer = 3.5f; m_color = Color(0.0f, 0.0f, 0.0f, 1.0f); }
        bool IsBurned() const
        { return m_burned; }

        void SetOily()
        { m_oilyness = 1.0f; }
        void SetOilyness(float oilyness)
        { m_oilyness = oilyness; }
        float GetOilyness() const
        { return m_oilyness; }

        void SetInWater(bool inWater)
        {
            if (inWater) m_partsInWater++;
            else m_partsInWater--;
            ROB_ASSERT(m_partsInWater >= 0);
        }
        bool IsInWater() const
        { return m_partsInWater != 0; }

        void Wash()
        { m_isWashed = true; }

        bool IsWashed() const
        { return m_isWashed; }

        void Update(const GameTime &gameTime);
        void Render(rob::Renderer *renderer);

        void SetNext(GameObject *object);
        GameObject *GetNext();

    private:
        b2Body *m_body;
        Logic *m_logic;

        Color m_color;
        rob::TextureHandle m_texture;
        rob::TextureHandle m_flameTexture;
        rob::TextureHandle m_flameGlowTexture;
        float m_textureScale;
        int m_renderLayer;

        float m_burnTimer;
        float m_oilyness;
        int m_partsInWater;

        bool m_destroyed;
        bool m_saved;
        bool m_burned;
        bool m_isWashed;

        GameObject *m_next;
    };

} // duck

#endif // H_DUCK_GAME_OBJECT_H

