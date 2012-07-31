/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDataStream>
#include <QFile>
#include <iostream>
#include <cstdio>
#include <unistd.h>

enum BinRecordType {
    FullFrameRecord = 1,
    SkippedFrameRecord = 2,
    TimerRecord = 3,
    CounterRecord = 4
};

int main(int argc, char *argv[])
{
    int c;
    bool doCsv = false;

    opterr = 0;
    while ((c = getopt(argc, argv, "c")) != -1) {
        switch (c) {
            case 'c':
                doCsv = true;
                break;
            case '?':
                fprintf(stderr, "reader: Unknown option `-%c'\n", optopt);
                return 1;
        }
    }

    if (argc - optind < 1) {
        std::cerr << "reader: No filename supplied.\nUsage: ./reader [-c] <pid.profile>\n   -c:  output csv format\n";
        return 1;
    }

    QFile *f = new QFile(argv[optind]);
    if (!f->open(QIODevice::ReadOnly)) {
        std::cerr << "reader: Could not open '" << argv[optind] << "'\n";
        return 1;
    }

    if (doCsv)
        printf("\"frameCount\",\"renderTimeMs\",\"avgTimeMs\",\"msPerFrame\",\"fps\"\n");

    QDataStream ds(f);
    while (!ds.atEnd()) {
        qint16 type;
        ds >> type;

        switch (type) {
            case FullFrameRecord: {
                qint32 frameCount;
                float renderTimeMs, avgTimeMs, msPerFrame, fps;
                ds >> frameCount;
                ds >> renderTimeMs;
                ds >> avgTimeMs;
                ds >> msPerFrame;
                ds >> fps;

                if (doCsv)
                    printf("%d,%.2f,%.2f,%.2f,%.2f\n",
                        frameCount, renderTimeMs, avgTimeMs, msPerFrame, fps);
                else
                    printf("  ==== ProfileTimer Log [F: %d | Render: %.2fms (%.2f) | Total: %.2fms (%.2f fps)] ====\n",
                        frameCount, renderTimeMs, avgTimeMs, msPerFrame, fps);
                break;
            }
            case SkippedFrameRecord: {
                qint32 frameCount;
                float renderTimeMs;
                ds >> frameCount;
                ds >> renderTimeMs;

                if (!doCsv)
                    printf("  ==== ProfileTimer Log [-- skipped -- Time: %.2f ms] ====\n",
                       renderTimeMs);
                break;
            }
            case TimerRecord: {
                char *nameIn;
                uint nameLen;
                ds.readBytes(nameIn, nameLen);
                QString name = QString::fromLatin1(nameIn, nameLen);
                float ms;
                ds >> ms;

                if (!doCsv)
                    printf("    %s: %.2f ms\n", qPrintable(name), ms);

                delete[] nameIn;
                break;
            }
            case CounterRecord: {
                char *nameIn;
                uint nameLen;
                ds.readBytes(nameIn, nameLen);
                QString name = QString::fromLatin1(nameIn, nameLen);
                qint32 cnt;
                ds >> cnt;

                if (!doCsv)
                    printf("    %s: %d\n", qPrintable(name), cnt);

                delete[] nameIn;
                break;
            }
        }
    }

    return 0;
}

