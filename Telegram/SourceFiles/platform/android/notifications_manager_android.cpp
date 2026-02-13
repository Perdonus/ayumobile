/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/android/notifications_manager_android.h"

#include "data/data_forum_topic.h"
#include "data/data_peer.h"
#include "data/data_saved_sublist.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "window/notifications_manager.h"

#include <map>

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
namespace Notifications {
namespace {

using NotificationId = Window::Notifications::Manager::NotificationId;
using ContextId = Window::Notifications::Manager::ContextId;

constexpr auto kNotifierClass = "one/ayugram/desktop/AyuGramNotifier";

#ifdef Q_OS_ANDROID
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

void AndroidShowNotification(
		int id,
		const QString &title,
		const QString &text,
		const QString &subtitle) {
	const auto context = AndroidContext();
	if (!context.isValid()) {
		return;
	}

	const auto jTitle = AndroidJniObject::fromString(title);
	const auto jText = AndroidJniObject::fromString(text);
	const auto jSubtitle = AndroidJniObject::fromString(subtitle);

	AndroidJniObject::callStaticMethod<void>(
		kNotifierClass,
		"show",
		"(Landroid/content/Context;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
		ToJObject(context),
		jint(id),
		jTitle.object<jstring>(),
		jText.object<jstring>(),
		jSubtitle.object<jstring>());
}

void AndroidCancelNotification(int id) {
	const auto context = AndroidContext();
	if (!context.isValid()) {
		return;
	}
	AndroidJniObject::callStaticMethod<void>(
		kNotifierClass,
		"cancel",
		"(Landroid/content/Context;I)V",
		ToJObject(context),
		jint(id));
}

void AndroidClearAllNotifications() {
	const auto context = AndroidContext();
	if (!context.isValid()) {
		return;
	}
	AndroidJniObject::callStaticMethod<void>(
		kNotifierClass,
		"clearAll",
		"(Landroid/content/Context;)V",
		ToJObject(context));
}
#else // Q_OS_ANDROID
void AndroidShowNotification(int, const QString &, const QString &, const QString &) {
}
void AndroidCancelNotification(int) {
}
void AndroidClearAllNotifications() {
}
#endif // !Q_OS_ANDROID

[[nodiscard]] int ComposeAndroidId(const NotificationId &id) {
	const auto &context = id.contextId;
	auto hash = uint64(1469598103934665603ULL);
	const auto mix = [&](uint64 value) {
		hash ^= value;
		hash *= 1099511628211ULL;
	};
	mix(context.sessionId);
	mix(uint64(context.peerId));
	mix(uint64(context.topicRootId));
	mix(uint64(context.monoforumPeerId));
	mix(uint64(id.msgId));

	auto result = int(hash & 0x7FFFFFFF);
	return result ? result : 1;
}

class AndroidManager final : public Window::Notifications::NativeManager {
public:
	using NativeManager::NativeManager;

protected:
	void doShowNativeNotification(
			NotificationInfo &&info,
			Ui::PeerUserpicView &) override {
		const auto notificationId = NotificationId{
			.contextId = ContextId{
				.sessionId = info.peer->session().uniqueId(),
				.peerId = info.peer->id,
				.topicRootId = info.topicRootId,
				.monoforumPeerId = info.monoforumPeerId,
			},
			.msgId = info.itemId,
		};
		const auto androidId = ComposeAndroidId(notificationId);
		_shown[notificationId] = androidId;
		AndroidShowNotification(
			androidId,
			info.title,
			info.message,
			info.subtitle);
	}

	void doClearAllFast() override {
		_shown.clear();
		AndroidClearAllNotifications();
	}

	void doClearFromItem(not_null<HistoryItem*> item) override {
		const auto notificationId = NotificationId{
			.contextId = ContextId{
				.sessionId = item->history()->session().uniqueId(),
				.peerId = item->history()->peer->id,
				.topicRootId = item->topicRootId(),
				.monoforumPeerId = (item->history()->amMonoforumAdmin()
					? item->sublistPeerId()
					: PeerId()),
			},
			.msgId = item->id,
		};
		const auto i = _shown.find(notificationId);
		if (i == end(_shown)) {
			return;
		}
		AndroidCancelNotification(i->second);
		_shown.erase(i);
	}

	void doClearFromTopic(not_null<Data::ForumTopic*> topic) override {
		const auto sessionId = topic->history()->session().uniqueId();
		const auto peerId = topic->peer()->id;
		const auto topicRootId = topic->rootId();
		clearIf([=](const NotificationId &id) {
			const auto &context = id.contextId;
			return (context.sessionId == sessionId)
				&& (context.peerId == peerId)
				&& (context.topicRootId == topicRootId);
		});
	}

	void doClearFromSublist(not_null<Data::SavedSublist*> sublist) override {
		const auto sessionId = sublist->owningHistory()->session().uniqueId();
		const auto historyPeerId = sublist->owningHistory()->peer->id;
		const auto sublistPeerId = sublist->sublistPeer()->id;
		clearIf([=](const NotificationId &id) {
			const auto &context = id.contextId;
			return (context.sessionId == sessionId)
				&& ((context.monoforumPeerId == sublistPeerId)
					|| (context.peerId == historyPeerId));
		});
	}

	void doClearFromHistory(not_null<History*> history) override {
		const auto sessionId = history->session().uniqueId();
		const auto peerId = history->peer->id;
		clearIf([=](const NotificationId &id) {
			const auto &context = id.contextId;
			return (context.sessionId == sessionId)
				&& (context.peerId == peerId);
		});
	}

	void doClearFromSession(not_null<Main::Session*> session) override {
		const auto sessionId = session->uniqueId();
		clearIf([=](const NotificationId &id) {
			return id.contextId.sessionId == sessionId;
		});
	}

	bool doSkipToast() const override {
		return true;
	}

	void doMaybePlaySound(Fn<void()>) override {
	}

	void doMaybeFlashBounce(Fn<void()>) override {
	}

private:
	void clearIf(Fn<bool(const NotificationId&)> predicate) {
		for (auto i = begin(_shown); i != end(_shown);) {
			if (!predicate(i->first)) {
				++i;
				continue;
			}
			AndroidCancelNotification(i->second);
			i = _shown.erase(i);
		}
	}

	std::map<NotificationId, int> _shown;

};

} // namespace

bool SkipToastForCustom() {
	return false;
}

void MaybePlaySoundForCustom(Fn<void()> playSound) {
	if (playSound) {
		playSound();
	}
}

void MaybeFlashBounceForCustom(Fn<void()> flashBounce) {
	if (flashBounce) {
		flashBounce();
	}
}

bool WaitForInputForCustom() {
	return false;
}

bool Supported() {
#ifdef Q_OS_ANDROID
	return AndroidContext().isValid();
#else // Q_OS_ANDROID
	return false;
#endif // !Q_OS_ANDROID
}

bool Enforced() {
	return true;
}

bool ByDefault() {
	return true;
}

bool VolumeSupported() {
	return false;
}

void Create(Window::Notifications::System *system) {
	system->setManager([=] {
		return Supported()
			? std::make_unique<AndroidManager>(system)
			: std::unique_ptr<Window::Notifications::Manager>();
	});
}

} // namespace Notifications
} // namespace Platform
