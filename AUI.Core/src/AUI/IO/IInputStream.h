#pragma once
#include "EOFException.h"

#include <AUI/Common/ByteBuffer.h>
#include <glm/glm.hpp>

class ByteBuffer;

class API_AUI_CORE IInputStream
{
public:
	virtual ~IInputStream() = default;

	/**
	 * \brief Прочитать из потока
	 * \return -1 - при ошибке
	 *          0 - при EOF
	 *         >0 - чилсо прочитанных байт из потока
	 */
	virtual int read(char* dst, int size) = 0;

	inline void read(const _<ByteBuffer>& dst)
	{
		const size_t BUFFER_SIZE = 0x10000;

		if (dst->getReserved() < BUFFER_SIZE) {
			dst->reserve(BUFFER_SIZE);
		}
		int r = read(dst->getCurrentPosAddress(), BUFFER_SIZE);

		if (r < 0)
			throw IOException();

		if (r == 0)
			throw EOFException();

		dst->setSize(dst->getCurrentPos() + r);
	}


	template<typename T>
	IInputStream& operator>>(T& out)
	{
		auto dst = reinterpret_cast<char*>(&out);

		int accumulator = sizeof(T);
		
		
		for (int r = 0; accumulator; dst += r, accumulator -= r) {
			r = read(dst, accumulator);
			if (r < 0)
				throw IOException();
			if (r == 0)
				throw EOFException();
		}
		return *this;
	}


	inline _<ByteBuffer> readSizedBuffer() {
		auto buf = _new<ByteBuffer>();
		uint32_t length;
		*this >> length;
		buf->reserve(length);
		buf->setSize(length);
		int r = read(buf->data(), length);
		assert(r == length);  // NOLINT(clang-diagnostic-sign-compare)

		return buf;
	}
};