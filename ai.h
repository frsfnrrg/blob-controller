#pragma once

#include <QtCore>
#include <QPixmap>

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

// next step: basic ring-structured avoidance.
// rank things in the nearby ring highly,
// and stay away from those.

// after that, start w/ opencv.
// do blob-finding and reduce problem to N-points
// of areas->radii, average colors, and centroids
// then move as needed
