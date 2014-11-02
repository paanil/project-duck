
#include "Sensors.h"
#include "DuckState.h"

namespace duck
{

    void OvenSensor::BeginContact(void *userData)
    {
        GameObject *bird = (GameObject*)userData;

        if (bird->IsBurned())
            return;

        GameObject *part = bird;

        do
        {
            part->SetBurned();
            part = part->GetNext();
        } while (part != bird);

        m_duckState->BirdBurned(bird);
    }


    SlideSensor::SlideSensor()
        : Sensor(BirdBits)
    { }

    void SlideSensor::BeginContact(void *userData)
    {
        GameObject *bird = (GameObject*)userData;
        ROB_ASSERT(bird != nullptr);
        if (!bird->IsBurned())
            m_duckState->BirdSaved(bird);
//        else
//            m_duckState->BirdDied();
        m_duckState->DestroyLinkedObjects(bird);
    }


    KillSensor::KillSensor()
        : Sensor(0xFFFF)
    { }

    void KillSensor::BeginContact(void *userData)
    {
        GameObject *obj = (GameObject*)userData;
//        if (obj) obj->SetDestroyed(true);
        if (obj) m_duckState->DestroyLinkedObjects(obj);
    }

    WaterSensor::WaterSensor()
        : Sensor(BirdBits)
    { }

    void WaterSensor::BeginContact(void* userData)
    {
        GameObject *firstPart = (GameObject*)userData;
        GameObject *part = firstPart;

        if (!part->IsInWater())
            m_duckState->BirdEnteredWater(part);

        do
        {
            part->SetInWater(true);
            part = part->GetNext();
        } while (part != firstPart);
    }

    void WaterSensor::EndContact(void* userData)
    {
        GameObject *firstPart = (GameObject*)userData;
        GameObject *part = firstPart;
        do
        {
            part->SetInWater(false);
            part = part->GetNext();
        } while (part != firstPart);

        if (!part->IsInWater())
            m_duckState->BirdExitedWater(part);
    }

} // duck
