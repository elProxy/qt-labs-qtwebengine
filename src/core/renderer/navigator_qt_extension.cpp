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

#include "common/qt_messages.h"

#include "content/public/renderer/render_view.h"
#include "content/public/renderer/v8_value_converter.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include <QScopedPointer>

static const char kNavigatorQtExtensionName[] = "v8/NavigatorQt";

static const char* const kNavigatorQtApi =
        "if (typeof(navigator) === 'undefined')"
        "  navigator = {};"
        "if (typeof(navigator.qt) === 'undefined')"
        "  navigator.qt = {};"
        "navigator.qt.postMessage = function(message) {"
        "  native function NativeQtPostMessage();"
        "  NativeQtPostMessage(message);"
        "};";

// FIXME: we probably want to put all of QWebChannel's JS into the extension for maximal gains.


content::RenderView *NavigatorQtExtension::GetRenderView()
{
    blink::WebFrame *webframe = blink::WebFrame::frameForCurrentContext();
    DCHECK(webframe) << "There should be an active frame since we just got a native function called.";
    if (!webframe)
        return 0;

    blink::WebView *webview = webframe->view();
    if (!webview)
        return 0;  // can happen during closing

    return content::RenderView::FromWebView(webview);
}

void NavigatorQtExtension::NativeQtPostMessage(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    content::RenderView *renderView = GetRenderView();
    if (!renderView || args.Length() != 1)
        return;

    QScopedPointer<content::V8ValueConverter> converter(content::V8ValueConverter::create());
    base::ListValue list;
    base::Value *value = converter->FromV8Value(args[0], args.GetIsolate()->GetCurrentContext());
    list.Set(0, value ? value : base::Value::CreateNullValue());

    renderView->Send(new QtRenderViewObserverHost_NavigatorQtPostMessage(renderView->GetRoutingID(), list));
}

NavigatorQtExtension::NavigatorQtExtension() : v8::Extension(kNavigatorQtExtensionName, kNavigatorQtApi)
{
}


v8::Handle<v8::FunctionTemplate> NavigatorQtExtension::GetNativeFunctionTemplate(v8::Isolate *isolate, v8::Handle<v8::String> name)
{
    if (name->Equals(v8::String::NewFromUtf8(isolate, "NativeQtPostMessage")))
        return v8::FunctionTemplate::New(isolate, NativeQtPostMessage);

    return v8::Handle<v8::FunctionTemplate>();
}

void NavigatorQtExtension::onMessage(const base::ListValue &message, blink::WebView *webView)
{
    Q_ASSERT(webView);
    const base::Value *extractedValue;
    if (!message.Get(0, &extractedValue))
        return;
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::Context> context = webView->mainFrame()->mainWorldScriptContext();
    v8::Context::Scope contextScope(context);

    v8::Handle<v8::Object> global(context->Global());
    v8::Handle<v8::Value> navigatorValue(global->Get(v8::String::NewFromUtf8(isolate, "navigator")));
    if (!navigatorValue->IsObject())
        return;
    v8::Handle<v8::Value> navigatorQtValue(navigatorValue->ToObject()->Get(v8::String::NewFromUtf8(isolate, "qt")));
    if (!navigatorQtValue->IsObject())
        return;
    v8::Handle<v8::Value> onmessageCallbackValue(navigatorQtValue->ToObject()->Get(v8::String::NewFromUtf8(isolate, "onmessage")));
    if (!onmessageCallbackValue->IsFunction()) {
        qWarning("onmessage is not a callable property of navigator.qt. Some things might not work as expected.");
        return;
    }
    v8::Handle<v8::Function> callback = v8::Handle<v8::Function>::Cast(onmessageCallbackValue);
    v8::Handle<v8::Value> argv[1];
    scoped_ptr<content::V8ValueConverter> converter(content::V8ValueConverter::create());
    argv[0] = converter->ToV8Value(extractedValue, context);
    callback->Call(navigatorQtValue, 1, argv);
}
