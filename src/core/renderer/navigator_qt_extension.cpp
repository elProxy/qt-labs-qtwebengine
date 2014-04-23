/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "navigator_qt_extension.h"

#include <QtCore/qcompilerdetection.h>

#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "v8/include/v8.h"

#include <QDebug>

static const char kNavigatorQtExtensionName[] = "v8/NavigatorQt";

static const char* const kNavigatorQtApi =
        "if (typeof(navigator) == 'undefined')"
        "  navigator = {};"
        "if (typeof(navigator.qt) == 'undefined')"
        "  navigator.qt = {};"
        "navigator.qt.postMessage = function(message) {"
        "  native function NativeQtPostMessage();"
        "  NativeQtPostMessage(message);"
        "};";

// FIXME: we probably want to put all of QWebChannel's JS into the extension for maximal gains.


class NavigatorQtExtensionWrapper : public v8::Extension {

public:
    static content::RenderView *GetRenderView();

    NavigatorQtExtensionWrapper() : v8::Extension(kNavigatorQtExtensionName, kNavigatorQtApi)
    {
    }

    virtual v8::Handle<v8::FunctionTemplate> GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Handle<v8::String> name) Q_DECL_OVERRIDE;

    static void NativeQtPostMessage(const v8::FunctionCallbackInfo<v8::Value>& args) {
        qDebug() << Q_FUNC_INFO << "called with" << args.Length() << "arguments." << args[0]->IsString();
        if (args.Length() != 1 || !args[0]->IsString())
            return;
        // FIXME: get render view and send the message over IPC
            qDebug() << (*v8::String::Utf8Value(args[0]->ToString()));
    }

};

content::RenderView *NavigatorQtExtensionWrapper::GetRenderView()
{
      blink::WebFrame* webframe = blink::WebFrame::frameForCurrentContext();
      DCHECK(webframe) << "There should be an active frame since we just got "
          "a native function called.";
      if (!webframe) return NULL;

      blink::WebView* webview = webframe->view();
      if (!webview) return NULL;  // can happen during closing

      return content::RenderView::FromWebView(webview);
}

v8::Handle<v8::FunctionTemplate> NavigatorQtExtensionWrapper::GetNativeFunctionTemplate(v8::Isolate *isolate, v8::Handle<v8::String> name)
{
    if (name->Equals(v8::String::NewFromUtf8(isolate, "NativeQtPostMessage")))
        return v8::FunctionTemplate::New(isolate, NativeQtPostMessage);

    return v8::Handle<v8::FunctionTemplate>();

}

v8::Extension *NavigatorQtExtension::Get()
{
    return new NavigatorQtExtensionWrapper;
}
