#include "ReverbEffect.h"

using namespace agbplay;

/*
 * public ReverbEffect
 */

ReverbEffect::ReverbEffect(int intensity, uint32_t streamRate, uint32_t agbRate, uint8_t numAgbBuffers)
{
    this->intensity = intensity;
    if (intensity <= 0) {
        rtype = RevType::NONE;
    } else if (intensity <= 0x7F) {
        rtype = RevType::NORMAL;
    } else {
        rtype = RevType::GS;
    }

}
