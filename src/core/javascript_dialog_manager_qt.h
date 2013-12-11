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
#ifndef JAVASCRIPT_DIALOG_MANAGER_QT_H
#define JAVASCRIPT_DIALOG_MANAGER_QT_H

#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/common/javascript_message_type.h"

#include "web_contents_adapter_client.h"

#include "qglobal.h"
#include <QMap>

namespace content {
class WebContents;
}

struct JavaScriptDialogControllerPrivate;

class JavaScriptDialogController : public QObject {
    Q_OBJECT
public:
    QString message() const;
    QString defaultPrompt() const;
    WebContentsAdapterClient::JavascriptDialogType type();

public Q_SLOTS:
    void textProvided(const QString &text);
    void accept();
    void reject();

Q_SIGNALS:
    void dialogCloseRequested();

private:
    JavaScriptDialogController(JavaScriptDialogControllerPrivate *);
    void dialogFinished(bool accepted, const base::string16 &);

    QScopedPointer<JavaScriptDialogControllerPrivate> d;
    friend class JavaScriptDialogManagerQt;
};


class JavaScriptDialogManagerQt : public content::JavaScriptDialogManager
{
public:
    // For use with the Singleton helper class from chromium
    static JavaScriptDialogManagerQt *GetInstance();

    virtual void RunJavaScriptDialog(content::WebContents *, const GURL &, const std::string &acceptLang, content::JavaScriptMessageType javascriptMessageType,
                                       const base::string16 &messageText, const base::string16 &defaultPromptText,
                                       const content::JavaScriptDialogManager::DialogClosedCallback &callback, bool *didSuppressMessage) Q_DECL_OVERRIDE;
    virtual void RunBeforeUnloadDialog(content::WebContents *, const base::string16 &messageText, bool isReload,
                                         const content::JavaScriptDialogManager::DialogClosedCallback &callback) Q_DECL_OVERRIDE { Q_UNUSED(messageText); Q_UNUSED(isReload); Q_UNUSED(callback); }
    virtual bool HandleJavaScriptDialog(content::WebContents *, bool accept, const base::string16 *promptOverride) Q_DECL_OVERRIDE;
    // FIXME: handle those as well
    virtual void CancelActiveAndPendingDialogs(content::WebContents *contents) Q_DECL_OVERRIDE { dialogDoneForContents(contents); }
    virtual void WebContentsDestroyed(content::WebContents *contents) Q_DECL_OVERRIDE { dialogDoneForContents(contents); }

    void dialogDoneForContents(content::WebContents *);

private:
    QMap<content::WebContents *, JavaScriptDialogController *> m_activeDialogs;

};

#endif // JAVASCRIPT_DIALOG_MANAGER_QT_H

