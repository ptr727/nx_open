// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "resource_access_rights_model.h"

#include <algorithm>
#include <vector>

#include <QtCore/QPointer>
#include <QtQml/QtQml>

#include <core/resource/camera_resource.h>
#include <core/resource/layout_resource.h>
#include <core/resource/media_server_resource.h>
#include <core/resource/videowall_resource.h>
#include <core/resource/webpage_resource.h>
#include <core/resource_access/subject_hierarchy.h>
#include <core/resource_management/resource_pool.h>
#include <nx/utils/log/format.h>
#include <nx/utils/scoped_connections.h>
#include <nx/vms/client/desktop/resource/layout_resource.h>
#include <nx/vms/client/desktop/resource_views/data/resource_tree_globals.h>
#include <nx/vms/client/desktop/resource_properties/user/utils/access_subject_editing_context.h>
#include <nx/vms/client/desktop/system_context.h>
#include <nx/vms/common/html/html.h>
#include <nx/vms/common/user_management/user_group_manager.h>

// We removed common metatype declaration but for some reason it's still required here for proper
// `QVector<AccessRight>` registration.
Q_DECLARE_METATYPE(nx::vms::api::AccessRight)

namespace nx::vms::client::desktop {

using namespace nx::core::access;
using namespace nx::vms::api;

namespace {

void modifyAccessRights(ResourceAccessMap& accessMap, const QnUuid& resourceOrGroupId,
    AccessRights accessRightsMask, bool value)
{
    const auto accessRights = accessMap.value(resourceOrGroupId);
    const auto newAccessRights = value
        ? accessRights | accessRightsMask
        : accessRights & ~accessRightsMask;

    if (newAccessRights == accessRights)
        return;

    if (newAccessRights != 0)
        accessMap.emplace(resourceOrGroupId, newAccessRights);
    else
        accessMap.remove(resourceOrGroupId);
}

} // namespace

// ------------------------------------------------------------------------------------------------
// ResourceAccessInfo

bool ResourceAccessInfo::operator==(const ResourceAccessInfo& other) const
{
    return providedVia == other.providedVia
        && providerUserGroups == other.providerUserGroups
        && indirectProviders == other.indirectProviders
        && checkedChildCount == other.checkedChildCount;
}

bool ResourceAccessInfo::operator!=(const ResourceAccessInfo& other) const
{
    return !(*this == other);
}

// ------------------------------------------------------------------------------------------------
// ResourceAccessRightsModel

struct ResourceAccessRightsModel::Private
{
    ResourceAccessRightsModel* const q;

    QPointer<AccessSubjectEditingContext> context;
    QVector<AccessRight> accessRightList;
    QnResourcePtr resource;
    ResourceTree::NodeType nodeType = ResourceTree::NodeType::spacer;
    QVector<ResourceAccessInfo> info;

    nx::utils::ScopedConnections contextConnections;

    void updateInfo(bool suppressSignals = false);
    QnResourceList getResources() const;
    QnResourceList getGroupResources(const QnUuid& groupId) const;
    std::vector<QnUuid> getGroupContents(const QnUuid& groupId) const;

    void countGroupResources(const QnUuid& groupId, AccessRight accessRight,
        int& checked, int& total) const;

    AccessRights relevantAccessRights() const;

    bool isEditable(int index) const;

    QString accessDetailsText(const ResourceAccessInfo& accessInfo) const;
};

ResourceAccessRightsModel::ResourceAccessRightsModel(QObject* parent):
    base_type(parent),
    d(new Private{this})
{
}

ResourceAccessRightsModel::~ResourceAccessRightsModel()
{
    // Required here for forward-declared scoped pointer destruction.
}

AccessSubjectEditingContext* ResourceAccessRightsModel::context() const
{
    return d->context;
}

void ResourceAccessRightsModel::setContext(AccessSubjectEditingContext* value)
{
    if (d->context == value)
        return;

    d->contextConnections.reset();

    d->context = value;
    d->updateInfo();

    if (d->context)
    {
        d->contextConnections <<
            connect(d->context, &AccessSubjectEditingContext::resourceAccessChanged, this,
                [this]() { d->updateInfo(); });

        d->contextConnections <<
            connect(d->context, &AccessSubjectEditingContext::resourceGroupsChanged, this,
                [this](const QSet<QnUuid>& resourceGroupIds)
                {
                    if (resourceGroupIds.contains(groupId()))
                        d->updateInfo();
                });
    }

    emit contextChanged();
}

QVector<AccessRight> ResourceAccessRightsModel::accessRightsList() const
{
    return d->accessRightList;
}

void ResourceAccessRightsModel::setAccessRightsList(const QVector<AccessRight>& value)
{
    if (d->accessRightList == value)
        return;

    beginResetModel();
    d->accessRightList = value;
    d->updateInfo(/*suppressSignals*/ true);
    endResetModel();

    emit accessRightsListChanged();
}

QnResource* ResourceAccessRightsModel::resource() const
{
    return d->resource.get();
}

void ResourceAccessRightsModel::setResource(QnResource* value)
{
    const auto shared = value ? value->toSharedPointer() : QnResourcePtr();
    if (!NX_ASSERT(shared.get() == value, "Cannot obtain a shared pointer to the resource"))
        return;

    if (d->resource == shared)
        return;

    d->resource = shared;
    d->updateInfo();

    emit resourceChanged();
}

QnUuid ResourceAccessRightsModel::groupId() const
{
    switch (d->nodeType)
    {
        case ResourceTree::NodeType::camerasAndDevices:
            return nx::vms::api::kAllDevicesGroupId;

        case ResourceTree::NodeType::videoWalls:
            return nx::vms::api::kAllVideoWallsGroupId;

        case ResourceTree::NodeType::integrations:
        case ResourceTree::NodeType::webPages:
            return nx::vms::api::kAllWebPagesGroupId;

        case ResourceTree::NodeType::servers:
            return nx::vms::api::kAllServersGroupId;

        default:
            return QnUuid();
    }
}

ResourceTree::NodeType ResourceAccessRightsModel::nodeType() const
{
    return d->nodeType;
}

void ResourceAccessRightsModel::setNodeType(ResourceTree::NodeType value)
{
    if (d->nodeType == value)
        return;

    d->nodeType = value;
    d->updateInfo();

    emit nodeTypeChanged();
}

QVariant ResourceAccessRightsModel::data(const QModelIndex& index, int role) const
{
    if (index.row() >= rowCount() || index.row() < 0)
        return {};

    switch (role)
    {
        case ProviderRole:
            return static_cast<int>(d->info[index.row()].providedVia);
        case TotalChildCountRole:
            return static_cast<int>(d->info[index.row()].totalChildCount);
        case CheckedChildCountRole:
            return static_cast<int>(d->info[index.row()].checkedChildCount);
        case AccessRightRole:
            return static_cast<int>(d->accessRightList[index.row()]);
        case EditableRole:
            return d->isEditable(index.row());
        case Qt::ToolTipRole:
            return d->accessDetailsText(d->info[index.row()]);
        default:
            return {};
    }
}

void ResourceAccessRightsModel::toggle(int index, bool withDependentAccessRights)
{
    if (index >= rowCount() || index < 0 || !d->context)
        return;

    const auto id = d->resource ? d->resource->getId() : groupId();
    if (!NX_ASSERT(!id.isNull()))
        return;

    const auto toggledRight = d->accessRightList[index];

    const bool isGroup = !d->resource;
    const auto outerGroupId = AccessSubjectEditingContext::specialResourceGroupFor(d->resource);
    const bool hasOuterGroup = !outerGroupId.isNull();

    auto accessMap = d->context->ownResourceAccessMap();
    const auto itemAccessRights = accessMap.value(id);

    const auto outerGroupAccessRights = hasOuterGroup
        ? accessMap.value(outerGroupId)
        : AccessRights{};

    const auto& info = d->info[index];
    const auto allChildrenWereChecked = info.totalChildCount > 0
        && info.totalChildCount == info.checkedChildCount;

    const bool outerGroupWasChecked = outerGroupAccessRights.testFlag(toggledRight);
    const bool itemWasChecked = outerGroupWasChecked || itemAccessRights.testFlag(toggledRight);
    const bool itemWillBeChecked = !(itemWasChecked || allChildrenWereChecked);

    AccessRights toggledMask = toggledRight;
    if (withDependentAccessRights)
    {
        toggledMask |= (itemWillBeChecked
            ? AccessSubjectEditingContext::requiredAccessRights(toggledRight)
            : AccessSubjectEditingContext::dependentAccessRights(toggledRight));
    }

    if (isGroup)
    {
        // If we're toggling a group on, we must explicitly toggle all its children off.
        // If we're toggling a group off, we must explicitly toggle all its children on.

        auto mask = itemWasChecked
            ? (toggledMask & itemAccessRights)
            : toggledMask;

        for (const auto& itemId: d->getGroupContents(id))
            modifyAccessRights(accessMap, itemId, mask, itemWasChecked);
    }

    if (outerGroupWasChecked)
    {
        // If we're toggling off an item that was implicitly toggled on by its group,
        // we must toggle the group off, and explicitly toggle all its children on.

        for (const auto& itemId: d->getGroupContents(outerGroupId))
            modifyAccessRights(accessMap, itemId, toggledMask, true);

        modifyAccessRights(accessMap, outerGroupId, toggledMask, false);
    }

    // Toggle the item.
    modifyAccessRights(accessMap, id, toggledMask, itemWillBeChecked);
    d->context->setOwnResourceAccessMap(accessMap);
}

QHash<int, QByteArray> ResourceAccessRightsModel::roleNames() const
{
    auto names = base_type::roleNames();
    names.insert(ProviderRole, "providedVia");
    names.insert(TotalChildCountRole, "totalChildCount");
    names.insert(CheckedChildCountRole, "checkedChildCount");
    names.insert(AccessRightRole, "accessRight");
    names.insert(EditableRole, "editable");
    return names;
}

int ResourceAccessRightsModel::rowCount(const QModelIndex&) const
{
    return d->accessRightList.size();
}

QVector<ResourceAccessInfo> ResourceAccessRightsModel::info() const
{
    return d->info;
}

ResourceAccessInfo::ProvidedVia ResourceAccessRightsModel::providerType(QnResource* provider)
{
    if (qobject_cast<QnLayoutResource*>(provider))
        return ResourceAccessInfo::ProvidedVia::layout;

    if (qobject_cast<QnVideoWallResource*>(provider))
        return ResourceAccessInfo::ProvidedVia::videowall;

    NX_ASSERT(false, "Unknown indirect provider type: %1", provider);
    return ResourceAccessInfo::ProvidedVia::unknown;
}

void ResourceAccessRightsModel::registerQmlTypes()
{
    qRegisterMetaType<QVector<AccessRight>>();
    qRegisterMetaType<QVector<ResourceAccessInfo>>();
    qRegisterMetaType<QVector<QnUuid>>();
    qRegisterMetaType<QVector<QnResource*>>();

    qmlRegisterUncreatableType<ResourceAccessInfo>("nx.vms.client.desktop", 1, 0,
        "ResourceAccessInfo", "Cannot create an instance of ResourceAccessInfo");

    qmlRegisterType<ResourceAccessRightsModel>(
        "nx.vms.client.desktop", 1, 0, "ResourceAccessRightsModel");
}

// ------------------------------------------------------------------------------------------------
// ResourceAccessRightsModel::Private

QnResourceList ResourceAccessRightsModel::Private::getResources() const
{
    if (!context)
        return {};

    const auto resourcePool = context->systemContext()->resourcePool();

    switch (nodeType)
    {
        case ResourceTree::NodeType::camerasAndDevices:
            return resourcePool->getAllCameras(QnUuid{}, /*ignoreDesktopCameras*/ true);

        case ResourceTree::NodeType::integrations:
        case ResourceTree::NodeType::webPages:
            return resourcePool->getResources<QnWebPageResource>();

        case ResourceTree::NodeType::layouts:
            return resourcePool->getResources<QnLayoutResource>().filtered(
                [](const QnLayoutResourcePtr& layout)
                {
                    return !layout->isFile() && !layout->hasFlags(Qn::cross_system);
                });

        case ResourceTree::NodeType::servers:
            return resourcePool->getResources<QnMediaServerResource>();

        case ResourceTree::NodeType::videoWalls:
            return resourcePool->getResources<QnVideoWallResource>();

        default:
            return resource ? QnResourceList{resource} : QnResourceList{};
    }
}

void ResourceAccessRightsModel::Private::updateInfo(bool suppressSignals)
{
    QVector<ResourceAccessInfo> newResourceAccessInfo;

    const int count = accessRightList.size();
    newResourceAccessInfo.resize(count);

    const auto groupId = q->groupId();
    const bool isResourceGroup = !groupId.isNull();

    if (context && !context->currentSubjectId().isNull() && (resource || isResourceGroup))
    {
        for (int i = 0; i < count; ++i)
        {
            auto& newInfo = newResourceAccessInfo[i];
            if (isResourceGroup)
            {
                if (context->hasOwnAccessRight(groupId, accessRightList[i]))
                {
                    newInfo.providedVia = ResourceAccessInfo::ProvidedVia::own;
                }
                else
                {
                    countGroupResources(groupId, accessRightList[i],
                        newInfo.checkedChildCount, newInfo.totalChildCount);
                }

                continue;
            }

            const auto details = context->accessDetails(resource, accessRightList[i]);
            if (details.contains(context->currentSubjectId()))
            {
                const auto providers = details.value(context->currentSubjectId());
                if (providers.contains(resource))
                {
                    const auto resourceGroupId =
                        AccessSubjectEditingContext::specialResourceGroupFor(resource);

                    const auto accessViaResourceGroup = !resourceGroupId.isNull()
                        && context->hasOwnAccessRight(resourceGroupId, accessRightList[i]);

                    newInfo.providedVia = accessViaResourceGroup
                        ? ResourceAccessInfo::ProvidedVia::ownResourceGroup
                        : ResourceAccessInfo::ProvidedVia::own;
                }
                else
                {
                    for (const auto& provider: providers)
                    {
                        if (!NX_ASSERT(provider))
                            continue;

                        static const auto priorityKey =
                            [](ResourceAccessInfo::ProvidedVia providerType)
                            {
                                switch (providerType)
                                {
                                    case ResourceAccessInfo::ProvidedVia::videowall:
                                        return 1;
                                    case ResourceAccessInfo::ProvidedVia::layout:
                                        return 2;
                                    default:
                                        return 0;
                                }
                            };

                        const auto providedVia = providerType(provider.get());
                        if (priorityKey(providedVia) > priorityKey(newInfo.providedVia))
                            newInfo.providedVia = providedVia;

                        // Keep arrays sorted for easy comparison.
                        const auto insertionPos = std::upper_bound(
                            newInfo.indirectProviders.begin(),
                            newInfo.indirectProviders.end(),
                            provider.get());

                        newInfo.indirectProviders.insert(insertionPos, provider);
                    }
                }
            }
            else if (!details.empty())
            {
                newInfo.providedVia = ResourceAccessInfo::ProvidedVia::parentUserGroup;

                // Show only the direct parents which provide current access rights.
                const auto directParents =
                    context->subjectHierarchy()->directParents(context->currentSubjectId());

                const QSet<QnUuid> providerIds{details.keyBegin(), details.keyEnd()};

                newInfo.providerUserGroups = {};
                for (const auto& key: directParents)
                {
                    if (providerIds.contains(key)
                        || context->subjectHierarchy()->isRecursiveMember(key, providerIds))
                    {
                        newInfo.providerUserGroups << key;
                    }
                }

                // Keep arrays sorted for easy comparison.
                std::sort(newInfo.providerUserGroups.begin(), newInfo.providerUserGroups.end());
            }
        }
    }

    if (newResourceAccessInfo == info)
        return;

    info = newResourceAccessInfo;

    if (info.size() > 0 && !suppressSignals)
    {
        emit q->dataChanged(
            q->index(0, 0),
            q->index(info.size() - 1, 0));
    }
}

QnResourceList ResourceAccessRightsModel::Private::getGroupResources(const QnUuid& groupId) const
{
    // Only special resource groups are supported at this time.

    const auto group = specialResourceGroup(groupId);
    if (!context || !NX_ASSERT(group))
        return {};

    const auto resourcePool = context->systemContext()->resourcePool();
    switch (*group)
    {
        case SpecialResourceGroup::allDevices:
            return resourcePool->getAllCameras(QnUuid{}, true);

        case SpecialResourceGroup::allServers:
            return resourcePool->servers();

        case SpecialResourceGroup::allWebPages:
            return resourcePool->getResources<QnWebPageResource>();

        case SpecialResourceGroup::allVideowalls:
            return resourcePool->getResources<QnVideoWallResource>();
    }

    NX_ASSERT(false, "Unhandled special resource group type");
    return {};
}

std::vector<QnUuid> ResourceAccessRightsModel::Private::getGroupContents(
    const QnUuid& groupId) const
{
    // Only special resource groups are supported at this time.

    const auto groupResources = getGroupResources(groupId);
    if (groupResources.empty())
        return {};

    std::vector<QnUuid> result;
    result.reserve(groupResources.size());
    std::transform(groupResources.begin(), groupResources.end(), std::back_inserter(result),
        [](const QnResourcePtr& resource) { return resource->getId(); });
    return result;
}

void ResourceAccessRightsModel::Private::countGroupResources(
    const QnUuid& groupId, AccessRight accessRight, int& checked, int& total) const
{
    const auto contents = getGroupResources(groupId);
    total = contents.size();

    const auto accessMap = context->ownResourceAccessMap();
    checked = std::count_if(contents.cbegin(), contents.cend(),
        [accessMap, accessRight](const QnResourcePtr& resource)
        {
            return accessMap.value(resource->getId()).testFlag(accessRight);
        });
}

QString ResourceAccessRightsModel::Private::accessDetailsText(
    const ResourceAccessInfo& accessInfo) const
{
    using namespace nx::vms::common;

    if (!resource)
        return {};

    QStringList groups, layouts, videoWalls;
    QStringList descriptions;

    for (const auto& groupId: accessInfo.providerUserGroups)
    {
        if (const auto group = resource->systemContext()->userGroupManager()->find(groupId))
            groups << html::bold(group->name);
    }

    for (const auto& providerResource: accessInfo.indirectProviders)
    {
        if (auto layout = providerResource.dynamicCast<QnLayoutResource>())
            layouts << html::bold(layout->getName());
        else if (auto videoWall = providerResource.dynamicCast<QnVideoWallResource>())
            videoWalls << html::bold(videoWall->getName());
    }

    const auto makeDescription =
        [this](const QString& single, const QString& plural, const QStringList& list)
        {
            return list.size() == 1
                ? nx::format(single, list.front()).toQString()
                : nx::format(plural, list.join(", ")).toQString();
        };

    if (!groups.empty())
    {
        descriptions << makeDescription(
            tr("Access granted by %1 group"),
            tr("Access granted by %n groups: %1", "", groups.size()),
            groups);
    }

    if (!layouts.empty())
    {
        descriptions << makeDescription(
            tr("Access granted by %1 layout"),
            tr("Access granted by %n layouts: %1", "", layouts.size()),
            layouts);
    }

    if (!videoWalls.empty())
    {
        descriptions << makeDescription(
            tr("Access granted by %1 video wall"),
            tr("Access granted by %n video walls: %1", "", videoWalls.size()),
            videoWalls);
    }

    return descriptions.join("<br>");
}

AccessRights ResourceAccessRightsModel::Private::relevantAccessRights() const
{
    if (resource)
        return AccessSubjectEditingContext::relevantAccessRights(resource);

    if (const auto group = nx::vms::api::specialResourceGroup(q->groupId()))
        return AccessSubjectEditingContext::relevantAccessRights(*group);

    return {};
}

bool ResourceAccessRightsModel::Private::isEditable(int index) const
{
    return relevantAccessRights().testFlag(accessRightList[index]);
}

} // namespace nx::vms::client::desktop
