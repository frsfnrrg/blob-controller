#pragma once

#include <QSize>
#include <QImage>
#include <X11/keysym.h>
#include <X11/X.h>

void sendKey(Window wId, KeySym key);
void sendVirtualPointerPosition(Window wId, int x, int y);
void sendClick(Window wId, int x, int y);
QSize getWindowSize(Window wId);
QImage grabWindowScreenshot(Window wId);
