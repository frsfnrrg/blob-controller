#pragma once

#include <QtCore>
#include <QtGui>

typedef struct Command_struct {
    QPoint mouse;
    bool W;
    bool space;
} Command;

class AIFace {
public:
    virtual ~AIFace(){}
    virtual void start() {}
    virtual void stop() {}
    virtual Command next(const QPixmap &screen) = 0;
};

class RotaryControl : public AIFace {
public:
    RotaryControl();
    void start() override;
    Command next(const QPixmap &screen) override;
private:
    QTime startTime;
};

class LightSeeker : public AIFace {
public:
    LightSeeker();
    Command next(const QPixmap &screen) override;
};

class RingRunner : public AIFace {
public:
    RingRunner();
    Command next(const QPixmap &screen) override;
};

class SimpleBlobDetectorParameterWindow;

class BlobChaser : public AIFace {
public:
    BlobChaser();
    virtual ~BlobChaser();
    Command next(const QPixmap &screen) override;
private:
    QDialog* dialog;
    QGraphicsScene* scene;
    QPen pen;
    QBrush brush;
    SimpleBlobDetectorParameterWindow* params;
};

// after that, start w/ opencv.
// do blob-finding and reduce problem to N-points
// of areas->radii, average colors, and centroids
// then move as needed

