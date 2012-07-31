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


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

struct Command
{
    const char *string;
    char cmd;
};

static Command cmds[] =
{
    { "profilebars", 'p' },
    { "screenshot", 's' },
    { "profile=full", 'f' },
    { "profile=simple", 'm' },
    { "profile=none", 'n' },
    { "profile=binary", 'b' },
    { "resetfps", 'r' }
};

static void usage()
{
    printf("USAGE: setrendereroption <target renderer PID> [profilebars | screenshot | profile=[full|simple|none|binary] | resetfps]\n");
    printf("\tprofilebars - Toggle whether profile bars are drawn.\n\t\tThe top bar shows render time, the bottom shows overall frame time.\n\t\tEach colored segment is 1/60th of a second.\n");
    printf("\tscreenshot - Save a screenshot to /tmp.\n");
    printf("\tprofile - Set profiling counters mode:\n");
    printf("\t\tnone - Disable profiler logging.\n");
    printf("\t\tsimple - Simple profiler stats (overall frame times).\n");
    printf("\t\tfull - Log all profile counters and timers.\n");
    printf("\t\tbinary - Log profile counters to binary file for later analysis.\n");
    printf("\tresetfps - Reset the average FPS counter statistics.\n");
}

static void sendCommand(const char *pid, char cmd)
{
    char pipename[128];
    snprintf(pipename, sizeof(pipename)-1, "/tmp/render_cmd_%s", pid);
    mkfifo(pipename, 0777);
    int fd = open(pipename, O_RDWR | O_NONBLOCK);

    write(fd, &cmd, 1);

    close(fd);
}

int main(int argc, char **argv)
{
    char cmd = 0;

    if (argc == 3) {
        for (int i=0 ; i < sizeof(cmds)/sizeof(cmds[0]) ; ++i) {
            if (strcmp(cmds[i].string, argv[2]) == 0) {
                cmd = cmds[i].cmd;
                break;
            }
        }
    }

    if (cmd == 0)
        usage();
    else
        sendCommand(argv[1], cmd);
}
