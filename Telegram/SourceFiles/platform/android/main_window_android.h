/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "platform/platform_main_window.h"

namespace Platform {

class MainWindow : public Window::MainWindow {
public:
	explicit MainWindow(not_null<Window::Controller*> controller);
	~MainWindow();

	void updateWindowIcon() override;

};

[[nodiscard]] int32 ScreenNameChecksum(const QString &name);
[[nodiscard]] int32 ScreenNameChecksum(const QScreen *screen);

[[nodiscard]] QString ScreenDisplayLabel(const QScreen *screen);

} // namespace Platform
