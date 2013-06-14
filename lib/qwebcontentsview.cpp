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

#include "qwebcontentsview.h"

// Needed to get access to content::GetContentClient()
#define CONTENT_IMPLEMENTATION

#include "content/public/browser/web_contents.h"

#include "browser_context_qt.h"
#include "content_browser_client_qt.h"
#include "web_contents_delegate_qt.h"
#include "web_contents_view_qt.h"
#include "web_engine_context.h"

#include "shared/backing_store_qt.h"
#include "shared/native_view_qt.h"
#include "shared/render_widget_host_view_qt.h"

#include <QPaintEvent>
#include <QUrl>

class QWebContentsViewPrivate : public NativeViewQt
{
    QWebContentsView *q_ptr;
    Q_DECLARE_PUBLIC(QWebContentsView)
public:
    QWebContentsViewPrivate();
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

QWebContentsView::QWebContentsView()
    : d_ptr(new QWebContentsViewPrivate)
{
    d_ptr->q_ptr = this;
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

QWebContentsView::~QWebContentsView()
{
}

void QWebContentsView::load(const QUrl& url)
{
    Q_D(QWebContentsView);
    QString urlString = url.toString();
    GURL gurl(urlString.toStdString());
    if (!gurl.has_scheme())
        gurl = GURL(std::string("http://") + urlString.toStdString());

    content::NavigationController::LoadURLParams params(gurl);
    params.transition_type = content::PageTransitionFromInt(content::PAGE_TRANSITION_TYPED | content::PAGE_TRANSITION_FROM_ADDRESS_BAR);
    d->webContentsDelegate->web_contents()->GetController().LoadURLWithParams(params);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QWebContentsView::back()
{
    Q_D(QWebContentsView);
    d->webContentsDelegate->web_contents()->GetController().GoToOffset(-1);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QWebContentsView::forward()
{
    Q_D(QWebContentsView);
    d->webContentsDelegate->web_contents()->GetController().GoToOffset(1);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QWebContentsView::reload()
{
    Q_D(QWebContentsView);
    d->webContentsDelegate->web_contents()->GetController().Reload(false);
    d->webContentsDelegate->web_contents()->GetView()->Focus();
}

void QWebContentsView::paintEvent(QPaintEvent *event)
{
    Q_D(QWebContentsView);
    if (!d->backingStore)
        return;
    QPainter painter(this);
    d->backingStore->paintToTarget(&painter, event->rect());
}

void QWebContentsView::resizeEvent(QResizeEvent *resizeEvent)
{
    Q_D(QWebContentsView);
    if (d->backingStore)
        d->backingStore->resize(resizeEvent->size());
    update();
}

bool QWebContentsView::event(QEvent *event)
{
    Q_D(QWebContentsView);
    if (!d->rwhv || !d->rwhv->handleEvent(event))
        return QWidget::event(event);
    return true;
}

QWebContentsViewPrivate::QWebContentsViewPrivate()
    // This has to be the first thing we do.
    : context(WebEngineContext::current())
    , rwhv(0)
    , backingStore(0)
{
    content::BrowserContext *browser_context = static_cast<ContentBrowserClientQt *>(content::GetContentClient()->browser())->browser_context();
    webContentsDelegate.reset(WebContentsDelegateQt::CreateNewWindow(this, browser_context, NULL, MSG_ROUTING_NONE, gfx::Size()));
}

void QWebContentsViewPrivate::setRenderWidgetHostView(content::RenderWidgetHostViewQt *rwhv)
{
    this->rwhv = rwhv;
}

void QWebContentsViewPrivate::setBackingStore(BackingStoreQt *backingStore)
{
    Q_Q(QWebContentsView);
    this->backingStore = backingStore;
    if (backingStore)
        backingStore->resize(q->size());
}

QRectF QWebContentsViewPrivate::screenRect() const
{
    Q_Q(const QWebContentsView);
    return QRectF(q->x(), q->y(), q->width(), q->height());
}

void QWebContentsViewPrivate::show()
{
    Q_Q(QWebContentsView);
    q->show();
}

void QWebContentsViewPrivate::hide()
{
    Q_Q(QWebContentsView);
    q->hide();
}

bool QWebContentsViewPrivate::isVisible() const
{
    Q_Q(const QWebContentsView);
    q->isVisible();
}

QWindow*QWebContentsViewPrivate:: window() const
{
    Q_Q(const QWebContentsView);
    q->windowHandle();
}

void QWebContentsViewPrivate::update(const QRect &rect)
{
    Q_Q(QWebContentsView);
    q->update(rect);
}
