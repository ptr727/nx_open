// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

import QtQuick
import QtQuick.Controls
import QtQml

import Nx
import Nx.Controls
import Nx.Core

import nx.vms.client.desktop
import nx.vms.client.desktop.analytics as Analytics

Control
{
    id: control
    required property Analytics.StateView taxonomy
    required property string objectTypeId
    required property string attributeName

    property Analytics.ObjectType objectType: taxonomy.objectTypeById(objectTypeId)
    property Analytics.Attribute attribute: findAttribute()
    property bool isGeneric: !objectType

    signal valueChanged(string value)

    function findAttributeRecursive(objectType, nameParts)
    {
        if (!objectType || nameParts.length < 1)
            return null

        const name = nameParts[0]
        for (let i = 0; i < objectType.attributes.length; ++i)
        {
            const attribute = objectType.attributes[i]
            if (attribute.name == name)
            {
                if (nameParts.length == 1)
                    return attribute

                if (attribute.type !== Analytics.Attribute.Type.attributeSet)
                {
                    console.warn(loggingCategory,
                        `Nested attribute ${attribute.name} is not an attribute set`)
                    return null
                }

                return findAttributeRecursive(attribute.attributeSet, nameParts.slice(1))
            }
        }
        return null
    }

    function findAttribute()
    {
        if (!objectType)
            return null

        return findAttributeRecursive(objectType, attributeName.split('.'))
    }

    Component
    {
        id: textDelegate

        TextField
        {
            onTextChanged: valueChanged(text)
        }
    }

    Component
    {
        id: numberDelegate

        NumberInput
        {
            minimum: attribute.minValue
            maximum: attribute.maxValue

            mode: attribute.subtype === "int"
                ? NumberInput.Integer
                : NumberInput.Real

            onValueChanged: control.valueChanged(value)
        }
    }

    Component
    {
        id: booleanDelegate

        ComboBox
        {
            textRole: "text"
            valueRole: "value"
            model: [
                {"text": qsTr("Any %1").arg(attribute.name), "value": undefined},
                {"text": qsTr("Yes"), "value": "0"},
                {"text": qsTr("No"), "value": "1"}
            ]
            onActivated: valueChanged(currentValue)
        }
    }

    Component
    {
        id: colorDelegate

        ComboBox
        {
            withColorSection: true
            textRole: "name"
            valueRole: "color"
            model:
            {
                return [{"name": qsTr("Any %1").arg(attribute.name), "value": undefined}].concat(
                    Array.prototype.map.call(attribute.colorSet.items,
                        name => ({"name": name, "color": attribute.colorSet.color(name)})))
            }
            onActivated: valueChanged(currentValue)
        }
    }

    Component
    {
        id: enumDelegate

        ComboBox
        {
            textRole: "text"
            valueRole: "value"
            model:
            {
                return [{"text": qsTr("Any %1").arg(attribute.name), "value": undefined}].concat(
                    Array.prototype.map.call(attribute.enumeration.items,
                        text => ({"text": text, "value": text})))
            }
            onActivated: valueChanged(currentValue)
        }
    }

    function calculateComponent()
    {
        if (isGeneric)
            return textDelegate

        if (!attribute)
        {
            console.warn(loggingCategory,
                `Attribute ${attributeName} was not found in ${objectTypeId}`)
            return textDelegate
        }

        switch (attribute.type)
        {
            case Analytics.Attribute.Type.number:
                return numberDelegate

            case Analytics.Attribute.Type.boolean:
                return booleanDelegate

            case Analytics.Attribute.Type.string:
                return textDelegate

            case Analytics.Attribute.Type.colorSet:
                return colorDelegate

            case Analytics.Attribute.Type.enumeration:
                return enumDelegate

            default:
                console.warn(loggingCategory,
                    `Unknown analytics attribute type "${attributeEditor.attribute.type}"`)
        }
        return textDelegate
    }

    contentItem: Loader
    {
        sourceComponent: calculateComponent()
    }

    LoggingCategory
    {
        id: loggingCategory
        name: "Nx.LookupLists"
    }
}