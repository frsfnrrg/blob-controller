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
    virtual ~AIFace() {}
    virtual void start() {}
    virtual void stop() {}
    virtual Command next(const QImage &screen) = 0;
};

class RotaryControl : public AIFace {
  public:
    RotaryControl();
    void start() override;
    Command next(const QImage &screen) override;

  private:
    QTime startTime;
};

class LightSeeker : public AIFace {
  public:
    LightSeeker();
    Command next(const QImage &screen) override;
};

class RingRunner : public AIFace {
  public:
    RingRunner();
    Command next(const QImage &screen) override;
};

class BlobChaser : public AIFace {
  public:
    BlobChaser();
    virtual ~BlobChaser();
    Command next(const QImage &screen) override;

  private:
    QDialog *dialog;
    QGraphicsScene *scene;
    QGraphicsView *widget;
    QPen pen;
    QBrush brush;

    double myRadius;
};

double attractivenessCurve(double x);

// next up: modified form of Blobchaser, with
// jagged edge detection (to avoid escaping spikes when it's)
// not needed. Then grid identification (periodic X/Y lines, self code),
// and it's easy. X-render offscreen; play nicer with the mouse (focus blink is
// bad). Make window sizes nicer, force optimal aspect ratio
// also, autoconfig with proper blue button seek, name entry
