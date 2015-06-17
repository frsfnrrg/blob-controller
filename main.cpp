#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include <QDir>
#include <unistd.h>

qint64 getWindowForPid(qint64 pid) {
    QProcess list;
    QStringList args;
    args << "-tree" // this is heavyweight, creates some stray numbers
         << "-root"
         << "-int";
    list.start("xwininfo", args);
    if (!list.waitForFinished(1000)) {
        qDebug(list.errorString().toAscii().constData());
        return 0;
    }
    QString data = list.readAll();
//    qDebug(data.toAscii().constData());
    QVector<qint64> ids;
    QStringList lines = data.split("\n");
    for (QString line : lines) {
        QString concise = line.simplified().split(" ").at(0);
        bool ok = false;
        long id = concise.toLong(&ok);
        if (ok) {
            ids.append(id);
        }
    }

    QVector<qint64> l2ids;
    for (qint64 id : ids) {
        QProcess checker;
        checker.start("xdotool", QStringList() << "getwindowpid"
                                               << QString::number(id));
        if (!checker.waitForFinished(1000)) {
            continue;
        }
        QString data = QString(checker.readAll()).split(" ").at(0);
        bool ok = false;
        long wpid = data.toLong(&ok);
        if (ok) {
//            qDebug("window %d has pid %d", id, wpid);
            if (wpid == pid) {
                l2ids.append(id);
            }
        }
    }

    for (qint64 i : l2ids) {
        qDebug("%lli", i);
    }
    if (l2ids.empty()) {
        qDebug("no hits");
        return 0;
    }
    return l2ids.last();
}

const char* launchXClock = "xclock -update 1";
const char* launchXterm = "xterm";
const char* launchChrome = "chrome --app=https://agar.io";

const char* program = launchChrome;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("Controller");
    a.setOrganizationName("Evil Inc.");

    QProcess process;
    QStringList args;
    //    args << "--app=https://www.google.com";
    qint64 pid = 0;
    QStringList sets = QString(program).split(" ");
    QString name = sets.at(0);
    sets.removeFirst();
    process.startDetached(name, sets, QDir::currentPath(), &pid);
    qDebug("TARGET PID: %lli", pid);

    // then
    // xdotool finds PID from WID
    // xwininfo -children -root -int yields a list of lines started by ids
    // (ignore such lines)

    qint64 wId = 0;
    int retries = 20;
    while (wId == 0 && retries > 0) {
        usleep(1000000);
        wId = getWindowForPid(pid);
        retries--;
    }

    if (wId == 0) {
        qCritical("Could not find target window in time.");
        return 1;
    }
    MainWindow w(wId);
    w.show();

    return a.exec();
}
