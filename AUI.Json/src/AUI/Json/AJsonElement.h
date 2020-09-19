#pragma once

#include <AUI/Json.h>
#include "AUI/Common/SharedPtrTypes.h"
#include "AUI/Common/AVariant.h"
#include "AUI/Common/AMap.h"
#include "AUI/Common/AVector.h"
#include <AUI/IO/IOutputStream.h>
#include <AUI/Thread/AThreadPool.h>

class IJsonElement;
class AJsonElement;

namespace AJson
{
    void API_AUI_JSON write(_<IOutputStream> os, const AJsonElement& json);
}


class API_AUI_JSON AJsonElement {
friend void AJson::write(_<IOutputStream> os, const AJsonElement& json);
private:
	_<IJsonElement> mJson;

protected:
    void serialize(_<IOutputStream> param) const;

public:
	explicit AJsonElement(const _<IJsonElement>& json_element);
	AJsonElement() {}


	[[nodiscard]] bool isVariant() const;
	[[nodiscard]] bool isObject() const;
	[[nodiscard]] bool isArray() const;

	[[nodiscard]] const AVariant& asVariant() const;
	[[nodiscard]] int asInt() const;
	[[nodiscard]] AString asString() const;
	[[nodiscard]] const AMap<AString, AJsonElement>& asObject() const;
	[[nodiscard]] const AVector<AJsonElement>& asArray() const;
	
	[[nodiscard]] const AJsonElement& operator[](size_t index) const;
	[[nodiscard]] const AJsonElement& operator[](const AString& key) const;
	[[nodiscard]] AJsonElement& operator[](const AString& key);
};

class API_AUI_JSON AJsonValue: public AJsonElement
{
public:
	AJsonValue(const AVariant& value);
};
class API_AUI_JSON AJsonObject: public AJsonElement
{
public:
	AJsonObject(const AMap<AString, AJsonElement>& value);
};
class API_AUI_JSON AJsonArray: public AJsonElement
{
public:
	AJsonArray(const AVector<AJsonElement>& value);
};