/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickwebcontentsview_p.h"
#include "qquickwebcontentsview_p_p.h"

#include "web_contents_adapter.h"
#include "render_widget_host_view_qt_delegate_quick.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QUrl>

// Uncomment for QML debugging
//#define UI_DELEGATES_DEBUG

class MenuItem : public QObject {
Q_OBJECT
public:
    enum Type {
        Item,
//        Menu,
        Separator
    };

    MenuItem(const QString& text, QObject *parent = 0, Type type = Item)
        : QObject(parent)
        , m_text(text)
        , m_type(type)
        , m_enabled(true)
    {
    }

    MenuItem(Type type, QObject *parent = 0)
        : QObject(parent)
        , m_type(type)
        , m_enabled(false)
    {
    }

    inline Type type() const { return m_type; }
    inline QString text() const { return m_text; }
    inline void setEnabled(bool on) { m_enabled = on; }
    inline bool enabled() const { return m_enabled; }

Q_SIGNALS:
    void triggered();

private:
    QString m_text;
    Type m_type;
    bool m_enabled;
};

class ContextMenuItems : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos CONSTANT FINAL)
    Q_PROPERTY(int count READ count CONSTANT FINAL)

public:
    enum Roles {
        EnabledRole = Qt::UserRole,
        SeparatorRole = Qt::UserRole + 1
    };

    ContextMenuItems(const QPointF &pos) : m_pos(pos) { }
    virtual int rowCount(const QModelIndex& = QModelIndex()) const { return m_items.size(); }
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames() const;
    inline QPointF pos() const { return m_pos; }
    inline int count() const { return m_items.count(); }
    inline void addItem(MenuItem* item) { m_items.append(item); item->setParent(this); }

    Q_INVOKABLE void accept(int);

Q_SIGNALS:
    void done();

private:
    QList<MenuItem*> m_items;
    QPointF m_pos;
};

QVariant ContextMenuItems::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    MenuItem* item = m_items.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item->text();
    case EnabledRole:
        return item->enabled() && (item->type() != MenuItem::Separator);
    case SeparatorRole:
        return (item->type() == MenuItem::Separator);
    }
    Q_UNREACHABLE();
    return QVariant();
}

QHash<int, QByteArray> ContextMenuItems::roleNames() const
{
    static QHash<int, QByteArray> roles;
    if (roles.isEmpty()) {
        roles.insert(Qt::DisplayRole, QByteArray("text"));
        roles.insert(EnabledRole, QByteArray("enabled"));
        roles.insert(SeparatorRole, QByteArray("isSeparator"));
    }
    return roles;
}

void ContextMenuItems::accept(int idx)
{
    if (idx >= 0 && idx < m_items.count())
        Q_EMIT m_items.at(idx)->triggered();
}

#include "qquickwebcontentsview.moc"


QQuickWebContentsViewPrivate::QQuickWebContentsViewPrivate()
    : adapter(new WebContentsAdapter(this))
    , contextMenu(0)
{
}

RenderWidgetHostViewQtDelegate *QQuickWebContentsViewPrivate::CreateRenderWidgetHostViewQtDelegate()
{
    return new RenderWidgetHostViewQtDelegateQuick;
}

void QQuickWebContentsViewPrivate::titleChanged(const QString &title)
{
    Q_Q(QQuickWebContentsView);
    Q_UNUSED(title);
    Q_EMIT q->titleChanged();
}

void QQuickWebContentsViewPrivate::urlChanged(const QUrl &url)
{
    Q_Q(QQuickWebContentsView);
    Q_UNUSED(url);
    Q_EMIT q->urlChanged();
}

void QQuickWebContentsViewPrivate::loadingStateChanged()
{
    Q_Q(QQuickWebContentsView);
    Q_EMIT q->loadingStateChanged();
}

QRectF QQuickWebContentsViewPrivate::viewportRect() const
{
    Q_Q(const QQuickWebContentsView);
    return QRectF(q->x(), q->y(), q->width(), q->height());
}

void QQuickWebContentsViewPrivate::loadFinished(bool success)
{
    Q_Q(QQuickWebContentsView);
    Q_UNUSED(success);
    Q_EMIT q->loadingStateChanged();
}

void QQuickWebContentsViewPrivate::focusContainer()
{
    Q_Q(QQuickWebContentsView);
    q->forceActiveFocus();
}

QQmlContext *QQuickWebContentsViewPrivate::createContextForComponent(QQmlComponent *component)
{
    Q_ASSERT(component);
    Q_Q(QQuickWebContentsView);

    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = new QQmlContext(qmlEngine(q)->rootContext());
    return baseContext;
}

bool QQuickWebContentsViewPrivate::contextMenuRequested(const QWebContextMenuData &data)
{
    Q_Q(QQuickWebContentsView);

    if (!contextMenu) {
        QQmlEngine* engine = qmlEngine(q);
        if (!engine)
            return false;
        contextMenu = new QQmlComponent(engine, QUrl("qrc:/qt-project.org/webengine/qml/ContextMenu.qml"), QQmlComponent::PreferSynchronous, q);
    }

    if (!contextMenu || contextMenu->status() != QQmlComponent::Ready) {
#ifdef UI_DELEGATES_DEBUG
        if (contextMenu) {
            Q_FOREACH (const QQmlError& err, contextMenu->errors())
                fprintf(stderr, "  QML error: %s\n", qPrintable(err.toString()));
        }
#endif
        return false;
    }

    QQmlContext* context(createContextForComponent(contextMenu));

    ContextMenuItems *model = new ContextMenuItems(data.pos);

    // Populate menu
    MenuItem *item = 0;

    item = new MenuItem(QObject::tr("Back"));
    QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::goBack);
    item->setEnabled(q->canGoBack());
    model->addItem(item);

    item = new MenuItem(QObject::tr("Forward"));
    QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::goForward);
    item->setEnabled(q->canGoForward());
    model->addItem(item);

    item = new MenuItem(MenuItem::Separator);
    model->addItem(item);

    item = new MenuItem((q->isLoading()? QObject::tr("Stop") : QObject::tr("Reload")));
    if (q->isLoading())
        QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::stop);
    else
        QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::reload);
    model->addItem(item);

    model->setParent(context);
    context->setContextProperty(QLatin1String("model"), model);
    context->setContextObject(model);

    QObject* menu = contextMenu->beginCreate(context);
    if (!menu) {
        delete context;
        return false;
    }
    menu->setParent(context);
    QObject::connect(model, &ContextMenuItems::done, context, &QObject::deleteLater, Qt::QueuedConnection);

    // Can prove useful when not using Qt Quick Controls' Menu
    if (QQuickItem* item = qobject_cast<QQuickItem*>(menu))
        item->setParentItem(q);

    // Now fire Component.onCompleted().
    contextMenu->completeCreate();
    return true;
}

QQuickWebContentsView::QQuickWebContentsView()
    : d_ptr(new QQuickWebContentsViewPrivate)
{
    d_ptr->q_ptr = this;
}

QQuickWebContentsView::~QQuickWebContentsView()
{
}

QUrl QQuickWebContentsView::url() const
{
    Q_D(const QQuickWebContentsView);
    return d->adapter->activeUrl();
}

void QQuickWebContentsView::setUrl(const QUrl& url)
{
    Q_D(QQuickWebContentsView);
    d->adapter->load(url);
}

void QQuickWebContentsView::goBack()
{
    Q_D(QQuickWebContentsView);
    d->adapter->navigateHistory(-1);
}

void QQuickWebContentsView::goForward()
{
    Q_D(QQuickWebContentsView);
    d->adapter->navigateHistory(1);
}

void QQuickWebContentsView::reload()
{
    Q_D(QQuickWebContentsView);
    d->adapter->reload();
}

void QQuickWebContentsView::stop()
{
    Q_D(QQuickWebContentsView);
    d->adapter->stop();
}

bool QQuickWebContentsView::isLoading() const
{
    Q_D(const QQuickWebContentsView);
    return d->adapter->isLoading();
}

QString QQuickWebContentsView::title() const
{
    Q_D(const QQuickWebContentsView);
    return d->adapter->pageTitle();
}

bool QQuickWebContentsView::canGoBack() const
{
    Q_D(const QQuickWebContentsView);
    return d->adapter->canGoBack();
}

bool QQuickWebContentsView::canGoForward() const
{
    Q_D(const QQuickWebContentsView);
    return d->adapter->canGoForward();
}

void QQuickWebContentsView::setContextMenu(QQmlComponent *contextMenu)
{
    Q_D(QQuickWebContentsView);
    if (d->contextMenu == contextMenu)
        return;
    d->contextMenu = contextMenu;
    emit contextMenuChanged();
}

QQmlComponent *QQuickWebContentsView::contextMenu() const
{
    Q_D(const QQuickWebContentsView);
    return d->contextMenu;
}

void QQuickWebContentsView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    Q_FOREACH(QQuickItem *child, childItems()) {
        Q_ASSERT(qobject_cast<RenderWidgetHostViewQtDelegateQuick *>(child));
        child->setSize(newGeometry.size());
    }
}
