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

#include "qquickwebengineview_p.h"
#include "qquickwebengineview_p_p.h"

#include "web_contents_adapter.h"
#include "render_widget_host_view_qt_delegate_quick.h"

#include <QAbstractListModel>
#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlProperty>
#include <QScreen>
#include <QStringBuilder>
#include <QUrl>

#include <private/qqmlmetatype_p.h>

// Uncomment for QML debugging
//#define UI_DELEGATES_DEBUG

class MenuItem : public QObject {
Q_OBJECT
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
    inline void setEnabled(bool on) { m_enabled = on; }

Q_SIGNALS:
    void triggered();

private:
    QString m_text;
    QString m_iconName;
    bool m_enabled;
};

class CopyMenuItem : public MenuItem {
    Q_OBJECT
public:
    CopyMenuItem(const QString &itemText, const QString &textToCopy)
        : MenuItem(itemText)
        , m_textToCopy(textToCopy)
    {
        connect(this, &MenuItem::triggered, this, &CopyMenuItem::onTriggered);
    }

private:
    void onTriggered() { qApp->clipboard()->setText(m_textToCopy); }

    QString m_textToCopy;

};

class NavigateMenuItem : public MenuItem {
    Q_OBJECT
public:
    NavigateMenuItem(const QString &itemText, const QExplicitlySharedDataPointer<WebContentsAdapter> &adapter, const QUrl &targetUrl)
        : MenuItem(itemText)
        , m_adapter(adapter)
        , m_targetUrl(targetUrl)
    {
        connect(this, &MenuItem::triggered, this, &NavigateMenuItem::onTriggered);
    }

private:
    void onTriggered() { m_adapter->load(m_targetUrl); }

    QExplicitlySharedDataPointer<WebContentsAdapter> m_adapter;
    QUrl m_targetUrl;

};

#include "qquickwebengineview.moc"

QT_BEGIN_NAMESPACE

QQuickWebEngineViewPrivate::QQuickWebEngineViewPrivate()
    : adapter(new WebContentsAdapter(qApp->property("QQuickWebEngineView_DisableHardwareAcceleration").toBool() ? SoftwareRenderingMode : HardwareAccelerationMode))
    , e(new QQuickWebEngineViewExperimental(this))
    , v(new QQuickWebEngineViewport(this))
    , contextMenuExtraItems(0)
    , menuComponent(0)
    , menuItemComponent(0)
    , menuSeparatorComponent(0)
    , loadProgress(0)
    , inspectable(false)
    , devicePixelRatio(QGuiApplication::primaryScreen()->devicePixelRatio())
    , m_dpiScale(1.0)
{
    // The gold standard for mobile web content is 160 dpi, and the devicePixelRatio expected
    // is the (possibly quantized) ratio of device dpi to 160 dpi.
    // However GUI toolkits on non-iOS platforms may be using different criteria than relative
    // DPI (depending on the history of that platform), dictating the choice of
    // QScreen::devicePixelRatio().
    // Where applicable (i.e. non-iOS mobile platforms), override QScreen::devicePixelRatio
    // and instead use a reasonable default value for viewport.devicePixelRatio to avoid every
    // app having to use this experimental API.
    QString platform = qApp->platformName().toLower();
    if (platform == QStringLiteral("qnx")) {
        qreal webPixelRatio = QGuiApplication::primaryScreen()->physicalDotsPerInch() / 160;

        // Quantize devicePixelRatio to increments of 1 to allow JS and media queries to select
        // 1x, 2x, 3x etc assets that fit an integral number of pixels.
        setDevicePixelRatio(qMax(1, qRound(webPixelRatio)));
    }

    adapter->initialize(this);
}

QQuickWebEngineViewExperimental *QQuickWebEngineViewPrivate::experimental() const
{
    return e.data();
}

QQuickWebEngineViewport *QQuickWebEngineViewPrivate::viewport() const
{
    return v.data();
}

RenderWidgetHostViewQtDelegate *QQuickWebEngineViewPrivate::CreateRenderWidgetHostViewQtDelegate(RenderWidgetHostViewQtDelegateClient *client, RenderingMode mode)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    if (mode == HardwareAccelerationMode)
        return new RenderWidgetHostViewQtDelegateQuick(client);
#endif
    return new RenderWidgetHostViewQtDelegateQuickPainted(client);
}

bool QQuickWebEngineViewPrivate::ensureComponentLoaded(QQmlComponent *&component, const QString& fileName)
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


QQmlContext *QQuickWebEngineViewPrivate::creationContextForComponent(QQmlComponent *component)
{
    Q_ASSERT(component);
    Q_Q(QQuickWebEngineView);

    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = new QQmlContext(qmlEngine(q)->rootContext());
    return baseContext;
}

static void initFromMenuItem(QObject* qmlItem, MenuItem* item) {
    Q_ASSERT(item);
    QQmlProperty prop(qmlItem, QStringLiteral("text"));
    prop.write(item->text());
    prop = QQmlProperty(qmlItem, QStringLiteral("iconName"));
    prop.write(item->iconName());
    prop = QQmlProperty(qmlItem, QStringLiteral("enabled"));
    prop.write(item->enabled());
    prop = QQmlProperty(qmlItem, QStringLiteral("onTriggered"));
    if (!prop.isSignalProperty())
        qWarning("MenuItem is missing onTriggered signal property\n");
    QObject::connect(qmlItem, prop.method(), item, QMetaMethod::fromSignal(&MenuItem::triggered));

}


void QQuickWebEngineViewPrivate::addMenuItem(QObject *menu, MenuItem *menuItem)
{
    Q_Q(QQuickWebEngineView);
    if (!ensureComponentLoaded(menuItemComponent, QStringLiteral("MenuItem.qml")))
        return;

    QObject* it = menuItemComponent->create(creationContextForComponent(menuItemComponent));
    initFromMenuItem(it, menuItem);
    menuItem->setParent(it); // for cleanup purposes

    it->setParent(menu);

    QQmlListReference entries(menu, QQmlMetaType::defaultProperty(menu).name(), qmlEngine(q));
    if (entries.isValid())
        entries.append(it);
}

void QQuickWebEngineViewPrivate::addMenuSeparator(QObject *menu)
{
    Q_Q(QQuickWebEngineView);
    if (!ensureComponentLoaded(menuSeparatorComponent, QStringLiteral("MenuSeparator.qml")))
        return;

    QQmlContext *itemContext = creationContextForComponent(menuSeparatorComponent);
    QObject* sep = menuSeparatorComponent->create(itemContext);
    sep->setParent(menu);

    QQmlListReference entries(menu, QQmlMetaType::defaultProperty(menu).name(), qmlEngine(q));
    if (entries.isValid())
        entries.append(sep);
}

QObject *QQuickWebEngineViewPrivate::addMenu(QObject *parentMenu, const QString &title, const QPoint& pos)
{
    Q_Q(QQuickWebEngineView);
    if (!ensureComponentLoaded(menuComponent, QStringLiteral("Menu.qml")))
        return 0;
    QQmlContext *context(creationContextForComponent(menuComponent));
    QObject *menu = menuComponent->beginCreate(context);
    // Useful when not using Qt Quick Controls' Menu
    if (QQuickItem* item = qobject_cast<QQuickItem*>(menu))
        item->setParentItem(q);
    menuComponent->completeCreate();

    if (!title.isEmpty()) {
        QQmlProperty titleProp(menu, QStringLiteral("title"));
        titleProp.write(title);
    }
    if (!pos.isNull()) {
        QQmlProperty posProp(menu, QStringLiteral("pos"));
        posProp.write(pos);
    }
    if (!parentMenu) {
        QQmlProperty doneSignal(menu, QStringLiteral("onDone"));
        static int deleteLaterIndex = menu->metaObject()->indexOfSlot("deleteLater()");
        if (doneSignal.isSignalProperty())
            QObject::connect(menu, doneSignal.method(), menu, menu->metaObject()->method(deleteLaterIndex));
    } else {
        menu->setParent(parentMenu);

        QQmlListReference entries(parentMenu, QQmlMetaType::defaultProperty(parentMenu).name(), qmlEngine(q));
        if (entries.isValid())
            entries.append(menu);
    }
    return menu;
}

QQmlComponent *QQuickWebEngineViewPrivate::loadDefaultUIDelegate(const QString &fileName)
{
    Q_Q(QQuickWebEngineView);
    QQmlEngine* engine = qmlEngine(q);
    if (!engine)
        return new QQmlComponent(q);
    QString absolutePath;
    Q_FOREACH (const QString &path, engine->importPathList()) {
        QFileInfo fi(path % QStringLiteral("/QtWebEngine/UIDelegates/") % fileName);
        if (fi.exists())
            absolutePath = fi.absoluteFilePath();
    }

    return new QQmlComponent(engine, QUrl(absolutePath), QQmlComponent::PreferSynchronous, q);
}

bool QQuickWebEngineViewPrivate::contextMenuRequested(const WebEngineContextMenuData &data)
{
    Q_Q(QQuickWebEngineView);

    QObject *menu = addMenu(0, QString(), data.pos);
    if (!menu)
        return false;

    // Populate our menu
    MenuItem *item = 0;

    if (data.selectedText.isEmpty()) {
        item = new MenuItem(QObject::tr("Back"), QStringLiteral("go-previous"));
        QObject::connect(item, &MenuItem::triggered, q, &QQuickWebEngineView::goBack);
        item->setEnabled(q->canGoBack());
        addMenuItem(menu, item);

        item = new MenuItem(QObject::tr("Forward"), QStringLiteral("go-next"));
        QObject::connect(item, &MenuItem::triggered, q, &QQuickWebEngineView::goForward);
        item->setEnabled(q->canGoForward());
        addMenuItem(menu, item);

        item = new MenuItem(QObject::tr("Reload"), QStringLiteral("view-refresh"));
        QObject::connect(item, &MenuItem::triggered, q, &QQuickWebEngineView::reload);
        addMenuItem(menu, item);
    } else {
        item = new CopyMenuItem(QObject::tr("Copy..."), data.selectedText);
        addMenuItem(menu, item);
    }

    if (!data.linkText.isEmpty() && data.linkUrl.isValid()) {
        item = new NavigateMenuItem(QObject::tr("Navigate to..."), adapter, data.linkUrl);
        addMenuItem(menu, item);
        item = new CopyMenuItem(QObject::tr("Copy link adress"), data.linkUrl.toString());
        addMenuItem(menu, item);
    }

    if (contextMenuExtraItems) {
        addMenuSeparator(menu);
        if (QObject* menuExtras = contextMenuExtraItems->create(creationContextForComponent(contextMenuExtraItems))) {
            menuExtras->setParent(menu);
            QQmlListReference entries(menu, QQmlMetaType::defaultProperty(menu).name(), qmlEngine(q));
            if (entries.isValid())
                entries.append(menuExtras);
        }
    }

    // Now fire the popup() method on the top level menu
    QMetaObject::invokeMethod(menu, "popup");
    return true;
}


void QQuickWebEngineViewPrivate::titleChanged(const QString &title)
{
    Q_Q(QQuickWebEngineView);
    Q_UNUSED(title);
    Q_EMIT q->titleChanged();
}

void QQuickWebEngineViewPrivate::urlChanged(const QUrl &url)
{
    Q_Q(QQuickWebEngineView);
    Q_UNUSED(url);
    Q_EMIT q->urlChanged();
}

void QQuickWebEngineViewPrivate::iconChanged(const QUrl &url)
{
    Q_Q(QQuickWebEngineView);
    icon = url;
    Q_EMIT q->iconChanged();
}

void QQuickWebEngineViewPrivate::loadingStateChanged()
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->loadingStateChanged();
}

void QQuickWebEngineViewPrivate::loadProgressChanged(int progress)
{
    Q_Q(QQuickWebEngineView);
    loadProgress = progress;
    Q_EMIT q->loadProgressChanged();
}

QRectF QQuickWebEngineViewPrivate::viewportRect() const
{
    Q_Q(const QQuickWebEngineView);
    return QRectF(q->x(), q->y(), q->width(), q->height());
}

qreal QQuickWebEngineViewPrivate::dpiScale() const
{
    return m_dpiScale;
}

void QQuickWebEngineViewPrivate::loadFinished(bool success)
{
    Q_Q(QQuickWebEngineView);
    Q_UNUSED(success);
    Q_EMIT q->loadingStateChanged();
}

void QQuickWebEngineViewPrivate::focusContainer()
{
    Q_Q(QQuickWebEngineView);
    q->forceActiveFocus();
}

void QQuickWebEngineViewPrivate::adoptNewWindow(WebContentsAdapter *newWebContents, WindowOpenDisposition disposition, const QRect &)
{
    Q_UNUSED(newWebContents);
    Q_UNUSED(disposition);
    Q_UNREACHABLE();
}

void QQuickWebEngineViewPrivate::close()
{
    Q_UNREACHABLE();
}

void QQuickWebEngineViewPrivate::setDevicePixelRatio(qreal devicePixelRatio)
{
    this->devicePixelRatio = devicePixelRatio;
    QScreen *screen = window ? window->screen() : QGuiApplication::primaryScreen();
    m_dpiScale = devicePixelRatio / screen->devicePixelRatio();
}

QQuickWebEngineView::QQuickWebEngineView(QQuickItem *parent)
    : QQuickItem(*(new QQuickWebEngineViewPrivate), parent)
{
    Q_D(const QQuickWebEngineView);
    d->e->q_ptr = this;
}

QQuickWebEngineView::~QQuickWebEngineView()
{
}

QUrl QQuickWebEngineView::url() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->activeUrl();
}

void QQuickWebEngineView::setUrl(const QUrl& url)
{
    Q_D(QQuickWebEngineView);
    d->adapter->load(url);
}

QUrl QQuickWebEngineView::icon() const
{
    Q_D(const QQuickWebEngineView);
    return d->icon;
}

void QQuickWebEngineView::goBack()
{
    Q_D(QQuickWebEngineView);
    d->adapter->navigateToOffset(-1);
}

void QQuickWebEngineView::goForward()
{
    Q_D(QQuickWebEngineView);
    d->adapter->navigateToOffset(1);
}

void QQuickWebEngineView::reload()
{
    Q_D(QQuickWebEngineView);
    d->adapter->reload();
}

void QQuickWebEngineView::stop()
{
    Q_D(QQuickWebEngineView);
    d->adapter->stop();
}

bool QQuickWebEngineView::isLoading() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->isLoading();
}

int QQuickWebEngineView::loadProgress() const
{
    Q_D(const QQuickWebEngineView);
    return d->loadProgress;
}

QString QQuickWebEngineView::title() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->pageTitle();
}

bool QQuickWebEngineView::canGoBack() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->canGoBack();
}

bool QQuickWebEngineView::canGoForward() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->canGoForward();
}

bool QQuickWebEngineView::inspectable() const
{
    Q_D(const QQuickWebEngineView);
    return d->inspectable;
}

void QQuickWebEngineView::setInspectable(bool enable)
{
    Q_D(QQuickWebEngineView);
    d->inspectable = enable;
    d->adapter->enableInspector(enable);
}

void QQuickWebEngineViewExperimental::setContextMenuExtraItems(QQmlComponent *contextMenuExtras)
{
    if (d_ptr->contextMenuExtraItems == contextMenuExtras)
        return;
    d_ptr->contextMenuExtraItems = contextMenuExtras;
    emit contextMenuExtraItemsChanged();
}

QQmlComponent *QQuickWebEngineViewExperimental::contextMenuExtraItems() const
{
    return d_ptr->contextMenuExtraItems;
}

void QQuickWebEngineView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    Q_FOREACH(QQuickItem *child, childItems()) {
        Q_ASSERT(
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
            qobject_cast<RenderWidgetHostViewQtDelegateQuick *>(child) ||
#endif
            qobject_cast<RenderWidgetHostViewQtDelegateQuickPainted *>(child));
        child->setSize(newGeometry.size());
    }
}

QQuickWebEngineViewExperimental::QQuickWebEngineViewExperimental(QQuickWebEngineViewPrivate *viewPrivate)
    : q_ptr(0)
    , d_ptr(viewPrivate)
{
}

QQuickWebEngineViewport *QQuickWebEngineViewExperimental::viewport() const
{
    Q_D(const QQuickWebEngineView);
    return d->viewport();
}

QQuickWebEngineViewport::QQuickWebEngineViewport(QQuickWebEngineViewPrivate *viewPrivate)
    : d_ptr(viewPrivate)
{
}

qreal QQuickWebEngineViewport::devicePixelRatio() const
{
    Q_D(const QQuickWebEngineView);
    return d->devicePixelRatio;
}

void QQuickWebEngineViewport::setDevicePixelRatio(qreal devicePixelRatio)
{
    Q_D(QQuickWebEngineView);
    // Valid range is [1, inf)
    devicePixelRatio = qMax(1.0, devicePixelRatio);
    if (d->devicePixelRatio == devicePixelRatio)
        return;
    d->setDevicePixelRatio(devicePixelRatio);
    d->adapter->dpiScaleChanged();
    Q_EMIT devicePixelRatioChanged();
}

QT_END_NAMESPACE
