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

#include "qquickwebcontentsview.h"

#include "content/public/browser/web_contents.h"

#include "browser_context_qt.h"
#include "content_browser_client_qt.h"
#include "web_contents_delegate_qt.h"
#include "web_contents_view_qt.h"
#include "web_engine_context.h"

#include "shared/backing_store_qt.h"
#include "shared/native_view_qt.h"
#include "shared/render_widget_host_view_qt.h"

#include <QUrl>

void QQuickWebContentsView::registerType()
{
    // FIXME: Do a proper plugin.
    qmlRegisterType<QQuickWebContentsView>("QtWebEngine", 1, 0, "WebContentsView");
}

class QQuickWebContentsViewPrivate : public NativeViewQt
{
    QQuickWebContentsView *q_ptr;
    Q_DECLARE_PUBLIC(QQuickWebContentsView)
public:
    QQuickWebContentsViewPrivate();
    virtual void setRenderWidgetHostView(content::RenderWidgetHostViewQt *rwhv);
    virtual void setBackingStore(BackingStoreQt *backingStore);
    virtual QRectF screenRect() const;
    virtual void show();
    virtual void hide();
    virtual bool isVisible() const;
    virtual QWindow *window() const;
    virtual void update(const QRect &rect = QRect());

    scoped_refptr<WebEngineContext> context;
    scoped_ptr<WebContentsDelegateQt> webContentsDelegate;
    content::RenderWidgetHostViewQt *rwhv;
    BackingStoreQt *backingStore;
};

QQuickWebContentsView::QQuickWebContentsView()
    : d_ptr(new QQuickWebContentsViewPrivate)
{
    d_ptr->q_ptr = this;
    setAcceptedMouseButtons(Qt::AllButtons);
}

QQuickWebContentsView::~QQuickWebContentsView()
{
}

QUrl QQuickWebContentsView::url() const
{
    Q_D(const QQuickWebContentsView);
    GURL gurl = d->webContentsDelegate->web_contents()->GetActiveURL();
    return QUrl(QString::fromStdString(gurl.spec()));
}

void QQuickWebContentsView::setUrl(const QUrl& url)
{
    Q_D(QQuickWebContentsView);
    GURL gurl(url.toString().toStdString());

    content::NavigationController::LoadURLParams params(gurl);
    params.transition_type = content::PageTransitionFromInt(content::PAGE_TRANSITION_TYPED | content::PAGE_TRANSITION_FROM_ADDRESS_BAR);
    d->webContentsDelegate->web_contents()->GetController().LoadURLWithParams(params);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QQuickWebContentsView::goBack()
{
    Q_D(QQuickWebContentsView);
    d->webContentsDelegate->web_contents()->GetController().GoToOffset(-1);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QQuickWebContentsView::goForward()
{
    Q_D(QQuickWebContentsView);
    d->webContentsDelegate->web_contents()->GetController().GoToOffset(1);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QQuickWebContentsView::reload()
{
    Q_D(QQuickWebContentsView);
    d->webContentsDelegate->web_contents()->GetController().Reload(false);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QQuickWebContentsView::paint(QPainter *painter)
{
    Q_D(QQuickWebContentsView);
    if (!d->backingStore)
        return;

    d->backingStore->paintToTarget(painter, boundingRect());
}

void QQuickWebContentsView::geometryChanged(const QRectF &newGeometry, const QRectF &)
{
    Q_D(QQuickWebContentsView);
    if (d->backingStore)
        d->backingStore->resize(newGeometry.size().toSize());
    update();
}

void QQuickWebContentsView::focusInEvent(QFocusEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleFocusEvent(event);
}

void QQuickWebContentsView::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleFocusEvent(event);
}

void QQuickWebContentsView::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickWebContentsView);
    forceActiveFocus(Qt::MouseFocusReason);
    d->rwhv->handleMouseEvent(event);
}

void QQuickWebContentsView::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleMouseEvent(event);
}

void QQuickWebContentsView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleMouseEvent(event);
}

void QQuickWebContentsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleMouseEvent(event);
}

void QQuickWebContentsView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleKeyEvent(event);
}

void QQuickWebContentsView::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleKeyEvent(event);
}

void QQuickWebContentsView::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickWebContentsView);
    d->rwhv->handleWheelEvent(event);
}

QQuickWebContentsViewPrivate::QQuickWebContentsViewPrivate()
    // This has to be the first thing we do.
    : context(WebEngineContext::current())
    , rwhv(0)
    , backingStore(0)
{
    content::BrowserContext *browser_context = static_cast<ContentBrowserClientQt *>(content::GetContentClient()->browser())->browser_context();
    webContentsDelegate.reset(WebContentsDelegateQt::CreateNewWindow(this, browser_context, NULL, MSG_ROUTING_NONE, gfx::Size()));
}

void QQuickWebContentsViewPrivate::setRenderWidgetHostView(content::RenderWidgetHostViewQt *rwhv)
{
    this->rwhv = rwhv;
}

void QQuickWebContentsViewPrivate::setBackingStore(BackingStoreQt *backingStore)
{
    Q_Q(QQuickWebContentsView);
    this->backingStore = backingStore;
    if (backingStore)
        backingStore->resize(QSize(q->width(), q->height()));
}

QRectF QQuickWebContentsViewPrivate::screenRect() const
{
    Q_Q(const QQuickWebContentsView);
    return QRectF(q->x(), q->y(), q->width(), q->height());
}

void QQuickWebContentsViewPrivate::show()
{
    Q_Q(QQuickWebContentsView);
    q->setVisible(true);
}

void QQuickWebContentsViewPrivate::hide()
{
    Q_Q(QQuickWebContentsView);
    q->setVisible(true);
}

bool QQuickWebContentsViewPrivate::isVisible() const
{
    Q_Q(const QQuickWebContentsView);
    q->isVisible();
}

QWindow *QQuickWebContentsViewPrivate::window() const
{
    Q_Q(const QQuickWebContentsView);
    q->window();
}

void QQuickWebContentsViewPrivate::update(const QRect &rect)
{
    Q_Q(QQuickWebContentsView);
    q->update(rect);
}
