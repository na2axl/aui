//
// Created by dervisdev on 2/9/2023.
//

#pragma once

#include "ISoundStream.h"
#include "vorbis/vorbisfile.h"
#include "AUI/IO/AFileInputStream.h"

class AUrl;
class ISeekableInputStream;


/**
 * @brief Sound stream for OGG format
 * @ingroup audio
 */
class AOggSoundStream: public ISoundStream {
public:
    explicit AOggSoundStream(_<ISeekableInputStream> fis);

    ~AOggSoundStream() override;

    AAudioFormat info() override;

    size_t read(char* dst, size_t size) override;

    void rewind() override;

    static _<AOggSoundStream> load(_<ISeekableInputStream> is);
    static _<AOggSoundStream> fromUrl(const AUrl& url);

private:
    _<ISeekableInputStream> mStream;
    OggVorbis_File mVorbisFile;
};
