/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "platform/platform_specific.h"

namespace Platform {

inline void IgnoreApplicationActivationRightNow() {
}

inline void WriteCrashDumpDetails() {
}

inline bool PreventsQuit(Core::QuitReason) {
	return false;
}

inline void ActivateThisProcess() {
}

inline uint64 ActivationWindowId(not_null<QWidget*>) {
	return 1;
}

inline void ActivateOtherProcess(uint64, uint64) {
}

} // namespace Platform

inline void psCheckLocalSocket(const QString &) {
}

QString psAppDataPath();
void psSendToMenu(bool send, bool silent = false);

int psCleanup();
int psFixPrevious();

inline QByteArray psDownloadPathBookmark(const QString &) {
	return QByteArray();
}
inline void psDownloadPathEnableAccess() {
}
