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

#include "render_widget_host_view_qt_delegate_quick.h"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
RenderWidgetHostViewQtDelegateQuick::RenderWidgetHostViewQtDelegateQuick(RenderWidgetHostViewQtDelegateClient *client, QQuickItem *parent)
    : RenderWidgetHostViewQtDelegateQuickBase<QQuickItem>(client, parent)
{
    setFlag(ItemHasContents);
}

WId RenderWidgetHostViewQtDelegateQuick::nativeWindowIdForCompositor() const
{
    return QQuickItem::window() ? QQuickItem::window()->winId() : 0;
}

void RenderWidgetHostViewQtDelegateQuick::update(const QRect&)
{
    QQuickItem::update();
}

void RenderWidgetHostViewQtDelegateQuick::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
    if (change == QQuickItem::ItemSceneChange && value.window)
        m_client->compositingSurfaceUpdated();
}

QSGNode *RenderWidgetHostViewQtDelegateQuick::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    return m_client->updatePaintNode(oldNode, QQuickItem::window());
}
#endif // QT_VERSION

RenderWidgetHostViewQtDelegateQuickPainted::RenderWidgetHostViewQtDelegateQuickPainted(RenderWidgetHostViewQtDelegateClient *client, QQuickItem *parent)
    : RenderWidgetHostViewQtDelegateQuickBase<QQuickPaintedItem>(client, parent)
{
}

WId RenderWidgetHostViewQtDelegateQuickPainted::nativeWindowIdForCompositor() const
{
    // This causes a failure of the compositor initialization which ends up disabling it completely.
    return 0;
}

void RenderWidgetHostViewQtDelegateQuickPainted::update(const QRect& rect)
{
    polish();
    QQuickPaintedItem::update(rect);
}

void RenderWidgetHostViewQtDelegateQuickPainted::paint(QPainter *painter)
{
    m_client->paint(painter, boundingRect());
}

void RenderWidgetHostViewQtDelegateQuickPainted::updatePolish()
{
    // paint will be called from the scene graph thread and this doesn't play well
    // with chromium's use of TLS while getting the backing store.
    // updatePolish() should be called from the GUI thread right before the rendering thread starts.
    m_client->fetchBackingStore();
}
