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

    ContextMenuItems(QQuickWebContentsView*, const QWebContextMenuData&);
    virtual int rowCount(const QModelIndex& = QModelIndex()) const { return m_items.size(); }
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames() const;
    inline QPointF pos() const { return m_pos; }
    inline int count() const { return m_items.count(); }

    Q_INVOKABLE void accept(int);
    Q_INVOKABLE void reject() { emit done(); }

Q_SIGNALS:
    void done();
    
private:
    QList<MenuItem*> m_items;
    QPointF m_pos;
};

ContextMenuItems::ContextMenuItems(QQuickWebContentsView *view, const QWebContextMenuData &data)
    : m_pos(data.pos)
{
    MenuItem *item = 0;

    item = new MenuItem(tr("Back"), this);
    connect(item, &MenuItem::triggered, view, &QQuickWebContentsView::goBack);
    item->setEnabled(view->canGoBack());
    m_items.append(item);

    item = new MenuItem(tr("Forward"), this);
    connect(item, &MenuItem::triggered, view, &QQuickWebContentsView::goForward);
    item->setEnabled(view->canGoForward());
    m_items.append(item);

    item = new MenuItem((view->isLoading()? tr("Stop") : tr("Reload")), this);
    if (view->isLoading())
        connect(item, &MenuItem::triggered, view, &QQuickWebContentsView::stop);
    else
        connect(item, &MenuItem::triggered, view, &QQuickWebContentsView::reload);
    m_items.append(item);

}

QVariant ContextMenuItems::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    MenuItem* item = m_items.at(index.row());
    if (item->type() == MenuItem::Separator) {
        if (role == SeparatorRole)
            return true;
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return item->text();
    case EnabledRole:
        return item->enabled();
    case SeparatorRole:
        return false;
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

void ContextMenuItems::accept(int)
{
    Q_EMIT done();
    deleteLater();
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

    QByteArray filename = qgetenv("QTWEBENGINE_CONTEXTMENU");
    if (!filename.isEmpty())
        contextMenu = new QQmlComponent(qmlEngine(q), QUrl(filename), QQmlComponent::PreferSynchronous, q);
    fprintf(stderr, "QQuickWebContentsViewPrivate::contextMenuRequested  %d\n", contextMenu->status());
    Q_FOREACH (const QQmlError& err, contextMenu->errors())
        fprintf(stderr, "  error: %s\n", qPrintable(err.toString()));

    if (!contextMenu || contextMenu->status() != QQmlComponent::Ready)
        return false;

    QQmlContext* context(createContextForComponent(contextMenu));

    ContextMenuItems *model = new ContextMenuItems(q, data);
    model->setParent(context);
    context->setContextProperty(QLatin1String("model"), model);
    context->setContextObject(model);

    QQuickItem* menu = qobject_cast<QQuickItem*>(contextMenu->beginCreate(context));
    fprintf(stderr, "Foooooo  %p\n", menu);
    if (!menu)
        return false;

    QObject::connect(model, &ContextMenuItems::done, menu, &QQuickItem::deleteLater);
    QObject::connect(model, &ContextMenuItems::done, context, &QQmlContext::deleteLater);
    menu->setParentItem(q);

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

void QQuickWebContentsView::componentComplete()
{
    Q_D(QQuickWebContentsView);
    QQuickItem::componentComplete();
    QQmlEngine* engine = qmlEngine(this);
    if (!engine)
        return;
    d->contextMenu = new QQmlComponent(engine, QUrl("qrc:/qml/contextmenu.qml"), QQmlComponent::Asynchronous, this);
}
