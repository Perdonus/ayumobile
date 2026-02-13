/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "platform/platform_current_geo_location.h"

#include "core/current_geo_location.h"

#ifdef Q_OS_ANDROID
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/qjniobject.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qnativeinterface.h>
#else // Qt >= 6.0.0
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <QtAndroidExtras/QtAndroid>
#endif // Qt < 6.0.0
#endif // Q_OS_ANDROID

namespace Platform {

inline void ResolveCurrentExactLocation(Fn<void(Core::GeoLocation)> callback) {
#ifdef Q_OS_ANDROID
	constexpr auto kBridgeClass = "one/ayugram/desktop/AyuGramBridge";
	constexpr auto kPermissionGrantedState = jint(0);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const auto toJObject = [](const QJniObject &object) {
		return object.object<jobject>();
	};
	const auto context = QNativeInterface::QAndroidApplication::context();
	if (!context.isValid()) {
		callback({});
		return;
	}
	const auto jPermission = QJniObject::fromString(
		QStringLiteral("android.permission.ACCESS_FINE_LOCATION"));
	auto permissionState = QJniObject::callStaticMethod<jint>(
		kBridgeClass,
		"permissionState",
		"(Landroid/content/Context;Ljava/lang/String;)I",
		toJObject(context),
		jPermission.object<jstring>());
	if (permissionState != kPermissionGrantedState) {
		permissionState = QJniObject::callStaticMethod<jint>(
			kBridgeClass,
			"requestPermissionSync",
			"(Landroid/content/Context;Ljava/lang/String;I)I",
			toJObject(context),
			jPermission.object<jstring>(),
			jint(10000));
	}
	if (permissionState != kPermissionGrantedState) {
		callback({});
		return;
	}
	const auto values = QJniObject::callStaticObjectMethod(
		kBridgeClass,
		"resolveLastKnownLocation",
		"(Landroid/content/Context;)[D",
		toJObject(context));
	if (!values.isValid()) {
		callback({});
		return;
	}

	auto env = QJniEnvironment();
	const auto array = values.object<jdoubleArray>();
	if (!array) {
		callback({});
		return;
	}
	const auto size = env->GetArrayLength(array);
	if (size < 2) {
		callback({});
		return;
	}

	jdouble point[2] = { 0., 0. };
	env->GetDoubleArrayRegion(array, 0, 2, point);
	callback({
		.point = QPointF(point[0], point[1]),
		.accuracy = Core::GeoLocationAccuracy::Exact,
	});
#else // Qt >= 6.0.0
	const auto toJObject = [](const QAndroidJniObject &object) {
		return object.object();
	};
	const auto context = QtAndroid::androidActivity();
	if (!context.isValid()) {
		callback({});
		return;
	}
	const auto jPermission = QAndroidJniObject::fromString(
		QStringLiteral("android.permission.ACCESS_FINE_LOCATION"));
	auto permissionState = QAndroidJniObject::callStaticMethod<jint>(
		kBridgeClass,
		"permissionState",
		"(Landroid/content/Context;Ljava/lang/String;)I",
		toJObject(context),
		jPermission.object<jstring>());
	if (permissionState != kPermissionGrantedState) {
		permissionState = QAndroidJniObject::callStaticMethod<jint>(
			kBridgeClass,
			"requestPermissionSync",
			"(Landroid/content/Context;Ljava/lang/String;I)I",
			toJObject(context),
			jPermission.object<jstring>(),
			jint(10000));
	}
	if (permissionState != kPermissionGrantedState) {
		callback({});
		return;
	}
	const auto values = QAndroidJniObject::callStaticObjectMethod(
		kBridgeClass,
		"resolveLastKnownLocation",
		"(Landroid/content/Context;)[D",
		toJObject(context));
	if (!values.isValid()) {
		callback({});
		return;
	}

	auto env = QAndroidJniEnvironment();
	const auto array = values.object<jdoubleArray>();
	if (!array) {
		callback({});
		return;
	}
	const auto size = env->GetArrayLength(array);
	if (size < 2) {
		callback({});
		return;
	}

	jdouble point[2] = { 0., 0. };
	env->GetDoubleArrayRegion(array, 0, 2, point);
	callback({
		.point = QPointF(point[0], point[1]),
		.accuracy = Core::GeoLocationAccuracy::Exact,
	});
#endif // Qt < 6.0.0
#else // Q_OS_ANDROID
	callback({});
#endif // !Q_OS_ANDROID
}

inline void ResolveLocationAddress(
		const Core::GeoLocation &location,
		const QString &language,
		Fn<void(Core::GeoAddress)> callback) {
	if (location.failed()) {
		callback({});
		return;
	}
#ifdef Q_OS_ANDROID
	constexpr auto kBridgeClass = "one/ayugram/desktop/AyuGramBridge";
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const auto toJObject = [](const QJniObject &object) {
		return object.object<jobject>();
	};
	const auto context = QNativeInterface::QAndroidApplication::context();
	if (!context.isValid()) {
		callback({});
		return;
	}
	const auto jLanguage = QJniObject::fromString(language);
	const auto address = QJniObject::callStaticObjectMethod(
		kBridgeClass,
		"resolveLocationAddress",
		"(Landroid/content/Context;DDLjava/lang/String;)Ljava/lang/String;",
		toJObject(context),
		jdouble(location.point.x()),
		jdouble(location.point.y()),
		jLanguage.object<jstring>());
	if (!address.isValid()) {
		callback({});
		return;
	}
	const auto text = address.toString();
	callback(text.isEmpty() ? Core::GeoAddress() : Core::GeoAddress{ .name = text });
#else // Qt >= 6.0.0
	const auto toJObject = [](const QAndroidJniObject &object) {
		return object.object();
	};
	const auto context = QtAndroid::androidActivity();
	if (!context.isValid()) {
		callback({});
		return;
	}
	const auto jLanguage = QAndroidJniObject::fromString(language);
	const auto address = QAndroidJniObject::callStaticObjectMethod(
		kBridgeClass,
		"resolveLocationAddress",
		"(Landroid/content/Context;DDLjava/lang/String;)Ljava/lang/String;",
		toJObject(context),
		jdouble(location.point.x()),
		jdouble(location.point.y()),
		jLanguage.object<jstring>());
	if (!address.isValid()) {
		callback({});
		return;
	}
	const auto text = address.toString();
	callback(text.isEmpty() ? Core::GeoAddress() : Core::GeoAddress{ .name = text });
#endif // Qt < 6.0.0
#else // Q_OS_ANDROID
	callback({});
#endif // !Q_OS_ANDROID
}

} // namespace Platform
