// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Shapes 1.15

import Nx 1.0
import Nx.Core 1.0
import Nx.Controls 1.0

import nx.vms.client.desktop 1.0

import ".."
import "../Components"

Item
{
    id: control

    property bool isSelf: false

    property bool deleteAvailable: true
    property bool auditAvailable: true

    property alias login: userLoginText.text
    property bool loginEditable: false
    property alias fullName: userFullNameTextField.text
    property bool fullNameEditable: true
    property alias email: userEmailTextField.text
    property bool emailEditable: true
    property string password: ""
    property bool passwordEditable: true

    property alias userEnabled: enabledUserSwitch.checked
    property bool userEnabledEditable: true

    property alias allowInsecure: allowInsecureCheckBox.checked
    property bool allowInsecureEditable: true

    property alias model: groupsComboBox.model
    property alias parentGroupsEditable: groupsComboBox.enabled

    property bool enabled: true
    property int userType: UserSettingsGlobal.LocalUser

    property bool ldapError: false

    property var self

    property var editingContext

    signal deleteRequested()
    signal auditTrailRequested()

    signal groupClicked(var id)
    signal moreGroupsClicked()

    function validate()
    {
        let result = true

        if (userLoginText.enabled)
            result = userLoginText.validate()

        if (userEmailTextField.visible)
            return userEmailTextField.validate() && result

        return result
    }

    ColumnLayout
    {
        anchors.fill: parent

        spacing: 0

        Rectangle
        {
            id: loginPanel

            color: ColorTheme.colors.dark8
            Layout.fillWidth: true

            Layout.preferredHeight: Math.max(
                103,
                enabledUserSwitch.y + enabledUserSwitch.height + 22)

            Image
            {
                id: userTypeIcon

                x: 24
                y: 24
                width: 64
                height: 64

                source:
                {
                    switch (control.userType)
                    {
                        case UserSettingsGlobal.LocalUser:
                            return "image://svg/skin/user_settings/user_type_local.svg"
                        case UserSettingsGlobal.CloudUser:
                            return "image://svg/skin/user_settings/user_type_cloud.svg"
                        case UserSettingsGlobal.LdapUser:
                            return "image://svg/skin/user_settings/user_type_ldap.svg"
                    }
                }

                sourceSize: Qt.size(width, height)
            }

            EditableLabel
            {
                id: userLoginText

                enabled: control.loginEditable && control.enabled

                anchors.left: userTypeIcon.right
                anchors.leftMargin: 24
                anchors.top: userTypeIcon.top

                anchors.right: parent.right
                anchors.rightMargin: 16

                validateFunc: control.self
                    ? (control.userType == UserSettingsGlobal.CloudUser
                        ? control.self.validateEmail : control.self.validateLogin)
                    : null
            }

            UserEnabledSwitch
            {
                id: enabledUserSwitch

                enabled: control.userEnabledEditable && control.enabled

                anchors.top: userLoginText.bottom
                anchors.topMargin: 14
                anchors.left: userLoginText.left
            }

            Row
            {
                anchors.top: userLoginText.bottom
                anchors.topMargin: 14
                anchors.right: parent.right
                anchors.rightMargin: 20
                spacing: 12

                TextButton
                {
                    id: auditTrailButton

                    icon.source: "image://svg/skin/user_settings/audit_trail.svg"
                    icon.width: 12
                    icon.height: 14
                    spacing: 8
                    text: qsTr("Audit Trail")
                    visible: control.auditAvailable
                    onClicked: control.auditTrailRequested()
                }

                TextButton
                {
                    id: deleteButton

                    visible: control.deleteAvailable
                    enabled: control.enabled
                    icon.source: "image://svg/skin/user_settings/user_delete.svg"
                    icon.width: 12
                    icon.height: 14
                    spacing: 8
                    text: qsTr("Delete")
                    onClicked: control.deleteRequested()
                }
            }
        }

        Rectangle
        {
            color: ColorTheme.colors.dark6

            Layout.fillWidth: true
            height: 1
        }

        Scrollable
        {
            id: scroll

            Layout.fillWidth: true
            Layout.fillHeight: true

            contentItem: Rectangle
            {
                color: ColorTheme.colors.dark7

                width: scroll.width
                height: childrenRect.height

                Column
                {
                    spacing: 8
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.right: parent.right
                    anchors.rightMargin: 16

                    SectionHeader
                    {
                        text: qsTr("Info")
                    }

                    CenteredField
                    {
                        text: qsTr("Full Name")

                        TextField
                        {
                            id: userFullNameTextField
                            width: parent.width
                            readOnly: !control.fullNameEditable
                                || !control.enabled
                                || control.userType == UserSettingsGlobal.CloudUser
                        }
                    }

                    CenteredField
                    {
                        visible: control.userType == UserSettingsGlobal.CloudUser && control.isSelf

                        Item
                        {
                            id: externalLink

                            width: childrenRect.width
                            height: childrenRect.height
                            property string link: "?"

                            Text
                            {
                                id: linkText
                                text: qsTr("Account Settings")
                                color: ColorTheme.colors.brand_core
                                font: Qt.font({pixelSize: 14, weight: Font.Normal, underline: true})
                            }

                            Shape
                            {
                                id: linkArrow

                                width: 5
                                height: 5

                                anchors.left: linkText.right
                                anchors.verticalCenter: linkText.verticalCenter
                                anchors.verticalCenterOffset: -1
                                anchors.leftMargin: 6

                                ShapePath
                                {
                                    strokeWidth: 1
                                    strokeColor: linkText.color
                                    fillColor: "transparent"

                                    startX: 1; startY: 0
                                    PathLine { x: linkArrow.width; y: 0 }
                                    PathLine { x: linkArrow.width; y: linkArrow.height - 1 }
                                    PathMove { x: linkArrow.width; y: 0 }
                                    PathLine { x: 0; y: linkArrow.height }
                                }
                            }

                            MouseArea
                            {
                                id: mouseArea
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked:
                                {
                                    Qt.openUrlExternally(UserSettingsGlobal.accountManagementUrl())
                                }
                            }
                        }
                    }

                    CenteredField
                    {
                        text: qsTr("Email")
                        visible: control.userType != UserSettingsGlobal.CloudUser

                        TextFieldWithValidator
                        {
                            id: userEmailTextField
                            width: parent.width
                            readOnly: !(control.emailEditable && control.enabled)
                            validateFunc: (text) =>
                            {
                                return control.self && enabled
                                    ? control.self.validateEmail(
                                        text, control.userType == UserSettingsGlobal.CloudUser)
                                    : ""
                            }
                        }
                    }

                    CenteredField
                    {
                        // Change password.

                        visible: control.passwordEditable
                            && control.userType == UserSettingsGlobal.LocalUser

                        Item
                        {
                            height: changePasswordButton.height + 8

                            Component
                            {
                                id: changePasswordDialog

                                PasswordChangeDialog
                                {
                                    id: dialog

                                    transientParent: control.Window.window
                                    visible: false

                                    login: control.login
                                    askCurrentPassword: control.isSelf
                                    currentPasswordValidator: control.isSelf
                                        ? dialog.self.validateCurrentPassword
                                        : null

                                    onAccepted:
                                    {
                                        control.password = newPassword
                                    }

                                    Connections
                                    {
                                        target: control.Window.window
                                        function onClosing() { dialog.reject() }
                                    }
                                }
                            }

                            Button
                            {
                                y: 4
                                id: changePasswordButton
                                text: qsTr("Change password")
                                enabled: control.enabled

                                onClicked:
                                {
                                    changePasswordDialog.createObject(control).openNew()
                                }
                            }
                        }
                    }

                    CenteredField
                    {
                        // Allow digest authentication.

                        visible: control.userType != UserSettingsGlobal.CloudUser

                        Component
                        {
                            id: changePasswordDigestDialog

                            PasswordChangeDialog
                            {
                                id: dialog

                                transientParent: control.Window.window
                                visible: false

                                text: qsTr("Set password to enable insecure authentication")
                                login: control.login
                                askCurrentPassword: control.isSelf
                                showLogin: true

                                currentPasswordValidator: control.isSelf
                                    ? dialog.self.validateCurrentPassword
                                    : null

                                onAccepted:
                                {
                                    allowInsecureCheckBox.checked = true
                                    control.password = newPassword
                                }

                                Connections
                                {
                                    target: control.Window.window
                                    function onClosing() { dialog.reject() }
                                }
                            }
                        }

                        CheckBox
                        {
                            id: allowInsecureCheckBox

                            text: qsTr("Allow insecure (digest) authentication")
                            font.pixelSize: 14

                            enabled: control.allowInsecureEditable && control.enabled

                            wrapMode: Text.WordWrap

                            anchors
                            {
                                left: parent.left
                                right: parent.right
                                leftMargin: -3
                            }

                            nextCheckState: () =>
                            {
                                if (checkState === Qt.Unchecked)
                                    changePasswordDigestDialog.createObject(control).openNew()

                                return Qt.Unchecked
                            }
                        }
                    }

                    CenteredField
                    {
                        visible: allowInsecureCheckBox.checked

                        InsecureWarning
                        {
                            width: parent.width
                        }
                    }

                    CenteredField
                    {
                        text: qsTr("Permission Groups")

                        GroupsComboBox
                        {
                            id: groupsComboBox

                            width: parent.width
                        }
                    }
                }
            }
        }
    }

    DialogBanner
    {
        id: banner

        style: DialogBanner.Style.Error

        visible: false
        closeVisible: true

        Binding
        {
            target: banner
            property: "visible"
            value: control.ldapError
        }

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        text: qsTr("This user is not found in LDAP database and is not able to log in.")
        buttonText: control.deleteAvailable && control.enabled ? qsTr("Delete") : ""
        buttonIcon: "image://svg/skin/user_settings/trash.svg"

        onButtonClicked: control.deleteRequested()
        onCloseClicked: banner.visible = false
    }
}
