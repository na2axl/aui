//
// Created by dervisdev on 1/17/2023.
//

#pragma once

#include <AUI/Platform/ACursor.h>
#include <AUI/Util/AMetric.h>
#include "IDeclaration.h"

namespace ass {
    namespace decl {
        template<>
        struct API_AUI_VIEWS Declaration<ATextOverflow>: IDeclarationBase {
        private:
            ATextOverflow mInfo;

        public:
            Declaration(const ATextOverflow& info) : mInfo(info) {

            }

            void applyFor(AView* view) override;

            [[nodiscard]]
            const auto& value() const noexcept {
                return mInfo;
            }
        };
    }
}
