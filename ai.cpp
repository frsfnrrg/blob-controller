#include "ai.h"

inline int min(int a, int b) { return a > b ? b : a; }

RotaryControl::RotaryControl() {}

void RotaryControl::start() { startTime = QTime::currentTime(); }

Command RotaryControl::next(const QPixmap &screen) {
    double elapsed = QTime::currentTime().msecsTo(startTime) / 1000.0;
    double scale = elapsed * 3;
    double w = screen.width();
    double h = screen.height();
    int r = h > w ? w / 3 : h / 3;
    int y = h / 2 - sin(scale) * r;
    int x = w / 2 - cos(scale) * r;
    Command out;
    out.mouse = QPoint(x, y);
    out.space = false;
    out.W = false;
    return out;
}

LightSeeker::LightSeeker() {}

Command LightSeeker::next(const QPixmap &screen) {
    // put mouse at weighted average of R+G+B
    QImage image = screen.toImage();
    qint64 x = 0;
    qint64 y = 0;
    Command m;
    qint64 net = 0;
    for (int ix = 0; ix < image.width(); ix++) {
        for (int iy = 0; iy < image.height(); iy++) {
            QRgb rgb = image.pixel(ix, iy);
            int sum = qRed(rgb) + qBlue(rgb) + qGreen(rgb);
            x += ix * sum;
            y += iy * sum;
            net += sum;
        }
    }
    int r =
        image.width() > image.height() ? image.height() / 2 : image.width() / 2;
    double vx = x / (double)net - image.width() / 2;
    double vy = y / (double)net - image.height() / 2;
    double mag = sqrt(vx * vx + vy * vy);

    double cx, cy;
    if (mag < 3.0) {
        double phase = M_PI * (qrand() / (double)RAND_MAX);
        cx = sin(phase);
        cy = cos(phase);
    } else {
        cx = vx / mag;
        cy = vy / mag;
    }

    int ox = image.width() / 2 + cx * r;
    int oy = image.height() / 2 + cy * r;
    m.mouse.setX(ox);
    m.mouse.setY(oy);
    m.space = false;
    m.W = false;
    qDebug("%lld %lld; %lld %lld; %f %f; %d %d; %f; %f %f", x, y, x / net,
           y / net, vx, vy, ox, oy, mag, cx, cy);
    return m;
}

RingRunner::RingRunner() {}
Command RingRunner::next(const QPixmap &screen) {
    QImage image = screen.toImage();

    QMap<int, QPair<QPointF, int>> points;

    int w = image.width();
    int h = image.height();
    int xm = w / 2;
    int ym = h / 2;

    // search by quadrant... every cell in quadrant has a different value

    for (int ix = 0; ix < w; ix++) {
        for (int iy = 0; iy < h; iy++) {
            QRgb rgb = image.pixel(ix, iy);
            int brightness = qRed(rgb) + qBlue(rgb) + qGreen(rgb);
            int xd = (ix - xm);
            int yd = (iy - ym);
            int r2 = xd * xd + yd * yd;
            float r = sqrt(r2);
            QPair<QPointF, int> &loc = points[r2];
            loc = QPair<QPointF, int>(
                loc.first + QPointF(brightness * xd / r, brightness * yd / r),
                loc.second + brightness);
        }
    }
    QPointF vector(0, 0);
    for (const auto i : points.toStdMap()) {
        int r2 = i.first;
        QPair<QPointF, int> vec = i.second;
        qreal rad = qSqrt(vec.first.x()*vec.first.x()+vec.first.y()*vec.first.y());
        QPointF average = rad == 0 ? QPointF(0,0) : vec.first / rad;

//        qDebug("%i %f %f", r2, average.x(), average.y());
        if (r2 > 25) {
            vector += average / r2;
        }
    }


    int r = min(image.width(), image.height()) / 2;
    int ox = image.width() / 2 + vector.x() * r;
    int oy = image.height() / 2 + vector.y() * r;


    qDebug("%f %f %d %d", vector.x(), vector.y(), ox, oy);

    Command m;
    m.mouse = QPoint(ox, oy);
    m.space = m.W = false;
    return m;
}
