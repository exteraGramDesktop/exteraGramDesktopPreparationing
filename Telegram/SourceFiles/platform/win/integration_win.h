/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitGramDesktop/rabbitGramDesktop/blob/dev/LEGAL
*/
#pragma once

#include "base/platform/win/base_windows_shlobj_h.h"
#include "base/platform/win/base_windows_winrt.h"
#include "platform/platform_integration.h"

#include <QAbstractNativeEventFilter>

namespace Platform {

class WindowsIntegration final
	: public Integration
	, public QAbstractNativeEventFilter {
public:
	void init() override;

	[[nodiscard]] ITaskbarList3 *taskbarList() const;

	[[nodiscard]] static WindowsIntegration &Instance();

private:
	bool nativeEventFilter(
		const QByteArray &eventType,
		void *message,
		long *result) override;
	bool processEvent(
		HWND hWnd,
		UINT msg,
		WPARAM wParam,
		LPARAM lParam,
		LRESULT *result);

	uint32 _taskbarCreatedMsgId = 0;
	winrt::com_ptr<ITaskbarList3> _taskbarList;

};

[[nodiscard]] std::unique_ptr<Integration> CreateIntegration();

} // namespace Platform
