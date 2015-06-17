#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QX11EmbedContainer>
#include <QTimer>
#include <QTime>
#include "ai.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(qint64 wId, QWidget *parent = 0);
    ~MainWindow();

  public slots:
    void start();
    void stop();
    void ready();
    void embed();
    void handleError(QX11EmbedContainer::Error);
    void closed();
    void sendW();
    void sendSpace();
    void sendReload();
    void takeSnapshot();
    void updateFrequency();
    void updateAutoResume();
    void pingStartButton();
    void autoChanged();
    void startGame();

  private:
    virtual void showEvent(QShowEvent *evt);

    void updateStatus(const QString &s);

    Ui::MainWindow *ui;
    QX11EmbedContainer *container;

    qint64 wId;
    QTimer snapshots;
    QTime startTime;
    QTimer resumer;
    AIFace *currentAI;
};

#endif // MAINWINDOW_H
