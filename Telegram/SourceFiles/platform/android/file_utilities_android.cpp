/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/android/file_utilities_android.h"

#include <QtGui/QDesktopServices>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

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
namespace File {
namespace {

#ifdef Q_OS_ANDROID
constexpr auto kBridgeClass = "one/ayugram/desktop/AyuGramBridge";

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using AndroidJniObject = QJniObject;
#else // Qt >= 6.0.0
using AndroidJniObject = QAndroidJniObject;
#endif // Qt < 6.0.0

[[nodiscard]] AndroidJniObject AndroidContext() {
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

void InstallPackage(const QString &pathOrUri) {
	const auto context = AndroidContext();
	if (!context.isValid()) {
		return;
	}
	const auto jPathOrUri = AndroidJniObject::fromString(pathOrUri);
	AndroidJniObject::callStaticMethod<void>(
		kBridgeClass,
		"installPackage",
		"(Landroid/content/Context;Ljava/lang/String;)V",
		ToJObject(context),
		jPathOrUri.object<jstring>());
}
#endif // Q_OS_ANDROID

} // namespace

bool UnsafeShowOpenWith(const QString &filepath) {
	if (filepath.isEmpty()) {
		return false;
	}
	const auto file = QFileInfo(filepath);
	const auto url = file.exists()
		? QUrl::fromLocalFile(file.absoluteFilePath())
		: QUrl(filepath);
	return QDesktopServices::openUrl(url);
}

void PostprocessDownloaded(const QString &filepath) {
	if (!filepath.endsWith(QStringLiteral(".apk"), Qt::CaseInsensitive)) {
		return;
	}

#ifdef Q_OS_ANDROID
	InstallPackage(filepath);
#endif // Q_OS_ANDROID
}

} // namespace File
} // namespace Platform
