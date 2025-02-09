/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitgramdesktop/rabbitgramdesktop/blob/dev/LEGAL
*/
#pragma once

class PeerData;

namespace Ui {
class VerticalLayout;
} // namespace Ui

namespace Data {
struct Boost;
struct BoostsListSlice;
struct CreditsHistoryEntry;
struct CreditsStatusSlice;
struct PublicForwardsSlice;
struct RecentPostId;
struct SupergroupStatistics;
} // namespace Data

namespace Main {
class SessionShow;
} // namespace Main

namespace Info::Statistics {

void AddPublicForwards(
	const Data::PublicForwardsSlice &firstSlice,
	not_null<Ui::VerticalLayout*> container,
	Fn<void(Data::RecentPostId)> requestShow,
	not_null<PeerData*> peer,
	Data::RecentPostId contextId);

void AddMembersList(
	Data::SupergroupStatistics data,
	not_null<Ui::VerticalLayout*> container,
	Fn<void(not_null<PeerData*>)> showPeerInfo,
	not_null<PeerData*> peer,
	rpl::producer<QString> title);

void AddBoostsList(
	const Data::BoostsListSlice &firstSlice,
	not_null<Ui::VerticalLayout*> container,
	Fn<void(const Data::Boost &)> boostClickedCallback,
	not_null<PeerData*> peer,
	rpl::producer<QString> title);

void AddCreditsHistoryList(
	std::shared_ptr<Main::SessionShow> show,
	const Data::CreditsStatusSlice &firstSlice,
	not_null<Ui::VerticalLayout*> container,
	Fn<void(const Data::CreditsHistoryEntry &)> entryClickedCallback,
	not_null<PeerData*> premiumBot,
	not_null<QImage*> creditIcon,
	bool in,
	bool out);

} // namespace Info::Statistics
