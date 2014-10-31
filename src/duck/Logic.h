
#ifndef H_DUCK_LOGIC_H
#define H_DUCK_LOGIC_H

#include "PidController.h"
#include "rob/Log.h"

namespace duck
{

    class GameObject;

    class Logic
    {
    public:
        Logic() : m_owner(nullptr) { }
        virtual ~Logic() { }

        virtual void Update(float deltaTime) = 0;

        void SetOwner(GameObject *owner) { m_owner = owner; }
    protected:
        GameObject *m_owner;
    };

    class BirdLogic : public Logic
    {
    public:
        BirdLogic(b2Body *body, b2RevoluteJoint *joint, b2RevoluteJoint *joint0, b2RevoluteJoint *joint1, b2RevoluteJoint *joint2)
            : m_neck0Controller()
            , m_neck1Controller()
            , m_neck2Controller()
            , m_targetAngle(0.0f) //30.0f * rob::DEG2RAD) //-90.0f * rob::DEG2RAD)
            , m_headBody(body)
            , m_neckJoint(joint)
            , m_neck0Joint(joint0)
            , m_neck1Joint(joint1)
            , m_neck2Joint(joint2)
        {
            m_neck0Controller.SetGains(0.5f, 0.5f, 0.5f);
        }

        void UpdateHead(float deltaTime)
        {
            {
                b2Body *body = m_neck0Joint->GetBodyB();
                float angle = body->GetAngle();
                angle = fmodf(angle, 2.0f * rob::PI_f);
                body->SetTransform(body->GetPosition(), angle);
                float err = m_targetAngle - angle;
                while (err > rob::PI_f) err = (rob::PI_f - err);
                while (err < -rob::PI_f) err = -(err + rob::PI_f);
                m_neck0Controller.SetError(err);
                m_neck0Controller.Step(deltaTime);
                const float targetSpeed = m_neck0Controller.GetOutput();
                m_neck0Joint->SetMotorSpeed(targetSpeed);
            }
            {
                b2Body *body = m_neck1Joint->GetBodyB();
                float angle = body->GetAngle();
                angle = fmodf(angle, 2.0f * rob::PI_f);
                body->SetTransform(body->GetPosition(), angle);
                float err = m_targetAngle - angle;
                while (err > rob::PI_f) err = (rob::PI_f - err);
                while (err < -rob::PI_f) err = -(err + rob::PI_f);
                m_neck1Controller.SetError(err);
                m_neck1Controller.Step(deltaTime);
                const float targetSpeed = m_neck1Controller.GetOutput();
                m_neck1Joint->SetMotorSpeed(targetSpeed);
            }
            {
                b2Body *body = m_neck2Joint->GetBodyB();
                float angle = body->GetAngle();
                angle = fmodf(angle, 2.0f * rob::PI_f);
                body->SetTransform(body->GetPosition(), angle);
                float err = m_targetAngle - angle;
                while (err > rob::PI_f) err = (rob::PI_f - err);
                while (err < -rob::PI_f) err = -(err + rob::PI_f);
                m_neck2Controller.SetError(err);
                m_neck2Controller.Step(deltaTime);
                const float targetSpeed = m_neck2Controller.GetOutput();
                m_neck2Joint->SetMotorSpeed(targetSpeed);
            }
        }

        void DisableHead()
        {
            m_neckJoint->EnableMotor(false);
            m_neck0Joint->EnableMotor(false);
            m_neck1Joint->EnableMotor(false);
            m_neck2Joint->EnableMotor(false);
        }

        void Update(float deltaTime) override
        {
            if (!m_owner->IsBurned())
                UpdateHead(deltaTime);
            else
                DisableHead();

            if (!m_owner->IsBurned())
            {
                float oilyness = m_owner->GetOilyness();
                if (oilyness > 0.0f && m_owner->IsInWater())
                {
                    float speed = 0.1f;
                    oilyness -= speed * deltaTime;
                    if (oilyness < 0.0f) oilyness = 0.0f;

                    GameObject *obj = m_owner;
                    do
                    {
                        obj->SetOilyness(oilyness);
                        obj = obj->GetNext();
                    } while (obj != m_owner);
                }

                float c = 0.1f * oilyness + 1.0f * (1.0 - oilyness);

                GameObject *obj = m_owner;
                do
                {
                    obj->SetColor(Color(c, c, c));
                    obj = obj->GetNext();
                } while (obj != m_owner);
            }
        }
    private:
        PidController m_neck0Controller;
        PidController m_neck1Controller;
        PidController m_neck2Controller;
        float m_targetAngle;
        b2Body *m_headBody;
        b2RevoluteJoint *m_neckJoint;
        b2RevoluteJoint *m_neck0Joint;
        b2RevoluteJoint *m_neck1Joint;
        b2RevoluteJoint *m_neck2Joint;
    };

} // duck

#endif // H_DUCK_LOGIC_H

