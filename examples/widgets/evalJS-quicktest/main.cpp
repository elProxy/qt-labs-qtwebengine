/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWebEngineWidgets>

#define USE_LAMBDA_SYNTAX

#ifndef USE_LAMBDA_SYNTAX
class WebEnginePage : public QWebEnginePage
{
public:
    void getHeadline() {
        connect(this, &QWebEnginePage::javascriptEvaluated, this, &WebEnginePage::onEvalJS);
        evaluateJavaScriptAsynchronously(QLatin1String("document.querySelector(\"h2\").innerText"));
    }

    void onEvalJS(qint64, const QVariant &stuff) {
        fprintf(stdout, "%s\n", qPrintable(stuff.toString()));
        qApp->quit();
    }

};
#else
typedef QWebEnginePage WebEnginePage;
#endif

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);
    QUrl url;
    if (argc > 1)
        url = QUrl::fromUserInput(argv[1]);
    else
        url = QUrl("http://www.google.com/ncr");
    WebEnginePage p;
    p.load(url);
    QObject::connect(&p, &QWebEnginePage::loadFinished, [&](bool success) {
		    if (!success)
		        qApp->quit();
		    p.evaluateJavaScriptAsynchronously(QLatin1String("document.querySelector(\"h2\").innerText"),
			    [&](const QVariant &stuff) { fprintf(stdout, "[%s] %s\n", qPrintable(p.url().toString()), qPrintable(stuff.toString())); qApp->quit(); });
		    });

    return app.exec();
}
