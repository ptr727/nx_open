// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtCore/QAbstractTableModel>
#include <QtCore/QPointer>

#include <nx/vms/api/data/lookup_list_data.h>

#include "lookup_list_model.h"

namespace nx::vms::client::desktop {

/** Entries of a single Lookup List. */
class LookupListEntriesModel: public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(LookupListModel* listModel
        READ listModel
        WRITE setListModel
        NOTIFY listModelChanged);

    using base_type = QAbstractTableModel;

    enum DataRole
    {
        TypeRole = Qt::UserRole
    };

public:
    explicit LookupListEntriesModel(QObject* parent = nullptr);
    virtual ~LookupListEntriesModel();

    virtual QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    virtual QHash<int, QByteArray> roleNames() const override;

    LookupListModel* listModel() const;
    void setListModel(LookupListModel* value);

    Q_INVOKABLE void addEntry(const QVariantMap& value);

signals:
    void listModelChanged();

private:
    QPointer<LookupListModel> m_data;
};

} // namespace nx::vms::client::desktop
