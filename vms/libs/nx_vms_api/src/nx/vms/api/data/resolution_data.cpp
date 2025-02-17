// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "resolution_data.h"
#include <nx/fusion/model_functions.h>

namespace nx::vms::api {

bool deserialize(QnJsonContext* /*ctx*/, const QJsonValue& value, ResolutionData* target)
{
    if (value.isString())
    {
        std::regex re("^([+-]?\\d+)([px])([+-]?\\d+)?$");
        std::smatch match;

        const auto str = value.toString().toStdString();
        if (!std::regex_search(str, match, re))
            return false;

        if (match.size() < 3)
            return false;

        try
        {
            if (match.size() < 4)
            {
                if (match[2] != "p")
                    return false;
                target->size.setHeight(std::stoi(match[1]));
                return true;
            }

            if (match[2] != "x")
                return false;
            target->size.setWidth(std::stoi(match[1]));
            target->size.setHeight(std::stoi(match[3]));
            return true;
        }
        catch (const std::exception& e)
        {
            NX_DEBUG(NX_SCOPE_TAG, "Failed to deserialize ResolutionData: %1", e.what());
            return false;
        }
    }
    return false;
}

void serialize(QnJsonContext* ctx, const ResolutionData& value, QJsonValue* target)
{
    if (value.size.isValid())
    {
        *target = QString::number(value.size.width()) + "x" + QString::number(value.size.height());
        return;
    }
    *target = QString::number(value.size.height()) + "p";
}

int ResolutionData::megaPixels() const
{
    return (size.width() * size.height() + 500'000) / 1'000'000;
}

} // namespace nx::vms::api
