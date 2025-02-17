// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include "user_data.h"

namespace nx::vms::api {

// VMS Transaction structure before 5.2.
struct NX_VMS_API UserDataDeprecated: ResourceData
{
    UserDataDeprecated(): ResourceData(UserData::kResourceTypeId) {}

    UserDataDeprecated(const UserDataDeprecated& other) = default;
    UserDataDeprecated(UserDataDeprecated&& other) = default;
    UserDataDeprecated& operator=(const UserDataDeprecated& other) = default;
    UserDataDeprecated& operator=(UserDataDeprecated&& other) = default;
    bool operator==(const UserDataDeprecated& other) const = default;

    void fillId();

    /**
     * Converts this legacy user data into the new one assigning default values to new fields.
     */
    UserData toUserData() const;

    bool isAdmin = false;
    GlobalPermissions permissions = GlobalPermission::none;
    QnUuid userRoleId;
    std::vector<QnUuid> userRoleIds;
    QString email;
    QnLatin1Array digest;
    QnLatin1Array hash;
    QnLatin1Array cryptSha512Hash;
    QString realm;
    bool isLdap = false;
    bool isEnabled = true;
    bool isCloud = false;
    QString fullName;

    static std::optional<QnUuid> permissionPresetToGroupId(GlobalPermissions preset);
    static GlobalPermissions groupIdToPermissionPreset(const QnUuid& id);
};
#define UserDataDeprecated_Fields \
    ResourceData_Fields \
    (isAdmin) \
    (permissions) \
    (email) \
    (digest) \
    (hash) \
    (cryptSha512Hash) \
    (realm) \
    (isLdap) \
    (isEnabled) \
    (userRoleId)  \
    (isCloud)  \
    (fullName) \
    (userRoleIds)
NX_VMS_API_DECLARE_STRUCT_AND_LIST(UserDataDeprecated)
NX_REFLECTION_INSTRUMENT(UserDataDeprecated, UserDataDeprecated_Fields)

} // namespace nx::vms::api
