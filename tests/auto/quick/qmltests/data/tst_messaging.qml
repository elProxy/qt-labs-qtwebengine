/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Milian Wolff <milian.wolff@kdab.com>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWebChannel module of the Qt Toolkit.
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

import QtQuick 2.0
import QtTest 1.0
import QtWebEngine 1.0

Item {
    TestWebEngineView {
        id: webView
        property variant lastMessage
        experimental.onMessageReceived: {
            lastMessage = message
        }
    }

    TestWebEngineView {
        id: otherWebView
        property variant lastMessage
        experimental.onMessageReceived: {
            lastMessage = message
        }
    }

    TestWebEngineView {
        id: disabledWebView
        property bool receivedMessage
        experimental.onMessageReceived: {
            receivedMessage = true
        }
    }

    SignalSpy {
        id: messageSpy
        target: webView.experimental
        signalName: "messageReceived"
    }

    SignalSpy {
        id: otherMessageSpy
        target: otherWebView.experimental
        signalName: "messageReceived"
    }

    TestCase {
        name: "WebViewMessaging"
        property url testUrl: Qt.resolvedUrl("messaging.html")

        function init() {
            messageSpy.clear()
            webView.lastMessage = null
            otherMessageSpy.clear()
            otherWebView.lastMessage = null
        }

        function test_basic() {
            webView.url = testUrl
            verify(webView.waitForLoadSucceeded())
            webView.experimental.postMessage("HELLO")
            messageSpy.wait()
            console.log(webView.lastMessage);
            compare(webView.lastMessage.data, "OLLEH")
            compare(webView.lastMessage.origin.toString(), testUrl.toString())
        }

        function test_twoWebViews() {
            webView.url = testUrl
            otherWebView.url = testUrl
            verify(webView.waitForLoadSucceeded())
            verify(otherWebView.waitForLoadSucceeded())
            webView.experimental.postMessage("FIRST")
            otherWebView.experimental.postMessage("SECOND")
            messageSpy.wait()
            otherMessageSpy.wait()
            compare(webView.lastMessage.data, "TSRIF")
            compare(otherWebView.lastMessage.data, "DNOCES")
        }
    }
}
