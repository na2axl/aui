#pragma once

#include "CommonRenderingContext.h"
#include <AUI/Platform/AWindow.h>

class API_AUI_VIEWS SoftwareRenderingContext: public CommonRenderingContext {
private:
#if AUI_PLATFORM_WIN
    AByteBuffer mBitmapBlob;
    BITMAPINFO* mBitmapInfo;
#else
    AByteBuffer mBitmapBlob;
#endif

protected:
    AByteBuffer mStencilBlob;
    glm::uvec2 mBitmapSize;

public:
    SoftwareRenderingContext();
    ~SoftwareRenderingContext() override;

    void destroyNativeWindow(ABaseWindow& window) override;
    void beginPaint(ABaseWindow& window) override;
    void endPaint(ABaseWindow& window) override;
    void beginResize(ABaseWindow& window) override;
    void init(const Init& init) override;

    AImage makeScreenshot() override;

    inline uint8_t& stencil(const glm::uvec2& position) {
        return mStencilBlob.at<uint8_t>(mBitmapSize.x * position.y + position.x);
    }

    [[nodiscard]]
    glm::uvec2 bitmapSize() const {
        return mBitmapSize;
    }

#if AUI_PLATFORM_WIN
    inline void putPixel(const glm::uvec2& position, const glm::u8vec3& color) noexcept {
        assert(("image out of bounds" && glm::all(glm::lessThan(position, mBitmapSize))));

        auto dataPtr = reinterpret_cast<uint8_t*>(mBitmapBlob.data() + sizeof(BITMAPINFO)
            + (mBitmapSize.x * position.y + position.x) * 4);
        dataPtr[0] = color[2];
        dataPtr[1] = color[1];
        dataPtr[2] = color[0];
        dataPtr[3] = 255;
    }
    inline glm::u8vec3 getPixel(const glm::uvec2& position) noexcept {
        assert(("image out of bounds" && glm::all(glm::lessThan(position, mBitmapSize))));
        auto dataPtr = reinterpret_cast<uint8_t*>(mBitmapBlob.data() + sizeof(BITMAPINFO)
                                                  + (mBitmapSize.x * position.y + position.x) * 4);

        return { dataPtr[2], dataPtr[1], dataPtr[0] };
    }
#else
    inline void putPixel(const glm::uvec2& position, const glm::u8vec3& color) noexcept {
        assert(("image out of bounds" && glm::all(glm::lessThan(position, mBitmapSize))));

        auto dataPtr = reinterpret_cast<uint8_t*>(mBitmapBlob.data() + (mBitmapSize.x * position.y + position.x) * 4);
        dataPtr[0] = color[0];
        dataPtr[1] = color[1];
        dataPtr[2] = color[2];
        dataPtr[3] = 255;
    }
    inline glm::u8vec3 getPixel(const glm::uvec2& position) noexcept {
        assert(("image out of bounds" && glm::all(glm::lessThan(position, mBitmapSize))));

        auto dataPtr = reinterpret_cast<uint8_t*>(mBitmapBlob.data() + (mBitmapSize.x * position.y + position.x) * 4);
        return {
            dataPtr[0],
            dataPtr[1],
            dataPtr[2]
        };
    }
#endif

    void endResize(ABaseWindow& window) override;
};