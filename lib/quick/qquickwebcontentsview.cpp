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
#define UI_DELEGATES_DEBUG

class ContextMenuItems;

class MenuItem : public QObject {
Q_OBJECT
    Q_PROPERTY(QString text READ text CONSTANT FINAL)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
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
    void enabledChanged();

public Q_SLOTS:
    inline void setEnabled(bool on) { m_enabled = on; }

private:
    QString m_text;
    QString m_iconName;
    bool m_enabled;
};

/*
class ContextMenuItems : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos CONSTANT FINAL)
    Q_PROPERTY(int count READ count CONSTANT FINAL)

public:
    enum Roles {
        EnabledRole = Qt::UserRole,
        SeparatorRole,
        SubMenuRole
    };

    ContextMenuItems(const QPointF &pos = QPointF()) : m_pos(pos) { }
    virtual int rowCount(const QModelIndex& = QModelIndex()) const { return m_items.size(); }
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames() const;
    inline QPointF pos() const { return m_pos; }
    inline int count() const { return m_items.count(); }
    inline void addItem(MenuItem* item) { m_items.append(item); item->setParent(this); }
    ContextMenuItems* addMenu(const QString& title)
    {
        MenuItem* item = new MenuItem(title, this, MenuItem::Menu);
        m_items.append(item);
        item->m_menu = new ContextMenuItems;
        return item->m_menu;
    }

    Q_INVOKABLE void accept(int);

Q_SIGNALS:
    void done();

private:
    QList<MenuItem*> m_items;
    QPointF m_pos;
};

ContextMenuItems *MenuItem::menu() const
{
    Q_ASSERT(m_type == Menu);
    return m_menu;
}

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
    case SubMenuRole:
        return QVariant::fromValue(item->menu());
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
        roles.insert(SubMenuRole, QByteArray("subMenu"));
    }
    return roles;
}

void ContextMenuItems::accept(int idx)
{
    if (idx >= 0 && idx < m_items.count())
        Q_EMIT m_items.at(idx)->triggered();
}

*/

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

QQmlContext *QQuickWebContentsViewPrivate::creationContextForComponent(QQmlComponent *component)
{
    Q_ASSERT(component);
    Q_Q(QQuickWebContentsView);

    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = new QQmlContext(qmlEngine(q)->rootContext());
    return baseContext;
}

void QQuickWebContentsViewPrivate::addMenuItem(QObject *menuInstance, QQmlComponent *menuItemComponent, QObject *menuItem)
{
    Q_Q(QQuickWebContentsView);
    if (!menuItemComponent || menuItemComponent->status() != QQmlComponent::Ready) {
#ifdef UI_DELEGATES_DEBUG
        if (menuItemComponent) {
            Q_FOREACH (const QQmlError& err, menuItemComponent->errors())
                fprintf(stderr, "  component error: %s\n", qPrintable(err.toString()));
        }
#endif
    }

    QQmlContext *itemContext = creationContextForComponent(menuItemComponent);
    QObject* it = menuItemComponent->beginCreate(itemContext);
    itemContext->setContextProperty(QLatin1String("item"), menuItem);
    menuItemComponent->completeCreate();
    it->setParent(menuInstance);

    QQmlListReference entries(menuInstance, QQmlMetaType::defaultProperty(menuInstance).name(), qmlEngine(q));
    if (entries.isValid())
        entries.append(it);
}

QQmlComponent *QQuickWebContentsViewPrivate::loadDefaultUIDelegate(const QString &fileName)
{
    Q_Q(QQuickWebContentsView);
    QQmlEngine* engine = qmlEngine(q);
    if (!engine)
        return 0;
    QString absolutePath;
    Q_FOREACH (const QString &path, engine->importPathList()) {
        QFileInfo fi(path + QStringLiteral("/QtWebEngine/UIDelegates/") + fileName);
        if (fi.exists())
            absolutePath = fi.absoluteFilePath();
    }
#ifdef UI_DELEGATES_DEBUG
    qDebug() << engine->importPathList() << absolutePath;
#endif

    return new QQmlComponent(engine, QUrl(absolutePath), QQmlComponent::PreferSynchronous, q);
}

bool QQuickWebContentsViewPrivate::contextMenuRequested(const QWebContextMenuData &data)
{
    Q_Q(QQuickWebContentsView);

#ifndef UI_DELEGATES_DEBUG
    if (!contextMenu)
#endif
        contextMenu = loadDefaultUIDelegate(QStringLiteral("WebContextMenu.qml"));

    if (!contextMenu || contextMenu->status() != QQmlComponent::Ready) {
#ifdef UI_DELEGATES_DEBUG
        if (contextMenu) {
            Q_FOREACH (const QQmlError& err, contextMenu->errors())
                fprintf(stderr, "  QML error: %s\n", qPrintable(err.toString()));
        }
#endif
        return false;
    }

    QQmlComponent *menuComp = loadDefaultUIDelegate(QStringLiteral("WebMenu.qml"));
    QQmlComponent *menuItemComp = loadDefaultUIDelegate(QStringLiteral("WebMenuItem.qml"));
    QQmlComponent *menuSeparator = loadDefaultUIDelegate(QStringLiteral("WebMenuSeparator.qml"));


    QQmlContext* context(creationContextForComponent(contextMenu));

//    QObject* menuContents = contextMenu->beginCreate(context);
//    if (!menuContents) {
//        delete context;
//        return false;
//    }

    QObject* menu = menuComp->beginCreate(context);
    if (!menu) {
//        delete menuContents;
        return false;
    }

    // TODO: get prepended items from MenuContents  ----------------------------------------------------------

    // Populate our menu
    MenuItem *item = 0;

    item = new MenuItem(QObject::tr("Back"), QStringLiteral("go-previous"));
    QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::goBack);
    item->setEnabled(q->canGoBack());
    addMenuItem(menu, menuItemComp, item);


    item = new MenuItem(QObject::tr("Forward"), QStringLiteral("go-next"));
    QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::goForward);
    item->setEnabled(q->canGoForward());
    addMenuItem(menu, menuItemComp, item);



    item = new MenuItem((q->isLoading()? QObject::tr("Stop") : QObject::tr("Reload")), (q->isLoading()? QString() : QStringLiteral("view-refresh")));
    if (q->isLoading())
        QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::stop);
    else
        QObject::connect(item, &MenuItem::triggered, q, &QQuickWebContentsView::reload);
    addMenuItem(menu, menuItemComp, item);

//    ContextMenuItems* sub = model->addMenu("Foobar");
//    sub->addItem(new MenuItem("FooFoo"));
//    sub->addItem(new MenuItem(MenuItem::Separator));
//    sub->addItem(new MenuItem("barbar"));

    // FIXME: Don't leak ==========================================================
//    menu->setParent(context);
//    QObject::connect(model, &ContextMenuItems::done, context, &QObject::deleteLater, Qt::QueuedConnection);

    // Can prove useful when not using Qt Quick Controls' Menu
    if (QQuickItem* item = qobject_cast<QQuickItem*>(menu))
        item->setParentItem(q);

    // Now fire Component.onCompleted().
    menuComp->completeCreate();
//    contextMenu->completeCreate();
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
