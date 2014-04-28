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

#ifndef NAVIGATOR_QT_EXTENSION_H
#define NAVIGATOR_QT_EXTENSION_H

#include "v8/include/v8.h"

#include <QtCore/qcompilerdetection.h>

namespace base {
class ListValue;
}
namespace blink {
class WebView;
}
namespace content {
class RenderView;
}


class NavigatorQtExtension : public v8::Extension
{

public:
    NavigatorQtExtension();

    // v8::Extension
    virtual v8::Handle<v8::FunctionTemplate> GetNativeFunctionTemplate(v8::Isolate *isolate, v8::Handle<v8::String> name) Q_DECL_OVERRIDE;

    void onMessage(const base::ListValue &message, blink::WebView *);

private:
    static content::RenderView *GetRenderView();
    static void NativeQtPostMessage(const v8::FunctionCallbackInfo<v8::Value> &args);


};



#endif // NAVIGATOR_QT_EXTENSION_H