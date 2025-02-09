/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitgramdesktop/rabbitgramdesktop/blob/dev/LEGAL
*/
#pragma once

#include "base/object_ptr.h"

class ChannelData;

namespace Ui {
class RpWidget;
} // namespace Ui

[[nodiscard]] object_ptr<Ui::RpWidget> CreateMembersVisibleButton(
	not_null<ChannelData*> megagroup);
