// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "field.h"

#include <nx/utils/qt_helpers.h>
#include <nx/vms/api/data/user_group_data.h>

#include "../action_builder_fields/extract_detail_field.h"
#include "../action_builder_fields/flag_field.h"
#include "../action_builder_fields/optional_time_field.h"
#include "../action_builder_fields/target_user_field.h"
#include "../action_builder_fields/text_with_fields.h"
#include "../aggregated_event.h"
#include "../basic_action.h"
#include "../event_filter_fields/state_field.h"

using namespace std::chrono_literals;

namespace nx::vms::rules::utils {

FieldDescriptor makeIntervalFieldDescriptor(
    const QString& displayName,
    const QString& description)
{
    return makeTimeFieldDescriptor<OptionalTimeField>(
        kIntervalFieldName,
        displayName,
        description,
        {.initialValue = 1min, .defaultValue = 1min, .minimumValue = 1s});
}

FieldDescriptor makePlaybackFieldDescriptor(
    const QString& displayName,
    const QString& description)
{
    return makeTimeFieldDescriptor<OptionalTimeField>(
        kPlaybackTimeFieldName,
        displayName,
        description,
        {.initialValue = 0s, .defaultValue = 1s, .maximumValue = 300s, .minimumValue = 0s});
}

FieldDescriptor makeStateFieldDescriptor(
    const QString& displayName,
    const QString& description,
    vms::rules::State defaultState)
{
    return makeFieldDescriptor<StateField>(
        kStateFieldName,
        displayName,
        description,
        {{"value", QVariant::fromValue(defaultState)}});
}

FieldDescriptor makeExtractDetailFieldDescriptor(
    const QString& fieldName,
    const QString& detailName)
{
    return makeFieldDescriptor<ExtractDetailField>(
        fieldName,
        fieldName,
        {},
        {{ "detailName", detailName }});
}

FieldDescriptor makeTextFormatterFieldDescriptor(
    const QString& fieldName,
    const QString& formatString)
{
    return makeFieldDescriptor<TextFormatter>(
        fieldName,
        fieldName,
        {},
        {{ "text", formatString }});
}

FieldDescriptor makeTargetUserFieldDescriptor(
    const QString& displayName,
    const QString& description,
    UserFieldPreset preset,
    bool visible,
    const QStringList& linkedFields)
{
    QVariantMap props;
    if (preset == UserFieldPreset::All)
        props["acceptAll"] = true;
    if (preset == UserFieldPreset::Power)
        props["ids"] = QVariant::fromValue(nx::utils::toQSet(vms::api::kAllPowerUserGroupIds));

    if (!visible)
        props["visible"] = false;

    return makeFieldDescriptor<TargetUserField>(
        utils::kUsersFieldName,
        displayName,
        description,
        props,
        linkedFields);
}

FieldDescriptor makeActionFlagFieldDescriptor(
    const QString& fieldName,
    const QString& displayName,
    const QString& description,
    bool defaultValue)
{
    return makeFieldDescriptor<ActionFlagField>(
        fieldName, displayName, description, {{"value", defaultValue}});
}

QnUuidList getDeviceIds(const AggregatedEventPtr& event)
{
    QnUuidList result;
    result << getFieldValue<QnUuid>(event, utils::kCameraIdFieldName);
    result << getFieldValue<QnUuidList>(event, utils::kDeviceIdsFieldName);
    result.removeAll(QnUuid());

    return result;
}

QnUuidList getResourceIds(const AggregatedEventPtr& event)
{
    auto result = getDeviceIds(event);
    result << getFieldValue<QnUuid>(event, kServerIdFieldName);
    result << getFieldValue<QnUuid>(event, kEngineIdFieldName);
    // TODO: #amalov Consider reporting user in resource list.
    result.removeAll(QnUuid());

    return result;
}

QnUuidList getResourceIds(const ActionPtr& action)
{
    QnUuidList result;
    result << getFieldValue<QnUuid>(action, kCameraIdFieldName);
    result << getFieldValue<QnUuidList>(action, kDeviceIdsFieldName);
    result << getFieldValue<QnUuid>(action, kServerIdFieldName);
    result.removeAll(QnUuid());

    return result;
}

} // namespace nx::vms::rules::utils
