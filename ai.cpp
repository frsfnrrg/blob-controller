#include "ai.h"
#include "opencv2/opencv.hpp" // the everything include
#include <QtGui>

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

void addToGrid(QGridLayout *g, const QString &s, QWidget *w) {
    int l = g->rowCount();
    g->addWidget(new QLabel(s), l, 0);
    g->addWidget(w, l, 1);
}

class SimpleBlobDetectorParameterWindow : public QDialog {
  public:
    SimpleBlobDetectorParameterWindow(QWidget *p) : QDialog(p) {
        setWindowTitle("Parameters");

        thresholdStep = new QDoubleSpinBox();
        thresholdStep->setRange(std::numeric_limits<float>::min(),
                                std::numeric_limits<float>::max());
        thresholdStep->setValue(50.0);

        minThreshold = new QDoubleSpinBox();
        minThreshold->setRange(std::numeric_limits<float>::min(),
                               std::numeric_limits<float>::max());
        minThreshold->setValue(0);

        maxThreshold = new QDoubleSpinBox();
        maxThreshold->setRange(std::numeric_limits<float>::min(),
                               std::numeric_limits<float>::max());
        maxThreshold->setValue(256);

        minRepeatability = new QSpinBox();
        minRepeatability->setRange(0, std::numeric_limits<int>::max());
        minRepeatability->setValue(2);

        minDistBetweenBlobs = new QDoubleSpinBox();
        minDistBetweenBlobs->setRange(std::numeric_limits<float>::min(),
                                      std::numeric_limits<float>::max());
        minDistBetweenBlobs->setValue(2);

        filterByColor = new QCheckBox();

        blobColor = new QSpinBox();
        blobColor->setRange(0, std::numeric_limits<int>::max());
        blobColor->setValue(0);

        filterByArea = new QCheckBox();

        minArea = new QDoubleSpinBox();
        minArea->setRange(std::numeric_limits<float>::min(),
                          std::numeric_limits<float>::max());
        minArea->setValue(15);

        maxArea = new QDoubleSpinBox();
        maxArea->setRange(std::numeric_limits<float>::min(),
                          std::numeric_limits<float>::max());
        maxArea->setValue(std::numeric_limits<float>::max());

        filterByCircularity = new QCheckBox();

        minCircularity = new QDoubleSpinBox();
        minCircularity->setRange(std::numeric_limits<float>::min(),
                                 std::numeric_limits<float>::max());
        minCircularity->setValue(0.8f);

        maxCircularity = new QDoubleSpinBox();
        maxCircularity->setRange(std::numeric_limits<float>::min(),
                                 std::numeric_limits<float>::max());
        maxCircularity->setValue(std::numeric_limits<float>::max());

        filterByInertia = new QCheckBox();

        minInertiaRatio = new QDoubleSpinBox();
        minInertiaRatio->setRange(std::numeric_limits<float>::min(),
                                  std::numeric_limits<float>::max());
        minInertiaRatio->setValue(0.1);

        maxInertiaRatio = new QDoubleSpinBox();
        maxInertiaRatio->setRange(std::numeric_limits<float>::min(),
                                  std::numeric_limits<float>::max());
        maxInertiaRatio->setValue(std::numeric_limits<float>::max());

        filterByConvexity = new QCheckBox();

        minConvexity = new QDoubleSpinBox();
        minConvexity->setRange(std::numeric_limits<float>::min(),
                               std::numeric_limits<float>::max());
        minConvexity->setValue(0.95);

        maxConvexity = new QDoubleSpinBox();
        maxConvexity->setRange(std::numeric_limits<float>::min(),
                               std::numeric_limits<float>::max());
        maxConvexity->setValue(std::numeric_limits<float>::max());

        QGridLayout *lg = new QGridLayout();
        addToGrid(lg, "thresholdStep", thresholdStep);
        addToGrid(lg, "minThreshold", minThreshold);
        addToGrid(lg, "maxThreshold", maxThreshold);
        addToGrid(lg, "minRepeatability", minRepeatability);
        addToGrid(lg, "minDistBetweenBlobs", minDistBetweenBlobs);
        addToGrid(lg, "filterByColor", filterByColor);
        addToGrid(lg, "blobColor", blobColor);
        addToGrid(lg, "filterByArea", filterByArea);
        addToGrid(lg, "minArea", minArea);
        addToGrid(lg, "maxArea", maxArea);
        addToGrid(lg, "filterByCircularity", filterByCircularity);
        addToGrid(lg, "minCircularity", minCircularity);
        addToGrid(lg, "maxCircularity", maxCircularity);
        addToGrid(lg, "filterByInertia", filterByInertia);
        addToGrid(lg, "minInertiaRatio", minInertiaRatio);
        addToGrid(lg, "maxInertiaRatio", maxInertiaRatio);
        addToGrid(lg, "filterByConvexity", filterByConvexity);
        addToGrid(lg, "minConvexity", minConvexity);
        addToGrid(lg, "maxConvexity", maxConvexity);
        this->setLayout(lg);
    }

    cv::SimpleBlobDetector::Params getParams() {
        cv::SimpleBlobDetector::Params params;
        params.thresholdStep = thresholdStep->value();
        params.minThreshold = minThreshold->value();
        params.maxThreshold = maxThreshold->value();
        params.minRepeatability = minRepeatability->value();
        params.minDistBetweenBlobs = minDistBetweenBlobs->value();
        params.filterByColor = filterByColor->isChecked();
        params.blobColor = blobColor->value();
        params.filterByArea = filterByArea->isChecked();
        params.minArea = minArea->value();
        params.maxArea = maxArea->value();
        params.filterByCircularity = filterByCircularity->isChecked();
        params.minCircularity = minCircularity->value();
        params.maxCircularity = maxCircularity->value();
        params.filterByInertia = filterByInertia->isChecked();
        params.minInertiaRatio = minInertiaRatio->value();
        params.maxInertiaRatio = maxInertiaRatio->value();
        params.filterByConvexity = filterByConvexity->isChecked();
        params.minConvexity = minConvexity->value();
        params.maxConvexity = maxConvexity->value();
        return params;
    }

  private:
    QDoubleSpinBox *thresholdStep;
    QDoubleSpinBox *minThreshold;
    QDoubleSpinBox *maxThreshold;
    QSpinBox *minRepeatability;
    QDoubleSpinBox *minDistBetweenBlobs;
    QCheckBox *filterByColor;
    QSpinBox *blobColor;
    QCheckBox *filterByArea;
    QDoubleSpinBox *minArea;
    QDoubleSpinBox *maxArea;
    QCheckBox *filterByCircularity;
    QDoubleSpinBox *minCircularity;
    QDoubleSpinBox *maxCircularity;
    QCheckBox *filterByInertia;
    QDoubleSpinBox *minInertiaRatio;
    QDoubleSpinBox *maxInertiaRatio;
    QCheckBox *filterByConvexity;
    QDoubleSpinBox *minConvexity;
    QDoubleSpinBox *maxConvexity;
};

BlobChaser::BlobChaser() {
    dialog = new QDialog();
    scene = new QGraphicsScene(dialog);
    scene->setSceneRect(0, 0, 50, 50);
    QBrush bgbrush(Qt::white);
    scene->setBackgroundBrush(bgbrush);
    QGraphicsView *widget = new QGraphicsView(scene, dialog);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(widget);
    dialog->setLayout(layout);
    dialog->setWindowTitle("Mirror");

    pen.setColor(Qt::red);
    brush.setColor(Qt::red);

    params = new SimpleBlobDetectorParameterWindow(dialog);
    params->show();
    dialog->show();
}

BlobChaser::~BlobChaser() { delete dialog; }

float dist(cv::Point2f a, cv::Point2f b) {
    float xd = a.x - b.x;
    float yd = a.y - b.y;
    return sqrt(xd * xd + yd * yd);
}

Command BlobChaser::next(const QPixmap &screen) {
    QImage swapped = screen.toImage();
    swapped.invertPixels();
    cv::Mat original(swapped.height(), swapped.width(), CV_8UC3,
                     const_cast<uchar *>(swapped.bits()),
                     swapped.bytesPerLine());
    cv::Mat mat = original;
//    cvtColor(original, mat, CV_RGB2GRAY);

    // might want to skip opencv and do zone control -- e.g, iterate through
    // image, pick undefined pixel, flood fill, repeat. problem is finding a
    // free
    // pixel -- would need voxel-ish tree (divide into N << cells) and a counter
    // per cell

    // requires:
    // a) Dark color scheme
    // b) No names
    // c) No sizes
    // d) No images
    // e) No colors

    scene->setSceneRect(0, 0, swapped.width(), swapped.height());
    scene->clear();

    qDebug("%d %d", mat.rows, mat.cols);

    cv::SimpleBlobDetector detector(params->getParams());
    std::vector<cv::KeyPoint> keypoints;
    detector.detect(mat, keypoints);

    // identify the most central blob. remove from list, define as Me.
    if (keypoints.empty()) {
        return {QPoint(mat.cols / 2, mat.rows / 2), false, false};
    }

    std::vector<cv::KeyPoint> regrow;
    cv::KeyPoint me = keypoints[0];

    cv::Point2f center(mat.cols / 2, mat.rows / 2);

    QImage image(original.data, mat.cols, mat.rows, QImage::Format_ARGB32);
    //    for (int i=0;i<255;i++) {
    //        image.setColor(i, qRgb(i,i,i));
    //    }
    scene->addPixmap(QPixmap::fromImage(image));

    float minDist = mat.rows + mat.cols;
    for (size_t i = 1; i < keypoints.size(); i++) {
        cv::KeyPoint pt = keypoints[i];

        float r = dist(pt.pt, center);
        if (r < minDist) {
            regrow.push_back(me);
            me = pt;
        } else {
            regrow.push_back(pt);
        }

        //        qDebug("x %f y %f angle %f size %f response %f", point.pt.x,
        //        point.pt.y,
        //               point.angle, point.size, point.response);

        scene->addEllipse(pt.pt.x - pt.size / 2, pt.pt.y - pt.size / 2, pt.size,
                          pt.size, pen, brush);
        scene->addRect(0, 0, mat.cols / 2, mat.rows / 2, pen, brush);
    }
    qDebug("ME x %f y %f angle %f size %f response %f %f %f", me.pt.x, me.pt.y,
           me.angle, me.size, me.response, mat.cols / 2.0, mat.rows / 2.0);
}
