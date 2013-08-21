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
#include <QFileInfo>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QUrl>

#include <private/qqmlmetatype_p.h>

// Uncomment for QML debugging
//#define UI_DELEGATES_DEBUG

class ContextMenuItems;

class MenuItem : public QObject {
Q_OBJECT
    Q_PROPERTY(QString text READ text CONSTANT FINAL)
    Q_PROPERTY(bool enabled READ enabled CONSTANT FINAL)
    Q_PROPERTY(QString iconName READ iconName CONSTANT FINAL)

public:

    MenuItem(const QString& text, const QString iconName = QString(), QObject *parent = 0)
        : QObject(parent)
        , m_text(text)
        , m_iconName(iconName)
        , m_enabled(true)
    {
    }


    inline QString text() const { return m_text; }
    inline QString iconName() const { return m_iconName; }
    inline bool enabled() const { return m_enabled; }

Q_SIGNALS:
    void triggered();

public Q_SLOTS:
    inline void setEnabled(bool on) { m_enabled = on; }

private:
    QString m_text;
    QString m_iconName;
    bool m_enabled;
};


#include "qquickwebcontentsview.moc"


QQuickWebContentsViewPrivate::QQuickWebContentsViewPrivate()
    : adapter(new WebContentsAdapter(this))
    , contextMenuExtraItems(0)
    , menuComponent(0)
    , menuItemComponent(0)
    , menuSeparatorComponent(0)
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

bool QQuickWebContentsViewPrivate::ensureComponentLoaded(QQmlComponent *&component, const QString& fileName)
{
#ifndef UI_DELEGATES_DEBUG
    if (component)
        return true;
#endif
    component = loadDefaultUIDelegate(fileName);

    if (component->status() != QQmlComponent::Ready) {
#ifdef UI_DELEGATES_DEBUG
        Q_FOREACH (const QQmlError& err, component->errors())
            fprintf(stderr, "  component error: %s\n", qPrintable(err.toString()));
#endif
        return false;
    }
    return true;
}


QQmlContext *QQuickWebContentsViewPrivate::creationContextForComponent(QQmlComponent *component)
{
    Q_ASSERT(component);
    Q_Q(QQuickWebContentsView);

    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = new QQmlContext(qmlEngine(q)->rootContext());
    return baseContext;
}

void QQuickWebContentsViewPrivate::addMenuItem(QObject *menu, MenuItem *menuItem)
{
    Q_Q(QQuickWebContentsView);
    if (!ensureComponentLoaded(menuItemComponent, QStringLiteral("WebMenuItem.qml")))
        return;

    QQmlContext *itemContext = creationContextForComponent(menuItemComponent);
    itemContext->setContextProperty(QLatin1String("item"), menuItem);
    QObject* it = menuItemComponent->create(itemContext);
    it->setParent(menu);

    QQmlListReference entries(menu, QQmlMetaType::defaultProperty(menu).name(), qmlEngine(q));
    if (entries.isValid())
        entries.append(it);
}

void QQuickWebContentsViewPrivate::addMenuSeparator(QObject *menu)
{
    Q_Q(QQuickWebContentsView);
    if (!ensureComponentLoaded(menuSeparatorComponent, QStringLiteral("WebMenuItem.qml")))
        return;

    QQmlContext *itemContext = creationContextForComponent(menuSeparatorComponent);
    QObject* sep = menuSeparatorComponent->create(itemContext);
    sep->setParent(menu);

    QQmlListReference entries(menu, QQmlMetaType::defaultProperty(menu).name(), qmlEngine(q));
    if (entries.isValid())
        entries.append(sep);
}

QObject *QQuickWebContentsViewPrivate::addMenu(QObject *parentMenu, const QString &title, const QPoint& pos)
{
    Q_Q(QQuickWebContentsView);
    if (!ensureComponentLoaded(menuComponent, QStringLiteral("WebMenu.qml")))
        return 0;
    QQmlContext *context(creationContextForComponent(menuComponent));
    QObject *sub = menuComponent->beginCreate(context);
    context->setContextProperty(QLatin1String("menuTitle"), title);
    context->setContextProperty(QLatin1String("pos"), pos);
    menuComponent->completeCreate();

    if (parentMenu) {
        sub->setParent(parentMenu);

        QQmlListReference entries(parentMenu, QQmlMetaType::defaultProperty(parentMenu).name(), qmlEngine(q));
        if (entries.isValid())
            entries.append(sub);
    }
    return sub;
}

QQmlComponent *QQuickWebContentsViewPrivate::loadDefaultUIDelegate(const QString &fileName)
{
    Q_Q(QQuickWebContentsView);
    QQmlEngine* engine = qmlEngine(q);
    if (!engine)
        return new QQmlComponent(q);
    QString absolutePath;
    Q_FOREACH (const QString &path, engine->importPathList()) {
        QFileInfo fi(path + QStringLiteral("/QtWebEngine/UIDelegates/") + fileName);
        if (fi.exists())
            absolutePath = fi.absoluteFilePath();
    }

    return new QQmlComponent(engine, QUrl(absolutePath), QQmlComponent::PreferSynchronous, q);
}

bool QQuickWebContentsViewPrivate::contextMenuRequested(const QWebContextMenuData &data)
{
    Q_Q(QQuickWebContentsView);

    QObject *menu = addMenu(0, QString(), data.pos);
    if (!menu)
        return false;

    // Populate our menu
    MenuItem *item = 0;

    item = new MenuItem(QObject::tr("Back"), QStringLiteral("go-previous"));
    QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::goBack);
    item->setEnabled(q->canGoBack());
    addMenuItem(menu, item);


    item = new MenuItem(QObject::tr("Forward"), QStringLiteral("go-next"));
    QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::goForward);
    item->setEnabled(q->canGoForward());
    addMenuItem(menu, item);

    item = new MenuItem(QObject::tr("Reload"), QStringLiteral("view-refresh"));
    QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::reload);
    addMenuItem(menu, item);

    QObject* sub = addMenu(menu, QLatin1String("Foobar"));
    addMenuItem(sub, new MenuItem("FooFoo"));
    addMenuItem(sub, new MenuItem("barbar winter bok"));

    if (contextMenuExtraItems) {
        if (QObject* menuExtras = contextMenuExtraItems->create(creationContextForComponent(contextMenuExtraItems))) {
            addMenuSeparator(menu);
            menuExtras->setParent(menu);
            QQmlListReference entries(menu, QQmlMetaType::defaultProperty(menu).name(), qmlEngine(q));
            if (entries.isValid())
                entries.append(menuExtras);
        }
    }


    // FIXME: Don't leak the menu... ==========================================================
//    QObject::connect(model, &ContextMenuItems::done, context, &QObject::deleteLater, Qt::QueuedConnection);

    // Useful when not using Qt Quick Controls' Menu
    if (QQuickItem* item = qobject_cast<QQuickItem*>(menu))
        item->setParentItem(q);

    // Now fire the popup() method on the top level menu
    QMetaObject::invokeMethod(menu, "popup");
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

void QQuickWebContentsView::setContextMenuExtraItems(QQmlComponent *contextMenu)
{
    Q_D(QQuickWebContentsView);
    if (d->contextMenuExtraItems == contextMenu)
        return;
    d->contextMenuExtraItems = contextMenu;
    emit contextMenuExtraItemsChanged();
}

QQmlComponent *QQuickWebContentsView::contextMenuExtraItems() const
{
    Q_D(const QQuickWebContentsView);
    return d->contextMenuExtraItems;
}

void QQuickWebContentsView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    Q_FOREACH(QQuickItem *child, childItems()) {
        Q_ASSERT(qobject_cast<RenderWidgetHostViewQtDelegateQuick *>(child));
        child->setSize(newGeometry.size());
    }
}
