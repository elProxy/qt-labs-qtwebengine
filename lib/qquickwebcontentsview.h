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

#ifndef QQUICKWEBCONTESTSVIEW_H
#define QQUICKWEBCONTESTSVIEW_H

#include <QQuickPaintedItem>
#include <QScopedPointer>

class QQuickWebContentsViewPrivate;

class Q_DECL_EXPORT QQuickWebContentsView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
public:
    static void registerType();

    QQuickWebContentsView();
    ~QQuickWebContentsView();

    QUrl url() const;
    void setUrl(const QUrl&);

public Q_SLOTS:
    void goBack();
    void goForward();
    void reload();

Q_SIGNALS:
    void titleChanged();
    void urlChanged();

protected:
    virtual void paint(QPainter *painter);
    virtual void geometryChanged(const QRectF &, const QRectF &);
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual void wheelEvent(QWheelEvent *);

private:
    Q_DECLARE_PRIVATE(QQuickWebContentsView)
    // Hides QObject::d_ptr allowing us to use the convenience macros.
    QScopedPointer<QQuickWebContentsViewPrivate> d_ptr;
};

QML_DECLARE_TYPE(QQuickWebContentsView)

#endif // QQUICKWEBCONTESTSVIEW_H
