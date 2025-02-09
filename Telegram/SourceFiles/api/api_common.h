/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitgramdesktop/rabbitgramdesktop/blob/dev/LEGAL
*/
#pragma once

#include "data/data_drafts.h"

class History;

namespace Data {
class Thread;
} // namespace Data

namespace Api {

inline constexpr auto kScheduledUntilOnlineTimestamp = TimeId(0x7FFFFFFE);

struct SendOptions {
	PeerData *sendAs = nullptr;
	TimeId scheduled = 0;
	BusinessShortcutId shortcutId = 0;
	EffectId effectId = 0;
	bool silent = false;
	bool handleSupportSwitch = false;
	bool invertCaption = false;
	bool hideViaBot = false;
	crl::time ttlSeconds = 0;
};
[[nodiscard]] SendOptions DefaultSendWhenOnlineOptions();

enum class SendType {
	Normal,
	Scheduled,
	ScheduledToUser, // For "Send when online".
};

struct SendAction {
	explicit SendAction(
		not_null<Data::Thread*> thread,
		SendOptions options = SendOptions());

	not_null<History*> history;
	SendOptions options;
	FullReplyTo replyTo;
	bool clearDraft = true;
	bool generateLocal = true;
	MsgId replaceMediaOf = 0;

	[[nodiscard]] MTPInputReplyTo mtpReplyTo() const;
};

struct MessageToSend {
	explicit MessageToSend(SendAction action) : action(action) {
	}

	SendAction action;
	TextWithTags textWithTags;
	Data::WebPageDraft webPage;
};

struct RemoteFileInfo {
	MTPInputFile file;
	std::optional<MTPInputFile> thumb;
	std::vector<MTPInputDocument> attachedStickers;
};

} // namespace Api
