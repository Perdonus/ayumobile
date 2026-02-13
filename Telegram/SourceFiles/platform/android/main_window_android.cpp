/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/android/main_window_android.h"

#include "window/main_window.h"

#include <QtGui/QScreen>

namespace Platform {

MainWindow::MainWindow(not_null<Window::Controller*> controller)
: Window::MainWindow(controller) {
}

MainWindow::~MainWindow() = default;

void MainWindow::updateWindowIcon() {
	setWindowIcon(Window::CreateIcon());
}

int32 ScreenNameChecksum(const QString &name) {
	return Window::DefaultScreenNameChecksum(name);
}

int32 ScreenNameChecksum(const QScreen *screen) {
	return screen
		? ScreenNameChecksum(screen->name())
		: 0;
}

QString ScreenDisplayLabel(const QScreen *screen) {
	return screen ? screen->name() : QString();
}

} // namespace Platform
