// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "bookmark_sort.h"

#include <algorithm>

#include <QtCore/QCoreApplication>

#include <api/helpers/camera_id_helper.h>
#include <core/resource/camera_bookmark.h>
#include <core/resource/camera_resource.h>
#include <core/resource/resource_display_info.h>
#include <core/resource/user_resource.h>
#include <core/resource_management/resource_pool.h>
#include <nx/utils/algorithm/comparator.h>
#include <nx/utils/log/assert.h>
#include <nx/utils/log/format.h>
#include <nx/vms/api/data/bookmark_models.h>
#include <utils/camera/bookmark_helpers.h>

using nx::vms::api::BookmarkSortField;

namespace {

struct BookmarkSort //< Just a context for translation functions.
{
    Q_DECLARE_TR_FUNCTIONS(BookmarkSort)
};

template<typename Bookmark>
struct BookmarkFacade
{
};

template<>
struct BookmarkFacade<QnCameraBookmark>: public BookmarkSort
{
    using Bookmark = QnCameraBookmark;

    static const auto& id(const Bookmark& bookmark) { return bookmark.guid; }
    static const auto& name(const Bookmark& bookmark) { return bookmark.name; }
    static auto description(const Bookmark& bookmark) { return bookmark.description; }
    static auto startTimeMs(const Bookmark& bookmark) { return bookmark.startTimeMs; }
    static auto durationMs(const Bookmark& bookmark) { return bookmark.durationMs; }

    static auto creationTimeMs(const Bookmark& bookmark)
    {
        return bookmark.creationTime().count();
    }

    static auto tags(const Bookmark& bookmark)
    {
        return std::set<QString>(bookmark.tags.cbegin(), bookmark.tags.cend());
    }

    static QString creatorName(const Bookmark& bookmark, QnResourcePool* resourcePool)
    {
        return helpers::getBookmarkCreatorName(bookmark.creatorId, resourcePool);
    }

    static QString cameraName(const Bookmark& bookmark, QnResourcePool* resourcePool)
    {
        if (!NX_ASSERT(resourcePool))
            return {};

        const auto cameraResource = resourcePool->getResourceById<QnVirtualCameraResource>(
            bookmark.cameraId);

        return cameraResource
            ? QnResourceDisplayInfo(cameraResource).toString(Qn::RI_NameOnly)
            : nx::format("<%1>", tr("Removed camera")).toQString();
    }
};

template<>
struct BookmarkFacade<nx::vms::api::Bookmark>: public BookmarkSort
{
    using Bookmark = nx::vms::api::Bookmark;

    static const auto& id(const Bookmark& bookmark) { return bookmark.id; }
    static const auto& name(const Bookmark& bookmark) { return bookmark.name; }
    static auto description(const Bookmark& bookmark) { return bookmark.description; }
    static auto startTimeMs(const Bookmark& bookmark) { return bookmark.startTimeMs; }
    static auto durationMs(const Bookmark& bookmark) { return bookmark.durationMs; }

    static auto creationTimeMs(const Bookmark& bookmark) { return bookmark.creationTimeMs; }

    static const auto& tags(const Bookmark& bookmark)
    {
        return bookmark.tags;
    }

    static QString creatorName(const Bookmark& bookmark, QnResourcePool* resourcePool)
    {
        if (bookmark.creatorUserId.isNull())
            return QString();

        if (bookmark.creatorUserId == QnCameraBookmark::systemUserId())
            return tr("System Event", "Shows that the bookmark was created by a system event");

        const auto userResource = resourcePool->getResourceById<QnUserResource>(
            bookmark.creatorUserId);

        return userResource ? userResource->getName() : QString();
    }

    static QString cameraName(const Bookmark& bookmark, QnResourcePool* resourcePool)
    {
        if (!NX_ASSERT(resourcePool))
            return {};

        const auto cameraResource = nx::camera_id_helper::findCameraByFlexibleId(
            resourcePool, bookmark.deviceId);

        return cameraResource
            ? QnResourceDisplayInfo(cameraResource).toString(Qn::RI_NameOnly)
            : nx::format("<%1>", tr("Removed camera")).toQString();
    }
};

template<typename Bookmark, typename GetterFunc>
std::function<bool(const Bookmark&, const Bookmark&)>
    createSortPredicate(bool ascending, GetterFunc getterFunc)
{
    return nx::utils::algorithm::Comparator(ascending, getterFunc, &BookmarkFacade<Bookmark>::id);
}

template<typename Bookmark>
std::function<bool(const Bookmark&, const Bookmark&)>
    createGenericBookmarkSortPredicate(
        BookmarkSortField sortField,
        bool ascending,
        QnResourcePool* resourcePool)
{
    using nx::utils::algorithm::Comparator;
    switch (sortField)
    {
        case BookmarkSortField::name:
            return createSortPredicate<Bookmark>(ascending, &BookmarkFacade<Bookmark>::name);

        case BookmarkSortField::description:
            return createSortPredicate<Bookmark>(ascending, &BookmarkFacade<Bookmark>::description);

        case BookmarkSortField::startTime:
            return createSortPredicate<Bookmark>(ascending, &BookmarkFacade<Bookmark>::startTimeMs);

        case BookmarkSortField::duration:
            return createSortPredicate<Bookmark>(ascending, &BookmarkFacade<Bookmark>::durationMs);

        case BookmarkSortField::creationTime:
            return createSortPredicate<Bookmark>(
                ascending, &BookmarkFacade<Bookmark>::creationTimeMs);

        case BookmarkSortField::tags:
            return createSortPredicate<Bookmark>(ascending, &BookmarkFacade<Bookmark>::tags);

        case BookmarkSortField::creator:
            return createSortPredicate<Bookmark>(ascending,
                [resourcePool](const Bookmark& bookmark)
                {
                    return BookmarkFacade<Bookmark>::creatorName(bookmark, resourcePool);
                });

        case BookmarkSortField::cameraName:
            return createSortPredicate<Bookmark>(ascending,
                [resourcePool](const Bookmark& bookmark)
                {
                    return BookmarkFacade<Bookmark>::cameraName(bookmark, resourcePool);
                });
    }

    NX_ASSERT(false, "Invalid bookmark sort field: '%1'", sortField);
    return createSortPredicate<Bookmark>(ascending, &BookmarkFacade<Bookmark>::id);
}

} // namespace

std::function<bool(const QnCameraBookmark&, const QnCameraBookmark&)>
    createBookmarkSortPredicate(
        BookmarkSortField sortField,
        Qt::SortOrder sortOrder,
        QnResourcePool* resourcePool)
{
    return createGenericBookmarkSortPredicate<QnCameraBookmark>(
        sortField, sortOrder == Qt::AscendingOrder, resourcePool);
}

namespace nx::vms::api {

std::function<bool(const Bookmark&, const Bookmark&)> createBookmarkSortPredicate(
    BookmarkSortField sortField,
    Qt::SortOrder sortOrder,
    QnResourcePool* resourcePool)
{
    return createGenericBookmarkSortPredicate<Bookmark>(
        sortField, sortOrder == Qt::AscendingOrder, resourcePool);
}

} // namespace nx::vms::api
