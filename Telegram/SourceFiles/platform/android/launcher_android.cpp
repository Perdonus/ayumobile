/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/android/launcher_android.h"

#ifdef Q_OS_ANDROID
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/qjniobject.h>
#include <QtCore/qnativeinterface.h>
#else // Qt >= 6.0.0
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QtAndroid>
#endif // Qt < 6.0.0
#endif // Q_OS_ANDROID

namespace Platform {
namespace {

#ifdef Q_OS_ANDROID
constexpr auto kBridgeClass = "one/ayugram/desktop/AyuGramBridge";

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using AndroidJniObject = QJniObject;
#else // Qt >= 6.0.0
using AndroidJniObject = QAndroidJniObject;
#endif // Qt < 6.0.0

[[nodiscard]] AndroidJniObject AndroidActivity() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return QNativeInterface::QAndroidApplication::context();
#else // Qt >= 6.0.0
	return QtAndroid::androidActivity();
#endif // Qt < 6.0.0
}

[[nodiscard]] jobject ToJObject(const AndroidJniObject &object) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return object.object<jobject>();
#else // Qt >= 6.0.0
	return object.object();
#endif // Qt < 6.0.0
}
#endif // Q_OS_ANDROID

} // namespace

Launcher::Launcher(int argc, char *argv[])
: Core::Launcher(argc, argv) {
}

bool Launcher::launchUpdater(UpdaterLaunch) {
#ifdef Q_OS_ANDROID
	const auto activity = AndroidActivity();
	if (!activity.isValid()) {
		return false;
	}
	return AndroidJniObject::callStaticMethod<jboolean>(
		kBridgeClass,
		"restartApp",
		"(Landroid/content/Context;)Z",
		ToJObject(activity));
#else // Q_OS_ANDROID
	return false;
#endif // !Q_OS_ANDROID
}

} // namespace Platform
