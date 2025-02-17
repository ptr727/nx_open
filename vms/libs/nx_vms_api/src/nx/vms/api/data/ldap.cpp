// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "ldap.h"

#include <nx/fusion/model_functions.h>

namespace nx::vms::api {

bool LdapSettingsBase::isValid(bool checkPassword) const
{
    return !uri.isEmpty()
        && (!checkPassword || !(adminDn.isEmpty() ^ adminPassword.value_or(QString()).isEmpty()));
}

int LdapSettingsBase::defaultPort(bool useSsl)
{
    constexpr int kDefaultLdapPort = 389;
    constexpr int kDefaultLdapSslPort = 636;
    return useSsl ? kDefaultLdapSslPort : kDefaultLdapPort;
}

LdapSettingsDeprecated::LdapSettingsDeprecated(LdapSettings settings)
{
    static_cast<LdapSettingsBase&>(*this) = static_cast<LdapSettingsBase&&>(settings);
    if (!settings.filters.empty())
    {
        searchBase = settings.filters[0].base;
        searchFilter = settings.filters[0].filter;
    }
}

QN_FUSION_ADAPT_STRUCT_FUNCTIONS(LdapSettingsDeprecated, (json), LdapSettingsDeprecated_Fields)
QN_FUSION_ADAPT_STRUCT_FUNCTIONS(LdapSettingSearchFilter, (json), LdapSettingSearchFilter_Fields)
QN_FUSION_ADAPT_STRUCT_FUNCTIONS(LdapSettings, (json), LdapSettings_Fields)
QN_FUSION_ADAPT_STRUCT_FUNCTIONS(LdapSettingsChange, (json), LdapSettingsChange_Fields)
QN_FUSION_ADAPT_STRUCT_FUNCTIONS(LdapUser, (json), LdapUser_Fields)
QN_FUSION_ADAPT_STRUCT_FUNCTIONS(LdapStatus, (json), LdapStatus_Fields)

} // namespace nx::vms::api
