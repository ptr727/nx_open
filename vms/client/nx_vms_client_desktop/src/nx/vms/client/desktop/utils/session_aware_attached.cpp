// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include <QtQml/QtQml>

#include "session_aware_attached.h"

#include <ui/workbench/workbench_state_manager.h>

namespace nx::vms::client::desktop {

struct SessionAwareAttached::Private: public QnSessionAwareDelegate
{
    using base_type = QnSessionAwareDelegate;

    Private(SessionAwareAttached* sessionAwareAttached, QObject* parent):
        base_type(parent, InitializationMode::qmlContext),
        q(sessionAwareAttached)
    {
    }

    bool tryClose(bool force) override
    {
        SessionAwareCloseEvent closeEvent;
        closeEvent.force = force;

        QQmlEngine::setObjectOwnership(&closeEvent, QQmlEngine::CppOwnership);
        emit q->tryClose(&closeEvent);

        return closeEvent.accepted;
    }

    void forcedUpdate() override
    {
        emit q->forcedUpdate();
    }

    SessionAwareAttached* const q;
};

SessionAwareAttached::SessionAwareAttached(QObject* parent):
    d(new Private(this, parent))
{
}

SessionAwareAttached::~SessionAwareAttached()
{
}

SessionAwareAttached* SessionAware::qmlAttachedProperties(QObject* object)
{
    return new SessionAwareAttached(object);
}

void SessionAware::registerQmlType()
{
    qmlRegisterType<SessionAware>("nx.vms.client.desktop", 1, 0, "SessionAware");
}

} // namespace nx::vms::client::desktop
