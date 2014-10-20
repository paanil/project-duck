
#ifndef H_DUCK_GAME_OBJECT_H
#define H_DUCK_GAME_OBJECT_H

#include "Physics.h"

#include "rob/application/GameTime.h"
#include "rob/graphics/GraphicsTypes.h"

namespace rob
{
    class Renderer;
} // rob

namespace duck
{

    using rob::vec2f;
    using rob::GameTime;

    class GameObject
    {
    public:
        GameObject();

        void SetPosition(const vec2f &pos);
        vec2f GetPosition() const;

        vec2f GetDimensions() const;

        void SetBody(b2Body *body) { m_body = body; }
        b2Body* GetBody() { return m_body; }

        void SetTexture(rob::TextureHandle texture);
        rob::TextureHandle GetTexture() const;

        void SetLayer(int layer);
        int GetLayer() const;

        void Update(const GameTime &gameTime);
        void Render(rob::Renderer *renderer);

    private:
        b2Body *m_body;
        rob::TextureHandle m_texture;
        int m_renderLayer;
    };

} // duck

#endif // H_DUCK_GAME_OBJECT_H

