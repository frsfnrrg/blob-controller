#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>

#include <X11/X.h> // defn. of Window

#include <X11/keysym.h> // list of keysyms

#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Display *display = nullptr;

XKeyEvent createTemplateEvent(Window wId, int type) {
    XKeyEvent event;
    event.display = display;
    event.window = wId;
    event.root = XDefaultRootWindow(display);
    event.same_screen = True;
    event.keycode = 1;
    event.state = 0;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.time = CurrentTime;
    event.type = type;
    return event;
}

void sendKey(Window wId, KeySym key) {
    display = XOpenDisplay(0);

    XKeyEvent event = createTemplateEvent(wId, KeyPress);
    event.keycode = XKeysymToKeycode(display, key);
    qDebug("Sym %lu creates code %u", key, event.keycode);

    if (!XSendEvent(display, wId, True, KeyPressMask, (XEvent *)&event)) {
        qWarning("sendkey send down failed");
        goto cleanup;
    }

    event.time = CurrentTime;
    event.type = KeyRelease;
    if (!XSendEvent(display, wId, True, KeyPressMask, (XEvent *)&event)) {
        qWarning("sendkey send up failed");
        goto cleanup;
    }
cleanup:
    XCloseDisplay(display);
}

void sendVirtualPointerPosition(Window wId, int x, int y) {
    display = XOpenDisplay(0);

    XKeyEvent event = createTemplateEvent(wId, MotionNotify);
    event.x = x;
    event.y = y;

    if (!XSendEvent(display, wId, True, PointerMotionMask, (XEvent *)&event)) {
        qWarning("sendVirtualPointerPosition send failed");
        goto cleanup;
    }

cleanup:
    XCloseDisplay(display);
}

void sendClick(Window wId, int x, int y) {
    display = XOpenDisplay(0);

    XKeyEvent event = createTemplateEvent(wId, ButtonPress);
    event.x = x;
    event.y = y;

    if (!XSendEvent(display, wId, True, ButtonPressMask, (XEvent *)&event)) {
        qWarning("sendClick down failed");
        goto cleanup;
    }

    event.time = CurrentTime;
    event.type = ButtonRelease;
    if (!XSendEvent(display, wId, True, ButtonPressMask, (XEvent *)&event)) {
        qWarning("sendClick up failed");
        goto cleanup;
    }
cleanup:
    XCloseDisplay(display);
}

QSize getWindowSize(Window wId) {
    QSize s;

    display = XOpenDisplay(0);
    XWindowAttributes e;

    if (!XGetWindowAttributes(display, wId, &e)) {
        qWarning("windowsize attributes failed");
        goto cleanup;
    }

    s.setWidth(e.width);
    s.setHeight(e.height);

cleanup:
    XCloseDisplay(display);

    return s;
}

MainWindow::MainWindow(qint64 wId, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    this->wId = wId;

    //    if (display == nullptr) {
    //        qFatal("couldn't open display");
    //    }

    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose);

    container = new QX11EmbedContainer();
    container->setMinimumSize(QSize(50, 50));
    container->discardClient();
    container->show();
    container->setWindowTitle("Victim");

    connect(container, SIGNAL(clientIsEmbedded()), SLOT(ready()));
    connect(container, SIGNAL(error(QX11EmbedContainer::Error)),
            SLOT(handleError(QX11EmbedContainer::Error)));
    connect(container, SIGNAL(clientClosed()), SLOT(closed()));

    connect(ui->actionStart, SIGNAL(triggered(bool)), SLOT(start()));
    connect(ui->actionStop, SIGNAL(triggered(bool)), SLOT(stop()));

    connect(ui->buttonReload, SIGNAL(clicked(bool)), SLOT(sendReload()));
    connect(ui->buttonW, SIGNAL(clicked(bool)), SLOT(sendW()));
    connect(ui->buttonSpace, SIGNAL(clicked(bool)), SLOT(sendSpace()));

    connect(ui->spinFrequency, SIGNAL(valueChanged(double)),
            SLOT(updateFrequency()));

    connect(ui->boxAutoresume, SIGNAL(stateChanged(int)),
            SLOT(updateAutoResume()));

    QTimer::singleShot(50, this, SLOT(embed()));

    updateFrequency();
    snapshots.setSingleShot(false);
    connect(&snapshots, SIGNAL(timeout()), SLOT(takeSnapshot()));

    resumer.setInterval(2500);
    resumer.setSingleShot(false);
    connect(&resumer, SIGNAL(timeout()), SLOT(pingStartButton()));
}

MainWindow::~MainWindow() {
    XCloseDisplay(display);
    delete ui;
    QApplication::quit();
}

void MainWindow::showEvent(QShowEvent *evt) {
    QMainWindow::showEvent(evt);
    updateStatus("Window shown");
}

void MainWindow::embed() {
    updateStatus("Window Displayed. Embedding subordinate (wId: " +
                 QString::number(wId) + ")");
    container->resize(getWindowSize(wId));
    container->embedClient(wId);
}

void MainWindow::start() { updateStatus("Start pressed"); }

void MainWindow::stop() { updateStatus("Stop pressed"); }

void MainWindow::ready() {
    updateStatus("Client is embedded (wId: " + QString::number(wId) + ")");
    startTime = QTime::currentTime();
    snapshots.start();
}

void MainWindow::closed() {
    updateStatus("Client was closed");
    snapshots.stop();
    startTime = QTime();
}

void MainWindow::updateFrequency() {
    snapshots.setInterval(1000.0 / ui->spinFrequency->value());
}
void MainWindow::updateAutoResume() {
    if (ui->boxAutoresume->checkState() == Qt::Checked) {
        resumer.start();
    } else {
        resumer.stop();
    }
}

void MainWindow::pingStartButton() {
    QSize s = getWindowSize(wId);
    int stretch = (s.height() - 650) / 3;
    int y = stretch * 2 + 150;
    int x = s.width() / 2;
    qDebug("ping %d %d -> %d %d", s.width(), s.height(), x, y);
    sendClick(wId, x, y);
}

void MainWindow::handleError(QX11EmbedContainer::Error e) {
    switch (e) {
    default:
        updateStatus("Window Error: really unknown");
        break;
    case QX11EmbedContainer::Unknown:
        updateStatus("Error: unknown");
        break;
    case QX11EmbedContainer::Internal:
        updateStatus("Error: internal");
        break;
    case QX11EmbedContainer::InvalidWindowID:
        updateStatus("Error: invalid window id");
        break;
    }
}

void MainWindow::updateStatus(const QString &e) {
    QTime current = QTime::currentTime();
    int composite = current.second() * 1000 + current.msec();
    QString x = QString::number(composite);

    ui->statusOutput->appendHtml(
        "<pre>" + QString("0").repeated(6 - x.length()) + x + "<post> " + e);
}

void MainWindow::sendW() {
    sendKey(wId, XK_w);
    updateStatus("Send 'W'");
}

void MainWindow::sendSpace() {
    sendKey(wId, XK_space);
    updateStatus("Send 'Space'");
}

void MainWindow::sendReload() {
    sendKey(wId, XK_F5);
    updateStatus("Send 'F5'");
}

void MainWindow::takeSnapshot() {
    QTime t = QTime::currentTime();

    QPixmap image = QPixmap::grabWindow(wId);
    QString name = QDir::homePath() + "/tmp/seq/target" + t.toString() + "-" +
                   QString::number(t.msec()) + ".png";
    if (ui->boxImageCapture->checkState() == Qt::Checked) {
        updateStatus("Taking and saving snapshot");
        QFile file(name);
        file.open(QIODevice::WriteOnly);
        image.save(&file, "PNG");
    }

    // annoying pointer control
    if (ui->boxAutomouse->checkState() == Qt::Checked) {
        double elapsed = t.msecsTo(startTime) / 1000.0;
        double scale = elapsed * 3;
        double w = image.width();
        double h = image.height();
        int r = h > w ? w / 3 : h / 3;
        int y = h / 2 - sin(scale) * r;
        int x = w / 2 - cos(scale) * r;
        qDebug("%d %d", x, y);
        sendVirtualPointerPosition(wId, x, y);
    }
}
