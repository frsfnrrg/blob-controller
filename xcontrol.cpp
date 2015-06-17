#include "xcontrol.h"

#include <X11/X.h> // defn. of Window


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
