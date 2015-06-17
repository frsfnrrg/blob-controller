#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <xcontrol.h>

MainWindow::MainWindow(qint64 wId, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    this->wId = wId;

    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose);

    currentAI = nullptr;

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

    ui->comboAutoChoice->addItem("Rotary");
    ui->comboAutoChoice->addItem("Lightseeker");
    ui->comboAutoChoice->addItem("Ringrunner");
    connect(ui->comboAutoChoice, SIGNAL(activated(int)), SLOT(autoChanged()));

    QTimer::singleShot(50, this, SLOT(embed()));

    snapshots.setSingleShot(false);
    connect(&snapshots, SIGNAL(timeout()), SLOT(takeSnapshot()));

    resumer.setInterval(2500);
    resumer.setSingleShot(false);
    connect(&resumer, SIGNAL(timeout()), SLOT(pingStartButton()));

    updateFrequency();
    autoChanged();
}

MainWindow::~MainWindow() {
    QApplication::quit();
    // delete ui;
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
    if (ui->boxAutoresume->isChecked()) {
        resumer.start();
    }
}

void MainWindow::closed() {
    updateStatus("Client was closed");
    snapshots.stop();
    startTime = QTime();
    if (resumer.isActive()) {
        resumer.stop();
    }
}

void MainWindow::updateFrequency() {
    snapshots.setInterval(1000.0 / ui->spinFrequency->value());
}
void MainWindow::updateAutoResume() {
    if (snapshots.isActive()) {
        if (ui->boxAutoresume->isChecked()) {
            resumer.start();
        } else {
            resumer.stop();
        }
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

void MainWindow::autoChanged() {
    if (currentAI != nullptr) {
        delete currentAI;
    }
    if (ui->comboAutoChoice->currentIndex() == 0) {
        currentAI = new RotaryControl();
    } else if (ui->comboAutoChoice->currentIndex() == 1) {
        currentAI = new LightSeeker();
    } else {
        currentAI = new RingRunner();
    }
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

    if (ui->boxAutomouse->isChecked()) {
        Command action = currentAI->next(image);
        sendVirtualPointerPosition(wId, action.mouse.x(), action.mouse.y());
        if (action.W) {
            sendKey(wId, XK_w);
        }
        if (action.space) {
            sendKey(wId, XK_space);
        }
    }
}
