#ifndef S2M_MUSCLE_FATIGUE_DYNAMIC_STATE_H
#define S2M_MUSCLE_FATIGUE_DYNAMIC_STATE_H
#include "biorbdConfig.h"
#include "s2mMuscleFatigueState.h"
#include "s2mMuscleCaracteristics.h"
#include "s2mMuscleFatigueParam.h"
#include "s2mMuscleStateActual.h"
#include "s2mVector.h"
#include "s2mMuscle.h"

class BIORBD_API s2mMuscleFatigueDynamicState : public s2mMuscleFatigueState
{
    public:
        s2mMuscleFatigueDynamicState(
                double active = 0,
                double fatigued = 0,
                double resting = 1);

        s2mMuscleFatigueDynamicState(
                const std::shared_ptr<s2mMuscle> m
                );

        double previousActiveFibers() const;
        double previousFatiguedFibers() const;
        double previousRestingFibers() const;

        virtual void timeDerivativeState(
                const s2mMuscleStateActual &EMG,
                const s2mMuscleCaracteristics &caract
         ) = 0;

    protected:
        double m_previousActiveFibers;
        double m_previousFatiguedFibers;
        double m_previousRestingFibers;
        double m_activeFibersDot;
        double m_fatiguedFibersDot;
        double m_restingFibersDot;

        virtual void setType();
};

#endif // S2M_MUSCLE_FATIGUE_DYNAMIC_STATE_H
