#include "xcontrol.h"

#include <X11/X.h> // defn. of Window

#include <X11/Xlib.h> // core X11
// Xr/Xc for offscreen window capture
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>

//#include <X11/Xutil.h> more X11 stuff

#include <QPixmap>

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

    if (!XSetInputFocus(display, wId, RevertToNone, CurrentTime)) {
        qWarning("sendkey focus failed");
        goto cleanup;
    }

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

    if (!XSetInputFocus(display, wId, RevertToNone, CurrentTime)) {
        qWarning("sendkey focus failed");
        goto cleanup;
    }

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

    if (!XSetInputFocus(display, wId, RevertToNone, CurrentTime)) {
        qWarning("sendkey focus failed");
        goto cleanup;
    }

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

QImage grabWindowScreenshot(Window wId) {
    Pixmap pixmap;
    XRenderPictFormat *format;
//    XRenderPictureAttributes pic_attrs;
//    Picture picture;
    XImage *xim;
    QImage image;

    display = XOpenDisplay(0);

    int event_base_return, error_base_return;

    if (!XCompositeQueryExtension(display, &event_base_return,
                                  &error_base_return)) {
        qWarning("getscreenshot xcomposite disabled");
        goto cleanup1;
    }

    XCompositeRedirectWindow(display, wId, CompositeRedirectAutomatic);
    pixmap = XCompositeNameWindowPixmap(display, wId);
    // ^ errors if window is neither redirected nor visible

    XWindowAttributes attr;
    if (!XGetWindowAttributes(display, wId, &attr)) {
        qWarning("getscreenshot couldn't grab window attributes");
        goto cleanup2;
    }

    format = XRenderFindVisualFormat(display, attr.visual);
    if (format->type != PictStandardRGB24) {
        qWarning("unexpected image type");
        goto cleanup2;
    }

//    pic_attrs.subwindow_mode = IncludeInferiors;

    //    picture =
    //        XRenderCreatePicture(display, wId, format, CPSubwindowMode,
    //        &pic_attrs);

    //    XRenderComposite(display, PictOpSrc, picture, None, pixmap,
    //    0,0,0,0,0,0,attr.width,attr.height);

    // Note: simply rendering to image.x11PictureHandle()
    // does not work if QPixmaps are raster (not native) -- the default

    // AllPlanes & XYPixmap + Format_Mono = BW jagged edges
    // XYBitmap = crash
    // AllPlanes, ZPixmap + Format_Mono = related to image, just... odd
    // AllPlanes, ZPixmap + Format_RGB32 = perfect copy!

    // only remaining issue:

    xim = XGetImage(display, /*pixmap*/ wId, 0, 0, attr.width, attr.height,
                    AllPlanes, ZPixmap);
    image = QImage((uchar *)xim->data, xim->width, xim->height,
                   QImage::Format_RGB32).copy();
    XDestroyImage(xim);

//    XRenderFreePicture(display, picture);

cleanup2:
    XFreePixmap(display, pixmap);
    XCompositeUnredirectWindow(display, wId, CompositeRedirectAutomatic);
cleanup1:
    XCloseDisplay(display);
    return image;
}
