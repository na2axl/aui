﻿#pragma once

#include <AUI/Url/AUrl.h>
#include <AUI/Logging/ALogger.h>
#include "IImageLoader.h"
#include "AUI/Common/ADeque.h"
#include "AUI/Common/SharedPtr.h"
#include "AUI/Util/Singleton.h"

class API_AUI_CORE AImageLoaderRegistry: public Singleton<AImageLoaderRegistry>
{
private:
	ADeque<_<IImageLoader>> mImageLoaders;
	
public:
	AImageLoaderRegistry()
	{
	}

	void registerImageLoader(_<IImageLoader> imageLoader);

	_<IDrawable> loadDrawable(_<AByteBuffer> buffer);
	_<AImage> loadImage(_<AByteBuffer> buffer);
	inline _<IDrawable> loadDrawable(const AUrl& url) {
	    return loadDrawable(AByteBuffer::fromStream(url.open()));
	}
	_<AImage> loadImage(const AUrl& url);
};
