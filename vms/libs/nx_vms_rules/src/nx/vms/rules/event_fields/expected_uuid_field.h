// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include "../simple_type_field.h"

namespace nx::vms::rules {

class NX_VMS_RULES_API ExpectedUuidField: public SimpleTypeEventField<QnUuid>
{
    Q_OBJECT
    Q_CLASSINFO("metatype", "nx.events.fields.expectedUuid")

    Q_PROPERTY(QnUuid value READ value WRITE setValue)

public:
    ExpectedUuidField() = default;
};

} // namespace nx::vms::rules
