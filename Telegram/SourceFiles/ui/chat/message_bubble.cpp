/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitgramdesktop/rabbitgramdesktop/blob/dev/LEGAL
*/
#include "ui/chat/message_bubble.h"

#include "ui/cached_round_corners.h"
#include "ui/image/image_prepare.h"
#include "ui/chat/chat_style.h"
#include "styles/style_chat.h"

namespace Ui {
namespace {

using Corner = BubbleCornerRounding;

template <
	typename FillBg, // fillBg(QRect rect)
	typename FillSh, // fillSh(QRect rect)
	typename FillCorner> // fillCorner(int x, int y, int index, Corner size)
void PaintBubbleGeneric(
		const SimpleBubble &args,
		FillBg &&fillBg,
		FillSh &&fillSh,
		FillCorner &&fillCorner) {
	using namespace Images;

	const auto topLeft = args.rounding.topLeft;
	const auto topRight = args.rounding.topRight;
	const auto bottomWithTailLeft = args.rounding.bottomLeft;
	const auto bottomWithTailRight = args.rounding.bottomRight;
	const auto bottomLeft = (bottomWithTailLeft == Corner::Tail)
		? Corner::Large
		: bottomWithTailLeft;
	const auto bottomRight = (bottomWithTailRight == Corner::Tail)
		? Corner::Large
		: bottomWithTailRight;
	const auto rect = args.geometry;
	const auto small = BubbleRadiusSmall();
	const auto large = BubbleRadiusLarge();
	const auto cornerSize = [&](Corner corner) {
		return (corner == Corner::Large)
			? large
			: (corner == Corner::None)
			? small
			: 0;
	};
	const auto verticalSkip = [&](Corner left, Corner right) {
		return std::max(cornerSize(left), cornerSize(right));
	};
	const auto top = verticalSkip(topLeft, topRight);
	const auto bottom = verticalSkip(bottomLeft, bottomRight);
	if (top) {
		const auto left = cornerSize(topLeft);
		const auto right = cornerSize(topRight);
		if (left) {
			fillCorner(rect.left(), rect.top(), kTopLeft, topLeft);
			if (const auto add = top - left) {
				fillBg({ rect.left(), rect.top() + left, left, add });
			}
		}
		if (const auto fill = rect.width() - left - right; fill > 0) {
			fillBg({ rect.left() + left, rect.top(), fill, top });
		}
		if (right) {
			fillCorner(
				rect.left() + rect.width() - right,
				rect.top(),
				kTopRight,
				topRight);
			if (const auto add = top - right) {
				fillBg({
					rect.left() + rect.width() - right,
					rect.top() + right,
					right,
					add,
				});
			}
		}
	}
	if (const auto fill = rect.height() - top - bottom; fill > 0) {
		fillBg({ rect.left(), rect.top() + top, rect.width(), fill });
	}
	if (bottom) {
		const auto left = cornerSize(bottomLeft);
		const auto right = cornerSize(bottomRight);
		if (left) {
			fillCorner(
				rect.left(),
				rect.top() + rect.height() - left,
				kBottomLeft,
				bottomLeft);
			if (const auto add = bottom - left) {
				fillBg({
					rect.left(),
					rect.top() + rect.height() - bottom,
					left,
					add,
				});
			}
		}
		if (const auto fill = rect.width() - left - right; fill > 0) {
			fillBg({
				rect.left() + left,
				rect.top() + rect.height() - bottom,
				fill,
				bottom,
			});
		}
		if (right) {
			fillCorner(
				rect.left() + rect.width() - right,
				rect.top() + rect.height() - right,
				kBottomRight,
				bottomRight);
			if (const auto add = bottom - right) {
				fillBg({
					rect.left() + rect.width() - right,
					rect.top() + rect.height() - bottom,
					right,
					add,
				});
			}
		}
	}
	if (!args.shadowed) {
		return;
	}
	const auto shLeft = rect.x() + cornerSize(bottomLeft);
	const auto shWidth = rect.x()
		+ rect.width()
		- cornerSize(bottomRight)
		- shLeft;
	if (shWidth > 0) {
		fillSh({ shLeft, rect.y() + rect.height(), shWidth, st::msgShadow });
	}
}

void PaintPatternBubble(QPainter &p, const SimpleBubble &args) {
	const auto opacity = args.st->msgOutBg()->c.alphaF();
	const auto shadowOpacity = opacity * args.st->msgOutShadow()->c.alphaF();
	const auto pattern = args.pattern;
	const auto fillBg = [&](const QRect &rect) {
		const auto fill = rect.intersected(args.patternViewport);
		if (!fill.isEmpty()) {
			PaintPatternBubblePart(
				p,
				args.patternViewport,
				pattern->pixmap,
				fill);
		}
	};
	const auto fillSh = [&](const QRect &rect) {
		p.setOpacity(shadowOpacity);
		fillBg(rect);
		p.setOpacity(opacity);
	};
	const auto fillPattern = [&](
			int x,
			int y,
			const QImage &mask,
			QImage &cache) {
		PaintPatternBubblePart(
			p,
			args.patternViewport,
			pattern->pixmap,
			QRect(QPoint(x, y), mask.size() / int(mask.devicePixelRatio())),
			mask,
			cache);
	};
	const auto fillCorner = [&](int x, int y, int index, Corner size) {
		auto &corner = (size == Corner::Large)
			? pattern->cornersLarge[index]
			: pattern->cornersSmall[index];
		auto &cache = (size == Corner::Large)
			? (index < 2
				? pattern->cornerTopLargeCache
				: pattern->cornerBottomLargeCache)
			: (index < 2
				? pattern->cornerTopSmallCache
				: pattern->cornerBottomSmallCache);
		fillPattern(x, y, corner, cache);
	};

	p.setOpacity(opacity);
	PaintBubbleGeneric(args, fillBg, fillSh, fillCorner);
	p.setOpacity(1.);
}

void PaintSolidBubble(QPainter &p, const SimpleBubble &args) {
	const auto &st = args.st->messageStyle(args.outbg, args.selected);
	const auto &bg = st.msgBg;
	const auto sh = (args.rounding.bottomRight == Corner::None)
		? nullptr
		: &st.msgShadow;

	PaintBubbleGeneric(args, [&](const QRect &rect) {
		p.fillRect(rect, bg);
	}, [&](const QRect &rect) {
		p.fillRect(rect, *sh);
	}, [&](int x, int y, int index, Corner size) {
		auto &corners = (size == Corner::Large)
			? st.msgBgCornersLarge
			: st.msgBgCornersSmall;
		p.drawPixmap(x, y, corners.p[index]);
	});
}

} // namespace

std::unique_ptr<BubblePattern> PrepareBubblePattern(
		not_null<const style::palette*> st) {
	auto result = std::make_unique<Ui::BubblePattern>();
	result->cornersSmall = Images::CornersMask(BubbleRadiusSmall());
	result->cornersLarge = Images::CornersMask(BubbleRadiusLarge());
	const auto addShadow = [&](QImage &bottomCorner) {
		auto result = QImage(
			bottomCorner.width(),
			(bottomCorner.height()
				+ st::msgShadow * int(bottomCorner.devicePixelRatio())),
			QImage::Format_ARGB32_Premultiplied);
		result.fill(Qt::transparent);
		result.setDevicePixelRatio(bottomCorner.devicePixelRatio());
		auto p = QPainter(&result);
		p.setOpacity(st->msgInShadow()->c.alphaF());
		p.drawImage(0, st::msgShadow, bottomCorner);
		p.setOpacity(1.);
		p.drawImage(0, 0, bottomCorner);
		p.end();

		bottomCorner = std::move(result);
	};
	addShadow(result->cornersSmall[2]);
	addShadow(result->cornersSmall[3]);
	result->cornerTopSmallCache = QImage(
		result->cornersSmall[0].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerTopLargeCache = QImage(
		result->cornersLarge[0].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerBottomSmallCache = QImage(
		result->cornersSmall[2].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerBottomLargeCache = QImage(
		result->cornersLarge[2].size(),
		QImage::Format_ARGB32_Premultiplied);
	return result;
}

void FinishBubblePatternOnMain(not_null<BubblePattern*> pattern) {
	pattern->tailCache = QImage(
		pattern->tailLeft.size(),
		QImage::Format_ARGB32_Premultiplied);
}

void PaintBubble(QPainter &p, const SimpleBubble &args) {
	if (!args.selected
		&& args.outbg
		&& args.pattern
		&& !args.patternViewport.isEmpty()
		&& !args.pattern->pixmap.size().isEmpty()) {
		PaintPatternBubble(p, args);
	} else {
		PaintSolidBubble(p, args);
	}
}

void PaintBubble(QPainter &p, const ComplexBubble &args) {
	if (args.selection.empty()) {
		PaintBubble(p, args.simple);
		return;
	}
	const auto rect = args.simple.geometry;
	const auto left = rect.x();
	const auto width = rect.width();
	const auto top = rect.y();
	const auto bottom = top + rect.height();
	const auto paintOne = [&](
			QRect geometry,
			bool selected,
			bool fromTop,
			bool tillBottom) {
		auto simple = args.simple;
		simple.geometry = geometry;
		simple.selected = selected;
		if (!fromTop) {
			simple.rounding.topLeft
				= simple.rounding.topRight
				= Corner::None;
		}
		if (!tillBottom) {
			simple.rounding.bottomLeft
				= simple.rounding.bottomRight
				= Corner::None;
			simple.shadowed = false;
		}
		PaintBubble(p, simple);
	};
	auto from = top;
	for (const auto &selected : args.selection) {
		if (selected.top > from) {
			paintOne(
				QRect(left, from, width, selected.top - from),
				false,
				(from <= top),
				false);
		}
		paintOne(
			QRect(left, selected.top, width, selected.height),
			true,
			(selected.top <= top),
			(selected.top + selected.height >= bottom));
		from = selected.top + selected.height;
	}
	if (from < bottom) {
		paintOne(
			QRect(left, from, width, bottom - from),
			false,
			false,
			true);
	}
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target) {
	const auto factor = pixmap.devicePixelRatio();
	if (viewport.size() * factor == pixmap.size()) {
		const auto fill = target.intersected(viewport);
		if (fill.isEmpty()) {
			return;
		}
		p.drawPixmap(fill, pixmap, QRect(
			(fill.topLeft() - viewport.topLeft()) * factor,
			fill.size() * factor));
	} else {
		const auto to = viewport;
		const auto from = QRect(QPoint(), pixmap.size());
		const auto deviceRect = QRect(
			QPoint(),
			QSize(p.device()->width(), p.device()->height()));
		const auto clip = (target != deviceRect);
		if (clip) {
			p.setClipRect(target);
		}
		p.drawPixmap(to, pixmap, from);
		if (clip) {
			p.setClipping(false);
		}
	}
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target,
		const QImage &mask,
		QImage &cache) {
	Expects(mask.bytesPerLine() == mask.width() * 4);
	Expects(mask.format() == QImage::Format_ARGB32_Premultiplied);

	if (cache.size() != mask.size()) {
		cache = QImage(
			mask.size(),
			QImage::Format_ARGB32_Premultiplied);
	}
	cache.setDevicePixelRatio(mask.devicePixelRatio());
	Assert(cache.bytesPerLine() == cache.width() * 4);
	memcpy(cache.bits(), mask.constBits(), mask.sizeInBytes());

	auto q = QPainter(&cache);
	q.setCompositionMode(QPainter::CompositionMode_SourceIn);
	PaintPatternBubblePart(
		q,
		viewport.translated(-target.topLeft()),
		pixmap,
		QRect(QPoint(), cache.size() / int(cache.devicePixelRatio())));
	q.end();

	p.drawImage(target, cache);
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target,
		Fn<void(QPainter&)> paintContent,
		QImage &cache) {
	Expects(paintContent != nullptr);

	const auto targetOrigin = target.topLeft();
	const auto targetSize = target.size();
	if (cache.size() != targetSize * style::DevicePixelRatio()) {
		cache = QImage(
			target.size() * style::DevicePixelRatio(),
			QImage::Format_ARGB32_Premultiplied);
		cache.setDevicePixelRatio(style::DevicePixelRatio());
	}
	cache.fill(Qt::transparent);
	auto q = QPainter(&cache);
	q.translate(-targetOrigin);
	paintContent(q);
	q.translate(targetOrigin);
	q.setCompositionMode(QPainter::CompositionMode_SourceIn);
	PaintPatternBubblePart(
		q,
		viewport.translated(-targetOrigin),
		pixmap,
		QRect(QPoint(), targetSize));
	q.end();

	p.drawImage(target, cache);
}

} // namespace Ui
