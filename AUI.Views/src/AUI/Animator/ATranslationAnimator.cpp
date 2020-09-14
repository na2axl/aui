//
// Created by alex2 on 27.08.2020.
//

#include <AUI/Util/Singleton.h>
#include "ATranslationAnimator.h"
#include <AUI/Render/Render.h>

ATranslationAnimator::ATranslationAnimator(const glm::vec2& from, const glm::vec2& to) : mFrom(from), mTo(to) {}

void ATranslationAnimator::doAnimation(float theta) {
    Render::instance().setTransform(glm::translate(glm::mat4(1.f),
                                                   glm::vec3(glm::mix(mFrom, mTo, theta), 0.f)));
}
