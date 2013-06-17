/****************************************************************************
**
** Copyright (C) 2013 Digia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
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

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickView>

class Engine : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString source READ source CONSTANT)
public:

    QString source() const { return m_source; }
    void setSource(const QString &src) { m_source = src; }

private:
    QString m_source;
};

void printHelp() {
    qDebug("Usage:\n"
           " > maskmaker [options] filename\n"
           "\n"
           "Options:\n"
           " -h     --help              This help..");
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    Engine engine;

    QStringList args = app.arguments();

    for (int i=0; i<args.size(); ++i) {
        if (args.at(i) == QStringLiteral("--help") || args.at(i) == QStringLiteral("-h")) {
            printHelp();
            return 0;
        } else if (i == args.size() - 1 && i > 0) {
            engine.setSource(args.at(i));
        }
    }

    QQuickView view;
    view.rootContext()->setContextProperty("engine", &engine);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:/Main.qml"));
    view.show();

    QObject::connect((QObject *) view.engine(), SIGNAL(quit()), &app, SLOT(quit()));

    app.exec();
}

#include "maskmaker.moc"
