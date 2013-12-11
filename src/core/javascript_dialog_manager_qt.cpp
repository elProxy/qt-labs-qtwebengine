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

#include "javascript_dialog_manager_qt.h"

#include "web_contents_adapter_client.h"
#include "web_contents_view_qt.h"
#include "type_conversion.h"

#include "base/memory/singleton.h"

struct JavaScriptDialogControllerPrivate {
    WebContentsAdapterClient::JavascriptDialogType type;
    QString message;
    QString defaultPrompt;
    QString userInput;
    content::JavaScriptDialogManager::DialogClosedCallback callback;
    content::WebContents *contents;
};


QString JavaScriptDialogController::message() const
{
    return d->message;
}

QString JavaScriptDialogController::defaultPrompt() const
{
    return d->defaultPrompt;
}

WebContentsAdapterClient::JavascriptDialogType JavaScriptDialogController::type()
{
    return d->type;
}

void JavaScriptDialogController::textProvided(const QString &text)
{
    d->userInput = text;
}

void JavaScriptDialogController::accept()
{
    dialogFinished(true, toString16(d->userInput));
}

void JavaScriptDialogController::reject()
{
    dialogFinished(false, toString16(d->defaultPrompt));
}

void JavaScriptDialogController::dialogFinished(bool accepted, const base::string16 &promptValue)
{
    // clear the queue first as this could result in the engine asking us to run another dialog.
    JavaScriptDialogManagerQt::GetInstance()->dialogDoneForContents(d->contents);

    d->callback.Run(accepted, promptValue);
}

JavaScriptDialogController::JavaScriptDialogController(JavaScriptDialogControllerPrivate *dd)
{
    Q_ASSERT(dd);
    d.reset(dd);
}

Q_STATIC_ASSERT_X(static_cast<int>(content::JAVASCRIPT_MESSAGE_TYPE_PROMPT) == static_cast<int>(WebContentsAdapterClient::PromptDialog), "These enums should be in sync.");

JavaScriptDialogManagerQt *JavaScriptDialogManagerQt::GetInstance()
{
    return Singleton<JavaScriptDialogManagerQt>::get();
}

void JavaScriptDialogManagerQt::RunJavaScriptDialog(content::WebContents *webContents, const GURL &originUrl, const std::string &acceptLang, content::JavaScriptMessageType javascriptMessageType, const base::string16 &messageText, const base::string16 &defaultPromptText, const content::JavaScriptDialogManager::DialogClosedCallback &callback, bool *didSuppressMessage)
{
    Q_UNUSED(originUrl);
    Q_UNUSED(acceptLang);

    WebContentsAdapterClient *client = WebContentsViewQt::from(webContents->GetView())->client();
    if (!client) {
        *didSuppressMessage = true;
        return;
    }

    WebContentsAdapterClient::JavascriptDialogType dialogType = static_cast<WebContentsAdapterClient::JavascriptDialogType>(javascriptMessageType);
    JavaScriptDialogControllerPrivate *dialogData = new JavaScriptDialogControllerPrivate;
    dialogData->type = dialogType;
    dialogData->message = toQt(messageText).toHtmlEscaped();
    dialogData->defaultPrompt = toQt(defaultPromptText).toHtmlEscaped();
    dialogData->callback = callback;
    dialogData->contents = webContents;

    JavaScriptDialogController *dialog = new JavaScriptDialogController(dialogData);

    // We shouldn't get new dialogs for a given WebContents until we gave back a result.
    Q_ASSERT(!m_activeDialogs.contains(webContents));
    m_activeDialogs.insert(webContents, dialog);

    // FIXME: now switch to this:
    // client->jsDialog(dialog);
}

bool JavaScriptDialogManagerQt::HandleJavaScriptDialog(content::WebContents *contents, bool accept, const base::string16 *promptOverride)
{
    if (!m_activeDialogs.contains(contents))
        return false;
    JavaScriptDialogController *dialog = m_activeDialogs.value(contents);
    /*emit*/ dialog->dialogCloseRequested();
    dialog->dialogFinished(accept, promptOverride ? *promptOverride : base::string16());
    return true;
}


void dialogDoneForContents(content::WebContents *contents)
{
    if (!m_activeDialogs.contains(contents))
        return;
    m_activeDialogs.value(contents)->deleteLater();
    m_activeDialogs.remove(contents);
}
