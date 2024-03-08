/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitgramdesktop/rabbitgramdesktop/blob/dev/LEGAL
*/
#include "history/view/media/history_view_contact.h"

#include "rabbit/settings/rabbit_settings.h"

#include "layout/layout_selection.h"
#include "mainwindow.h"
#include "boxes/add_contact_box.h"
#include "core/click_handler_types.h" // ClickHandlerContext
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item_components.h"
#include "history/view/history_view_cursor_state.h"
#include "history/view/history_view_reply.h"
#include "history/view/media/history_view_media_common.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "styles/style_boxes.h"
#include "styles/style_chat.h"
#include "ui/chat/chat_style.h"
#include "ui/empty_userpic.h"
#include "ui/painter.h"
#include "ui/power_saving.h"
#include "ui/rect.h"
#include "ui/text/format_values.h" // Ui::FormatPhone
#include "ui/text/text_options.h"
#include "window/window_session_controller.h"

namespace HistoryView {
namespace {

class ContactClickHandler : public LambdaClickHandler {
public:
	using LambdaClickHandler::LambdaClickHandler;

	void setDragText(const QString &t) {
		_dragText = t;
	}

	QString dragText() const override {
		return _dragText;
	}

private:
	QString _dragText;

};

ClickHandlerPtr SendMessageClickHandler(not_null<PeerData*> peer) {
	const auto clickHandlerPtr = std::make_shared<ContactClickHandler>([peer](
			ClickContext context) {
		const auto my = context.other.value<ClickHandlerContext>();
		if (const auto controller = my.sessionWindow.get()) {
			if (controller->session().uniqueId()
					!= peer->session().uniqueId()) {
				return;
			}
			controller->showPeerHistory(
				peer->id,
				Window::SectionShow::Way::Forward);
		}
	});
	if (const auto user = peer->asUser()) {
		clickHandlerPtr->setDragText(user->phone().isEmpty()
			? peer->name()
			: Ui::FormatPhone(user->phone()));
	}
	return clickHandlerPtr;
}

ClickHandlerPtr AddContactClickHandler(not_null<HistoryItem*> item) {
	const auto session = &item->history()->session();
	const auto sharedContact = [=, fullId = item->fullId()] {
		if (const auto item = session->data().message(fullId)) {
			if (const auto media = item->media()) {
				return media->sharedContact();
			}
		}
		return (const Data::SharedContact *)nullptr;
	};
	const auto clickHandlerPtr = std::make_shared<ContactClickHandler>([=](
			ClickContext context) {
		const auto my = context.other.value<ClickHandlerContext>();
		if (const auto controller = my.sessionWindow.get()) {
			if (controller->session().uniqueId() != session->uniqueId()) {
				return;
			}
			if (const auto contact = sharedContact()) {
				controller->show(Box<AddContactBox>(
					session,
					contact->firstName,
					contact->lastName,
					contact->phoneNumber));
			}
		}
	});
	if (const auto contact = sharedContact()) {
		clickHandlerPtr->setDragText(Ui::FormatPhone(contact->phoneNumber));
	}
	return clickHandlerPtr;
}

} // namespace

Contact::Contact(
	not_null<Element*> parent,
	UserId userId,
	const QString &first,
	const QString &last,
	const QString &phone)
: Media(parent)
, _st(st::historyPagePreview)
, _pixh(st::contactsPhotoSize)
, _userId(userId) {
	history()->owner().registerContactView(userId, parent);

	_nameLine.setText(
		st::webPageTitleStyle,
		tr::lng_full_name(
			tr::now,
			lt_first_name,
			first,
			lt_last_name,
			last).trimmed(),
		Ui::WebpageTextTitleOptions());

	_phoneLine.setText(
		st::webPageDescriptionStyle,
		Ui::FormatPhone(phone),
		Ui::WebpageTextTitleOptions());

#if 0 // No info.
	_infoLine.setText(
		st::webPageDescriptionStyle,
		phone,
		Ui::WebpageTextTitleOptions());
#endif
}

Contact::~Contact() {
	history()->owner().unregisterContactView(_userId, _parent);
	if (!_userpic.null()) {
		_userpic = {};
		_parent->checkHeavyPart();
	}
}

void Contact::updateSharedContactUserId(UserId userId) {
	if (_userId != userId) {
		history()->owner().unregisterContactView(_userId, _parent);
		_userId = userId;
		history()->owner().registerContactView(_userId, _parent);
	}
}

QSize Contact::countOptimalSize() {
	_contact = _userId
		? _parent->data()->history()->owner().userLoaded(_userId)
		: nullptr;
	if (_contact) {
		_contact->loadUserpic();
	} else {
		const auto full = _nameLine.toString();
		_photoEmpty = std::make_unique<Ui::EmptyUserpic>(
			Ui::EmptyUserpic::UserpicColor(Data::DecideColorIndex(_userId
				? peerFromUser(_userId)
				: Data::FakePeerIdForJustName(full))),
			full);
	}

	_buttons.clear();
	if (_contact) {
		const auto message = tr::lng_contact_send_message(tr::now).toUpper();
		_buttons.push_back({
			message,
			st::semiboldFont->width(message),
			SendMessageClickHandler(_contact),
		});
		if (!_contact->isContact()) {
			const auto add = tr::lng_contact_add(tr::now).toUpper();
			_buttons.push_back({
				add,
				st::semiboldFont->width(add),
				AddContactClickHandler(_parent->data()),
			});
		}
		_mainButton.link = _buttons.front().link;
	} else {
#if 0 // Can't view contact.
		const auto view = tr::lng_profile_add_contact(tr::now).toUpper();
		_buttons.push_back({
			view,
			st::semiboldFont->width(view),
			AddContactClickHandler(_parent->data()),
		});
#endif
		_mainButton.link = nullptr;
	}

	const auto padding = inBubblePadding() + innerMargin();
	const auto full = Rect(currentSize());
	const auto outer = full - inBubblePadding();
	const auto inner = outer - innerMargin();
	const auto lineLeft = inner.left() + _pixh + inner.left() - outer.left();
	const auto lineHeight = UnitedLineHeight();

	auto maxWidth = _parent->skipBlockWidth();
	auto minHeight = 0;

	auto textMinHeight = 0;
	if (!_nameLine.isEmpty()) {
		accumulate_max(maxWidth, lineLeft + _nameLine.maxWidth());
		textMinHeight += 1 * lineHeight;
	}
	if (!_phoneLine.isEmpty()) {
		accumulate_max(maxWidth, lineLeft + _phoneLine.maxWidth());
		textMinHeight += 1 * lineHeight;
	}
	if (!_infoLine.isEmpty()) {
		accumulate_max(maxWidth, lineLeft + _infoLine.maxWidth());
		textMinHeight += std::min(_infoLine.minHeight(), 1 * lineHeight);
	}
	minHeight = std::max(textMinHeight, st::contactsPhotoSize);

	if (!_buttons.empty()) {
		auto buttonsWidth = rect::m::sum::h(st::historyPageButtonPadding);
		for (const auto &button : _buttons) {
			buttonsWidth += button.width;
		}
		accumulate_max(maxWidth, buttonsWidth);
	}
	maxWidth += rect::m::sum::h(padding);
	minHeight += rect::m::sum::v(padding);

	return { maxWidth, minHeight };
}

void Contact::draw(Painter &p, const PaintContext &context) const {
	if (width() < rect::m::sum::h(st::msgPadding) + 1) {
		return;
	}

	const auto st = context.st;
	const auto sti = context.imageStyle();
	const auto stm = context.messageStyle();

	const auto bubble = st::msgPadding;
	const auto full = Rect(currentSize());
	const auto outer = full - inBubblePadding();
	const auto inner = outer - innerMargin();
	auto tshift = inner.top();

	const auto selected = context.selected();
	const auto view = parent();
	const auto colorIndex = _contact
		? _contact->colorIndex()
		: Data::DecideColorIndex(
			Data::FakePeerIdForJustName(_nameLine.toString()));
	const auto cache = context.outbg
		? stm->replyCache[st->colorPatternIndex(colorIndex)].get()
		: st->coloredReplyCache(selected, colorIndex).get();
	const auto backgroundEmojiId = _contact
		? _contact->backgroundEmojiId()
		: DocumentId();
	const auto backgroundEmoji = backgroundEmojiId
		? st->backgroundEmojiData(backgroundEmojiId).get()
		: nullptr;
	const auto backgroundEmojiCache = backgroundEmoji
		? &backgroundEmoji->caches[Ui::BackgroundEmojiData::CacheIndex(
			selected,
			context.outbg,
			true,
			colorIndex + 1)]
		: nullptr;
	Ui::Text::ValidateQuotePaintCache(*cache, _st);
	Ui::Text::FillQuotePaint(p, outer, *cache, _st);
	if (backgroundEmoji) {
		ValidateBackgroundEmoji(
			backgroundEmojiId,
			backgroundEmoji,
			backgroundEmojiCache,
			cache,
			view);
		if (!backgroundEmojiCache->frames[0].isNull()) {
			const auto end = rect::bottom(inner) + _st.padding.bottom();
			const auto r = outer
				- QMargins(0, 0, 0, rect::bottom(outer) - end);
			FillBackgroundEmoji(p, r, false, *backgroundEmojiCache);
		}
	}

	if (_mainButton.ripple) {
		_mainButton.ripple->paint(
			p,
			outer.x(),
			outer.y(),
			width(),
			&cache->bg);
		if (_mainButton.ripple->empty()) {
			_mainButton.ripple = nullptr;
		}
	}

	{
		const auto left = inner.left();
		const auto top = tshift;
		if (_userId) {
			if (_contact) {
				const auto was = !_userpic.null();
				_contact->paintUserpic(p, _userpic, left, top, _pixh);
				if (!was && !_userpic.null()) {
					history()->owner().registerHeavyViewPart(_parent);
				}
			} else {
				_photoEmpty->paintCircle(p, left, top, _pixh, _pixh);
			}
		} else {
			_photoEmpty->paintCircle(p, left, top, _pixh, _pixh);
		}
		if (context.selected()) {
			auto hq = PainterHighQualityEnabler(p);
			p.setBrush(p.textPalette().selectOverlay);
			p.setPen(Qt::NoPen);
			p.drawEllipse(left, top, _pixh, _pixh);
		}
	}

	const auto lineHeight = UnitedLineHeight();
	const auto lineLeft = inner.left() + _pixh + inner.left() - outer.left();
	const auto lineWidth = rect::right(inner) - lineLeft;

	{
		p.setPen(cache->icon);
		p.setTextPalette(context.outbg
			? stm->semiboldPalette
			: st->coloredTextPalette(selected, colorIndex));

		const auto endskip = _nameLine.hasSkipBlock()
			? _parent->skipBlockWidth()
			: 0;
		_nameLine.drawLeftElided(
			p,
			lineLeft,
			tshift,
			lineWidth,
			width(),
			1,
			style::al_left,
			0,
			-1,
			endskip,
			false,
			context.selection);
		tshift += lineHeight;

		p.setTextPalette(stm->textPalette);
	}
	p.setPen(stm->historyTextFg);
	{
		tshift += st::lineWidth * 3; // Additional skip.
		const auto endskip = _phoneLine.hasSkipBlock()
			? _parent->skipBlockWidth()
			: 0;
		_phoneLine.drawLeftElided(
			p,
			lineLeft,
			tshift,
			lineWidth,
			width(),
			1,
			style::al_left,
			0,
			-1,
			endskip,
			false,
			toTitleSelection(context.selection));
		tshift += 1 * lineHeight;
	}
	if (!_infoLine.isEmpty()) {
		tshift += st::lineWidth * 3; // Additional skip.
		const auto endskip = _infoLine.hasSkipBlock()
			? _parent->skipBlockWidth()
			: 0;
		_parent->prepareCustomEmojiPaint(p, context, _infoLine);
		_infoLine.draw(p, {
			.position = { lineLeft, tshift },
			.outerWidth = width(),
			.availableWidth = lineWidth,
			.spoiler = Ui::Text::DefaultSpoilerCache(),
			.now = context.now,
			.pausedEmoji = context.paused || On(PowerSaving::kEmojiChat),
			.pausedSpoiler = context.paused || On(PowerSaving::kChatSpoiler),
			.selection = toDescriptionSelection(context.selection),
			.elisionHeight = (1 * lineHeight),
			.elisionRemoveFromEnd = endskip,
		});
		tshift += (1 * lineHeight);
	}

	if (!_buttons.empty()) {
		p.setFont(st::semiboldFont);
		p.setPen(cache->icon);
		const auto end = rect::bottom(inner) + _st.padding.bottom();
		const auto line = st::historyPageButtonLine;
		auto color = cache->icon;
		color.setAlphaF(color.alphaF() * 0.3);
		const auto top = end + st::historyPageButtonPadding.top();
		const auto buttonWidth = inner.width() / float64(_buttons.size());
		p.fillRect(inner.x(), end, inner.width(), line, color);
		for (auto i = 0; i < _buttons.size(); i++) {
			const auto &button = _buttons[i];
			const auto left = inner.x() + i * buttonWidth;
			if (button.ripple) {
				button.ripple->paint(p, left, end, buttonWidth, &cache->bg);
				if (button.ripple->empty()) {
					_buttons[i].ripple = nullptr;
				}
			}
			p.drawText(
				left + (buttonWidth - button.width) / 2,
				top + st::semiboldFont->ascent,
				button.text);
		}
	}
}

TextState Contact::textState(QPoint point, StateRequest request) const {
	auto result = TextState(_parent);

	const auto full = Rect(currentSize());
	const auto outer = full - inBubblePadding();
	const auto inner = outer - innerMargin();

	_lastPoint = point;

	if (_buttons.size() > 1) {
		const auto end = rect::bottom(inner) + _st.padding.bottom();
		const auto line = st::historyPageButtonLine;
		const auto bWidth = inner.width() / float64(_buttons.size());
		const auto bHeight = rect::bottom(outer) - end;
		for (auto i = 0; i < _buttons.size(); i++) {
			const auto left = inner.x() + i * bWidth;
			if (QRectF(left, end, bWidth, bHeight).contains(point)) {
				result.link = _buttons[i].link;
				return result;
			}
		}
	}
	if (outer.contains(point)) {
		result.link = _mainButton.link;
		return result;
	}
	return result;
}

void Contact::unloadHeavyPart() {
	_userpic = {};
}

bool Contact::hasHeavyPart() const {
	return !_userpic.null();
}

void Contact::clickHandlerPressedChanged(
		const ClickHandlerPtr &p,
		bool pressed) {
	const auto full = Rect(currentSize());
	const auto outer = full - inBubblePadding();
	const auto inner = outer - innerMargin();
	const auto end = rect::bottom(inner) + _st.padding.bottom();
	if ((_lastPoint.y() < end) || (_buttons.size() <= 1)) {
		if (p != _mainButton.link) {
			return;
		}
		if (pressed) {
			if (!_mainButton.ripple) {
				const auto owner = &parent()->history()->owner();
				_mainButton.ripple = std::make_unique<Ui::RippleAnimation>(
					st::defaultRippleAnimation,
					Ui::RippleAnimation::RoundRectMask(
						outer.size(),
						_st.radius),
					[=] { owner->requestViewRepaint(parent()); });
			}
			_mainButton.ripple->add(_lastPoint - outer.topLeft());
		} else if (_mainButton.ripple) {
			_mainButton.ripple->lastStop();
		}
		return;
	} else if (_buttons.empty()) {
		return;
	}
	const auto bWidth = inner.width() / float64(_buttons.size());
	const auto bHeight = rect::bottom(outer) - end;
	for (auto i = 0; i < _buttons.size(); i++) {
		const auto &button = _buttons[i];
		if (p != button.link) {
			continue;
		}
		if (pressed) {
			if (!button.ripple) {
				const auto owner = &parent()->history()->owner();

				_buttons[i].ripple = std::make_unique<Ui::RippleAnimation>(
					st::defaultRippleAnimation,
					Ui::RippleAnimation::MaskByDrawer(
						QSize(bWidth, bHeight),
						false,
						[=](QPainter &p) {
							p.drawRect(0, 0, bWidth, bHeight);
						}),
					[=] { owner->requestViewRepaint(parent()); });
			}
			button.ripple->add(_lastPoint
				- QPoint(inner.x() + i * bWidth, end));
		} else if (button.ripple) {
			button.ripple->lastStop();
		}
	}
}

QMargins Contact::inBubblePadding() const {
	return {
		st::msgPadding.left(),
		isBubbleTop() ? st::msgPadding.left() : 0,
		st::msgPadding.right(),
		isBubbleBottom() ? (st::msgPadding.left() + bottomInfoPadding()) : 0
	};
}

QMargins Contact::innerMargin() const {
	const auto button = _buttons.empty() ? 0 : st::historyPageButtonHeight;
	return _st.padding + QMargins(0, 0, 0, button);
}

int Contact::bottomInfoPadding() const {
	if (!isBubbleBottom()) {
		return 0;
	}

	auto result = st::msgDateFont->height;

	// We use padding greater than st::msgPadding.bottom() in the
	// bottom of the bubble so that the left line looks pretty.
	// but if we have bottom skip because of the info display
	// we don't need that additional padding so we replace it
	// back with st::msgPadding.bottom() instead of left().
	result += st::msgPadding.bottom() - st::msgPadding.left();
	return result;
}

TextSelection Contact::toTitleSelection(TextSelection selection) const {
	return UnshiftItemSelection(selection, _nameLine);
}

TextSelection Contact::toDescriptionSelection(TextSelection selection) const {
	return UnshiftItemSelection(toTitleSelection(selection), _phoneLine);
}

} // namespace HistoryView
