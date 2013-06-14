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
#ifndef BROWSER_MAIN_PARTS_QT_H
#define BROWSER_MAIN_PARTS_QT_H

#include "content/public/browser/browser_main_parts.h"

#include "base/i18n/icu_util.h"
#include "content/public/common/main_function_params.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

#include <QLocale>

class BrowserMainPartsQt : public content::BrowserMainParts
{
public:
    BrowserMainPartsQt(const content::MainFunctionParams& parameters)
        : content::BrowserMainParts()
        , m_parameters(parameters)
        , m_runMessageLoop(true)
    { }

    void PreMainMessageLoopStart()
    {
        ui::RegisterPathProvider();
        icu_util::Initialize();
        ResourceBundle::InitSharedInstanceWithLocale(QLocale::system().bcp47Name().toLatin1().constData(), NULL);
    }
    void PostMainMessageLoopStart() { }
    void PreEarlyInitialization() { }

    void PreMainMessageLoopRun() {

        m_browserContext.reset(new BrowserContextQt());

        if (m_parameters.ui_task) {
            m_parameters.ui_task->Run();
            delete m_parameters.ui_task;
            m_runMessageLoop = false;
        }
    }

    bool MainMessageLoopRun(int* result_code)  {
        return !m_runMessageLoop;
    }

    void PostMainMessageLoopRun() {
        m_browserContext.reset();
    }

    BrowserContextQt* browser_context() const {
        return m_browserContext.get();
    }

private:
    scoped_ptr<BrowserContextQt> m_browserContext;

    // For running content_browsertests.
    const content::MainFunctionParams& m_parameters;
    bool m_runMessageLoop;

    DISALLOW_COPY_AND_ASSIGN(BrowserMainPartsQt);
};

#endif // BROWSER_MAIN_PARTS_QT_H
