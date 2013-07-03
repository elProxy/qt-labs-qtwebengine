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
#include "qwebcontentsview_p.h"

#include "render_widget_host_view_qt_delegate_widget.h"
#include "web_contents_adapter.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QIcon>
#include <QMenu>
#include <QContextMenuEvent>
#include <QStackedLayout>
#include <QUrl>

QWebContentsViewPrivate::QWebContentsViewPrivate()
    : m_isLoading(false)
    , m_pendingContextMenuEvent(false)
    , adapter(new WebContentsAdapter(this))
{
}

void QWebContentsViewPrivate::titleChanged(const QString &title)
{
    Q_EMIT q_ptr->titleChanged(title);
}

void QWebContentsViewPrivate::urlChanged(const QUrl &url)
{
    Q_EMIT q_ptr->urlChanged(url);
}

void QWebContentsViewPrivate::loadingStateChanged()
{
    Q_Q(QWebContentsView);
    const bool wasLoading = m_isLoading;
    m_isLoading = adapter->isLoading();
    if (m_isLoading != wasLoading) {
        if (m_isLoading)
            Q_EMIT q->loadStarted();
    }
}

QRectF QWebContentsViewPrivate::viewportRect() const
{
    Q_Q(const QWebContentsView);
    return q->geometry();
}

void QWebContentsViewPrivate::loadFinished(bool success)
{
    Q_Q(QWebContentsView);
    m_isLoading = adapter->isLoading();
    Q_EMIT q->loadFinished(success);
}

void QWebContentsViewPrivate::focusContainer()
bool QWebContentsViewPrivate::contextMenuRequested(const QContextMenuData &data)
{
    Q_Q(QWebContentsView);
    QContextMenuEvent event(QContextMenuEvent::Mouse, data.pos, q->mapToGlobal(data.pos));
    switch (q->contextMenuPolicy()) {
    case Qt::PreventContextMenu:
        return false;
    case Qt::DefaultContextMenu:
        m_menuData = data;
        q->contextMenuEvent(&event);
        break;
    case Qt::CustomContextMenu:
        Q_EMIT q->customContextMenuRequested(data.pos);
        break;
    case Qt::ActionsContextMenu:
        if (q->actions().count()) {
            QMenu::exec(q->actions(), event.globalPos(), 0, q);
            break;
        }
        // fall through
    default:
        event.ignore();
        return false;
        break;
    }

    Q_ASSERT(m_pendingContextMenuEvent);
    m_pendingContextMenuEvent = false;
    m_menuData = QContextMenuData();
    return true;
}

RenderWidgetHostViewQtDelegate *QWebContentsViewPrivate::CreateRenderWidgetHostViewQtDelegate()
{
    Q_Q(QWebContentsView);
    q->setFocus();
}

RenderWidgetHostViewQtDelegate *QWebContentsViewPrivate::CreateRenderWidgetHostViewQtDelegate()
{
    return new RenderWidgetHostViewQtDelegateWidget;
}

QWebContentsView::QWebContentsView()
    : d_ptr(new QWebContentsViewPrivate)
{
    d_ptr->q_ptr=this;
    // This causes the child RenderWidgetHostViewQtDelegateWidgets to fill this widget.
    setLayout(new QStackedLayout);
}

QWebContentsView::~QWebContentsView()
{
}

void QWebContentsView::load(const QUrl& url)
{
    Q_D(QWebContentsView);
    d->adapter->load(url);
}

bool QWebContentsView::canGoBack() const
{
    Q_D(const QWebContentsView);
    return d->adapter->canGoBack();
}

bool QWebContentsView::canGoForward() const
{
    Q_D(const QWebContentsView);
    return d->adapter->canGoForward();
}

void QWebContentsView::back()
{
    Q_D(QWebContentsView);
    d->adapter->navigateHistory(-1);
}

void QWebContentsView::forward()
{
    Q_D(QWebContentsView);
    d->adapter->navigateHistory(1);
}

void QWebContentsView::reload()
{
    Q_D(QWebContentsView);
    d->adapter->reload();
}

void QWebContentsView::stop()
{
    Q_D(QWebContentsView);
    d->adapter->stop();
}

bool QWebContentsView::event(QEvent *ev)
{
    Q_D(QWebContentsView);
    // We swallow spontaneous contextMenu events and synthethize those back later on when we get the
    // HandleContextMenu callback from chromium
    if (ev->type() == QEvent::ContextMenu) {
        Q_ASSERT(!d->m_pendingContextMenuEvent);
        ev->accept();
        d->m_pendingContextMenuEvent = true;
        return true;
    }
    return QWidget::event(ev);
}

void QWebContentsView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    menu->popup(event->globalPos());
}

QMenu *QWebContentsView::createStandardContextMenu()
{
    Q_D(QWebContentsView);
    QMenu *menu = new QMenu(this);
    QAction *action = 0;
    if (d->m_menuData.selectionText.isEmpty()) {
        action = new QAction(QIcon::fromTheme("go-previous"), tr("&Back"), menu);
        connect(action, &QAction::triggered, this, &QWebContentsView::back);
        action->setEnabled(canGoBack());
        menu->addAction(action);

        action = new QAction(QIcon::fromTheme("go-next"), tr("&Forward"), menu);
        connect(action, &QAction::triggered, this, &QWebContentsView::forward);
        action->setEnabled(canGoForward());
        menu->addAction(action);

        action = new QAction(QIcon::fromTheme("view-refresh"), tr("&Reload"), menu);
        connect(action, &QAction::triggered, this, &QWebContentsView::reload);
        menu->addAction(action);
    } else {
        action = new QAction(QLatin1String("Copy..."), menu);
        // FIXME: We probably can't keep "cheating" with lambdas, but for now it keeps this patch smaller ;)
        connect(action, &QAction::triggered, [=]() { qApp->clipboard()->setText(d->m_menuData.selectionText); });
        menu->addAction(action);
    }

    if (!d->m_menuData.linkText.isEmpty() && d->m_menuData.linkUrl.isValid()) {
        menu->addSeparator();
        action = new QAction(QLatin1String("Navigate to..."), menu);
        connect(action, &QAction::triggered, [=]() { load(d->m_menuData.linkUrl); });
        menu->addAction(action);
        action = new QAction(QLatin1String("Copy link address"), menu);
        connect(action, &QAction::triggered, [=]() { qApp->clipboard()->setText(d->m_menuData.linkUrl.toString()); });
        menu->addAction(action);
    }
    return menu;
}

#include "moc_qwebcontentsview.cpp"
