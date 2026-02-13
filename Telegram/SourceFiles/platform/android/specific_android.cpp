/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/android/specific_android.h"

#include "window/main_window.h"
#include "data/data_location.h"

#include <QtWidgets/QApplication>
#include <QtGui/QDesktopServices>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
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

namespace {

constexpr auto kBridgeClass = "one/ayugram/desktop/AyuGramBridge";
constexpr auto kPermissionGrantedState = jint(0);
constexpr auto kPermissionCanRequestState = jint(1);

[[nodiscard]] QString PermissionName(Platform::PermissionType type) {
	switch (type) {
	case Platform::PermissionType::Microphone:
		return QStringLiteral("android.permission.RECORD_AUDIO");
	case Platform::PermissionType::Camera:
		return QStringLiteral("android.permission.CAMERA");
	}
	return QString();
}

[[nodiscard]] Platform::PermissionStatus PermissionStateFromBridge(jint state) {
	return (state == kPermissionGrantedState)
		? Platform::PermissionStatus::Granted
		: (state == kPermissionCanRequestState)
		? Platform::PermissionStatus::CanRequest
		: Platform::PermissionStatus::Denied;
}

#ifdef Q_OS_ANDROID

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

[[nodiscard]] jint BridgePermissionState(const QString &permission) {
	const auto activity = AndroidActivity();
	if (!activity.isValid()) {
		return kPermissionCanRequestState;
	}

	const auto jPermission = AndroidJniObject::fromString(permission);
	return AndroidJniObject::callStaticMethod<jint>(
		kBridgeClass,
		"permissionState",
		"(Landroid/content/Context;Ljava/lang/String;)I",
		ToJObject(activity),
		jPermission.object<jstring>());
}

[[nodiscard]] jint BridgeRequestPermission(const QString &permission) {
	const auto activity = AndroidActivity();
	if (!activity.isValid()) {
		return kPermissionCanRequestState;
	}

	const auto jPermission = AndroidJniObject::fromString(permission);
	return AndroidJniObject::callStaticMethod<jint>(
		kBridgeClass,
		"requestPermissionSync",
		"(Landroid/content/Context;Ljava/lang/String;I)I",
		ToJObject(activity),
		jPermission.object<jstring>(),
		jint(20000));
}

void BridgeOpenNotificationSettings() {
	const auto activity = AndroidActivity();
	if (!activity.isValid()) {
		return;
	}
	AndroidJniObject::callStaticMethod<void>(
		kBridgeClass,
		"openAppNotificationSettings",
		"(Landroid/content/Context;)V",
		ToJObject(activity));
}

void BridgeOpenAppSettings() {
	const auto activity = AndroidActivity();
	if (!activity.isValid()) {
		return;
	}
	AndroidJniObject::callStaticMethod<void>(
		kBridgeClass,
		"openAppSettings",
		"(Landroid/content/Context;)V",
		ToJObject(activity));
}

#endif // Q_OS_ANDROID

} // namespace

namespace Platform {

void SetApplicationIcon(const QIcon &icon) {
	QApplication::setWindowIcon(icon);
}

QString SingleInstanceLocalServerName(const QString &hash) {
	return QDir::tempPath()
		+ '/'
		+ hash
		+ '-'
		+ QCoreApplication::applicationName();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
std::optional<bool> IsDarkMode() {
	return std::nullopt;
}
#endif // Qt < 6.5.0

void start() {
}

void finish() {
}

PermissionStatus GetPermissionStatus(PermissionType type) {
#ifdef Q_OS_ANDROID
	return PermissionStateFromBridge(BridgePermissionState(PermissionName(type)));
#else // Q_OS_ANDROID
	return PermissionStatus::Granted;
#endif // !Q_OS_ANDROID
}

void RequestPermission(
		PermissionType type,
		Fn<void(PermissionStatus)> resultCallback) {
	if (!resultCallback) {
		return;
	}
#ifdef Q_OS_ANDROID
	resultCallback(PermissionStateFromBridge(BridgeRequestPermission(
		PermissionName(type))));
#else // Q_OS_ANDROID
	resultCallback(PermissionStatus::Granted);
#endif // !Q_OS_ANDROID
}

void OpenSystemSettingsForPermission(PermissionType) {
#ifdef Q_OS_ANDROID
	BridgeOpenAppSettings();
#endif // Q_OS_ANDROID
}

bool OpenSystemSettings(SystemSettingsType type) {
#ifdef Q_OS_ANDROID
	switch (type) {
	case SystemSettingsType::Audio:
		BridgeOpenNotificationSettings();
		return true;
	}
#endif // Q_OS_ANDROID
	return false;
}

bool AutostartSupported() {
	return false;
}

void AutostartRequestStateFromSystem(Fn<void(bool)> callback) {
	if (callback) {
		callback(false);
	}
}

void AutostartToggle(bool, Fn<void(bool)> done) {
	if (done) {
		done(false);
	}
}

bool AutostartSkip() {
	return true;
}

bool TrayIconSupported() {
	return false;
}

bool SkipTaskbarSupported() {
	return false;
}

void NewVersionLaunched(int) {
}

QImage DefaultApplicationIcon() {
	return Window::Logo();
}

QString ApplicationIconName() {
	return QStringLiteral("one.ayugram.desktop");
}

QString ExecutablePathForShortcuts() {
	return QCoreApplication::applicationFilePath();
}

void LaunchMaps(const Data::LocationPoint &point, Fn<void()> fail) {
	const auto url = QUrl(QStringLiteral("geo:%1,%2").arg(
		point.latAsString(),
		point.lonAsString()));
	if (!QDesktopServices::openUrl(url) && fail) {
		fail();
	}
}

namespace ThirdParty {

void start() {
}

void finish() {
}

} // namespace ThirdParty
} // namespace Platform

QString psAppDataPath() {
	return QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation) + '/';
}

void psSendToMenu(bool, bool) {
}

int psCleanup() {
	return 0;
}

int psFixPrevious() {
	return 0;
}
