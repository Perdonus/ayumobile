/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/android/tray_android.h"

namespace Platform {

Tray::Tray() = default;

rpl::producer<> Tray::aboutToShowRequests() const {
	return rpl::never<>();
}

rpl::producer<> Tray::showFromTrayRequests() const {
	return rpl::never<>();
}

rpl::producer<> Tray::hideToTrayRequests() const {
	return rpl::never<>();
}

rpl::producer<> Tray::iconClicks() const {
	return rpl::never<>();
}

bool Tray::hasIcon() const {
	return _hasIcon;
}

void Tray::createIcon() {
	_hasIcon = false;
}

void Tray::destroyIcon() {
	_hasIcon = false;
}

void Tray::updateIcon() {
}

void Tray::createMenu() {
	_actionsLifetime.destroy();
}

void Tray::destroyMenu() {
	_actionsLifetime.destroy();
}

void Tray::addAction(rpl::producer<QString>, Fn<void()> &&) {
}

void Tray::showTrayMessage() const {
}

bool Tray::hasTrayMessageSupport() const {
	return false;
}

rpl::lifetime &Tray::lifetime() {
	return _lifetime;
}

bool HasMonochromeSetting() {
	return false;
}

} // namespace Platform
