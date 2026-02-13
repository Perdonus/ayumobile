/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Core {
struct GeoLocation;
struct GeoAddress;
} // namespace Core

namespace Platform {

void ResolveCurrentExactLocation(Fn<void(Core::GeoLocation)> callback);
void ResolveLocationAddress(
	const Core::GeoLocation &location,
	const QString &language,
	Fn<void(Core::GeoAddress)> callback);

} // namespace Platform

// Platform dependent implementations.

#if defined Q_OS_WINRT || defined Q_OS_WIN
#include "platform/win/current_geo_location_win.h"
#elif defined Q_OS_MAC // Q_OS_WINRT || Q_OS_WIN
#include "platform/mac/current_geo_location_mac.h"
#elif defined Q_OS_ANDROID // Q_OS_WINRT || Q_OS_WIN || Q_OS_MAC
#include "platform/android/current_geo_location_android.h"
#else // Q_OS_WINRT || Q_OS_WIN || Q_OS_MAC
#include "platform/linux/current_geo_location_linux.h"
#endif // else for Q_OS_WINRT || Q_OS_WIN || Q_OS_MAC
