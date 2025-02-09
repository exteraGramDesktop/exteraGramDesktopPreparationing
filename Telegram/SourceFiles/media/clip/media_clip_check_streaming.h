/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitgramdesktop/rabbitgramdesktop/blob/dev/LEGAL
*/
#pragma once

namespace Core {
class FileLocation;
} // namespace Core

namespace Media {
namespace Clip {

bool CheckStreamingSupport(
	const Core::FileLocation &location,
	QByteArray data);

} // namespace Clip
} // namespace Media
