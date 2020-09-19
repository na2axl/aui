#include <AUI/IO/IOutputStream.h>
#include "AJsonElement.h"
#include "JsonValue.h"
#include "JsonObject.h"
#include "JsonArray.h"

AJsonElement::AJsonElement(const _<IJsonElement>& json_element): mJson(json_element)
{
}


bool AJsonElement::isVariant() const
{
	return mJson->isVariant();
}

bool AJsonElement::isObject() const
{
	return mJson->isObject();
}

bool AJsonElement::isArray() const
{
	return mJson->isArray();
}

const AVariant& AJsonElement::asVariant() const
{
	return mJson->asVariant();
}

int AJsonElement::asInt() const
{
	return mJson->asVariant().toInt();
}

AString AJsonElement::asString() const
{
	return mJson->asVariant().toString();
}

const AMap<AString, AJsonElement>& AJsonElement::asObject() const
{
	return mJson->asObject();
}

const AVector<AJsonElement>& AJsonElement::asArray() const
{
	return mJson->asArray();
}

const AJsonElement& AJsonElement::operator[](size_t index) const
{
	return mJson->asArray()[index];
}

const AJsonElement& AJsonElement::operator[](const AString& key) const
{
	return mJson->asObject().at(key);
}

void AJsonElement::serialize(_<IOutputStream> param) const {
    mJson->serialize(param);
}

AJsonElement& AJsonElement::operator[](const AString& key) {
    return mJson->asObject()[key];
}

AJsonValue::AJsonValue(const AVariant& value):
	AJsonElement(_new<JsonValue>(value))
{
}

AJsonObject::AJsonObject(const AMap<AString, AJsonElement>& value):
	AJsonElement(_new<JsonObject>(value))
{
}

AJsonObject::AJsonObject():
    AJsonElement(_new<JsonObject>())
{

}

AJsonArray::AJsonArray(const AVector<AJsonElement>& value) :
	AJsonElement(_new<JsonArray>(value))
{
}


