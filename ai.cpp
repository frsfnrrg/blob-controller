#include "ai.h"
#include "opencv2/opencv.hpp" // the everything include
#include <QtGui>

inline int min(int a, int b) { return a > b ? b : a; }

RotaryControl::RotaryControl() {}

void RotaryControl::start() { startTime = QTime::currentTime(); }

Command RotaryControl::next(const QImage &screen) {
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

Command LightSeeker::next(const QImage &image) {
    // put mouse at weighted average of R+G+B
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
Command RingRunner::next(const QImage &image) {
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
        qreal rad = qSqrt(vec.first.x() * vec.first.x() +
                          vec.first.y() * vec.first.y());
        QPointF average = rad == 0 ? QPointF(0, 0) : vec.first / rad;

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

BlobChaser::BlobChaser() {
    dialog = new QDialog();
    scene = new QGraphicsScene(dialog);
    scene->setSceneRect(0, 0, 50, 50);
    QBrush bgbrush(Qt::white);
    scene->setBackgroundBrush(bgbrush);
    widget = new QGraphicsView(scene, dialog);
    widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    widget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addStretch(1);
    layout->addWidget(widget, 0);
    layout->addStretch(1);
    dialog->setLayout(layout);
    dialog->setWindowTitle("Mirror");
    dialog->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    pen.setColor(Qt::red);
    brush.setColor(Qt::red);

    dialog->show();

    myRadius = 0.1;
}

BlobChaser::~BlobChaser() { delete dialog; }

double attractivenessCurve(double x) {
    // X is logarithmic
    // fear proportional to size
    // problem is the pixel curve - ought to divide by screen max dim proportion
    const double curve[][2] = {
        {-5, 0}, // 0.00625x
        {-2, 1}, // 0.25x
        {-1, 2}, // 0.5x
        {0, -0.15},
        {1, -20}, // 2x
        {2, -15}, // 5x
        {5, -1}   // 32x
    };
    int len = sizeof(curve) / sizeof(curve[0]);
    if (x < curve[0][0]) {
        return curve[0][1];
    }
    for (int i = 0; i < len - 1; i++) {
        if (x > curve[i + 1][0]) {
            continue;
        }
        return (x - curve[i][0]) * (curve[i + 1][1] - curve[i][1]) /
                   (curve[i + 1][0] - curve[i][0]) +
               curve[i][1];
    }
    return curve[len - 1][1];
}

float dist(cv::Point2f a, cv::Point2f b) {
    float xd = a.x - b.x;
    float yd = a.y - b.y;
    return sqrt(xd * xd + yd * yd);
}

qreal dist(QPointF a, QPointF b) {
    qreal xd = a.x() - b.x();
    qreal yd = a.y() - b.y();
    return qSqrt(xd * xd + yd * yd);
}

Command BlobChaser::next(const QImage &screen) {
    // requires:
    // a) Dark color scheme
    // b) No names
    // c) No sizes
    // d) No images
    // e) No colors
    cv::Mat original(screen.height(), screen.width(), CV_8UC4,
                     const_cast<uchar *>(screen.bits()), screen.bytesPerLine());

    scene->setSceneRect(0, 0, screen.width(), screen.height());
    widget->setMinimumSize(QSize(screen.width() + 2, screen.height() + 2));
    widget->fitInView(scene->sceneRect());
    scene->clear();

    // probably should insert intermediate step to filter out the score

    cv::Mat cleared(original.rows, original.cols, original.type());
    cv::threshold(original, cleared, 256 - 25, 256, cv::THRESH_BINARY);

    cv::Mat simple(original.rows, original.cols, CV_8UC1);
    cvtColor(cleared, simple, CV_RGBA2GRAY);

    cv::Mat display = simple.clone();
    QImage image(display.data, display.cols, display.rows, display.step,
                 QImage::Format_Indexed8);
    image.invertPixels();
    scene->addPixmap(QPixmap::fromImage(image));

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(simple, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);

    typedef struct Blob_struct {
        cv::Point2f center;
        double area;
        float radius;
        double circumference;
    } Blob;

    std::vector<Blob> blobs(contours.size());
    for (std::vector<cv::Point> contour : contours) {
        Blob blob;
        blob.area = cv::contourArea(contour);
        cv::minEnclosingCircle(contour, blob.center, blob.radius);
        blob.circumference = cv::arcLength(contour, false);
        blobs.push_back(blob);
    }

    int w = simple.cols, h = simple.rows;

    if (blobs.size() == 0) {
        // command: sit!
        return {QPoint(w / 2, h / 2), false, false};
    }

    std::vector<Blob> allBlobsButMe;
    Blob me = blobs[0];

    cv::Point2f center(w / 2, h / 2);

    float minDist = w + h;
    for (size_t i = 1; i < blobs.size(); i++) {
        Blob pt = blobs[i];

        float r = dist(pt.center, center);
        // need a second condition: containment... (a larger blob containing a
        // smaller blob)
        float s = dist(pt.center, me.center);

        // closer & not engulfed or further and engulfing
        if ((r < minDist && (s + me.radius > pt.radius)) ||
            (s + me.radius < pt.radius)) {
            allBlobsButMe.push_back(me);
            me = pt;
            minDist = r;
        } else {
            allBlobsButMe.push_back(pt);
        }

        float d1 = pt.radius * 2;
        scene->addEllipse(pt.center.x - d1 / 2, pt.center.y - d1 / 2, d1, d1,
                          QPen(Qt::green));
        float d2 = sqrt(pt.area / M_PI) * 2;
        scene->addEllipse(pt.center.x - d2 / 2, pt.center.y - d2 / 2, d2, d2,
                          QPen(Qt::blue));
        float d3 = pt.circumference / M_PI;
        scene->addEllipse(pt.center.x - d3 / 2, pt.center.y - d3 / 2, d3, d3,
                          QPen(Qt::red));
    }
    // The best area estimator is the one derived from the radius (of bounding
    // circle)

    // C/r/pi > 2.15 indicates jagged edges. 2 is typical for a blob. less
    // indicates chunks remove
    if (me.radius > 0.0 && me.circumference / me.radius < 2.15 * M_PI) {
        // not hiding under a spiky ball
        myRadius = me.radius;
    }

    //    qDebug("ME x %f y %f area %f rad %f center %f %f real %s my %f",
    //           me.center.x, me.center.y, me.area, (double)me.radius, w / 2.0,
    //           h / 2.0,
    //           (me.circumference / me.radius > 2.15 * M_PI ? "nope" : "yep"),
    //           myRadius);

    // all motion/attraction relative to the blob, per se
    QPointF qcenter(me.center.x, me.center.y);

    QPointF vector(0, 0);
    for (Blob blob : allBlobsButMe) {
        if (blob.radius == 0.0 ||
            blob.circumference / blob.radius > 2.15 * M_PI) {
            // degenerate or spiky
            continue;
        }

        QPointF bc(blob.center.x, blob.center.y);
        double sep = dist(bc, qcenter);
        if (sep < myRadius) {
            qDebug("%f %f", sep, myRadius);
            // ignore things inside the blob (like text)
            continue;
        }

        // log base E
        double logratio = log2(blob.radius / me.radius);
        double scale = attractivenessCurve(logratio);

        //        qDebug("%f -> %f %f | %f %f", logratio, scale, dist(bc,
        //        qcenter),
        //               bc.x() - qcenter.x(), bc.y() - qcenter.y());
        QPointF shift = (bc - qcenter) * scale / (sep * sep) * 1.1;
        vector += shift;

        QPen pen;
        if (scale < 0) {
            pen.setColor(Qt::red);
        } else {
            pen.setColor(Qt::green);
        }

        scene->addLine(bc.x(), bc.y(), bc.x() + shift.x() * 1000,
                       bc.y() + shift.y() * 1000, pen);
    }

    double x = vector.x();
    double y = vector.y();
    double mag = sqrt(x * x + y * y);
    double r = min(w, h) / 2;
    QPointF final = QPointF(w / 2, h / 2) + vector / mag * r;
    return {final.toPoint(), false, false};
}
