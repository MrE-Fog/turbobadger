// ================================================================================
// == This file is a part of Tinkerbell UI Toolkit. (C) 2011-2012, Emil Seger�s ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "tb_layout.h"
#include <assert.h>

namespace tinkerbell {

// == TBLayout ==========================================================================

TBLayout::TBLayout(AXIS axis)
	: m_axis(axis)
	, m_spacing(SPACING_FROM_SKIN)
	, m_overflow(0)
	, m_overflow_scroll(0)
	, m_packed_init(0)
{
	m_packed.layout_mode_size = LAYOUT_SIZE_GRAVITY;
	m_packed.layout_mode_pos = LAYOUT_POSITION_CENTER;
	m_packed.layout_mode_overflow = LAYOUT_OVERFLOW_CLIP;
	m_packed.layout_mode_dist = LAYOUT_DISTRIBUTION_PREFERRED;
	m_packed.layout_mode_dist_pos = LAYOUT_DISTRIBUTION_POSITION_CENTER;
	m_packed.paint_overflow_fadeout = 1;
}

void TBLayout::SetAxis(AXIS axis)
{
	if (axis == m_axis)
		return;
	m_axis = axis;
	InvalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

void TBLayout::SetSpacing(int spacing)
{
	if (spacing == m_spacing)
		return;
	m_spacing = spacing;
	InvalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

void TBLayout::SetOverflowScroll(int overflow_scroll)
{
	overflow_scroll = MIN(overflow_scroll, m_overflow);
	overflow_scroll = MAX(overflow_scroll, 0);
	if (overflow_scroll == m_overflow_scroll)
		return;
	m_overflow_scroll = overflow_scroll;
	Invalidate();
}

void TBLayout::SetLayoutSize(LAYOUT_SIZE size)
{
	if (size == m_packed.layout_mode_size)
		return;
	m_packed.layout_mode_size = size;
	InvalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::SetLayoutPosition(LAYOUT_POSITION pos)
{
	if (pos == m_packed.layout_mode_pos)
		return;
	m_packed.layout_mode_pos = pos;
	InvalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::SetLayoutOverflow(LAYOUT_OVERFLOW overflow)
{
	if (overflow == m_packed.layout_mode_overflow)
		return;
	m_packed.layout_mode_overflow = overflow;
	InvalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::SetLayoutDistribution(LAYOUT_DISTRIBUTION distribution)
{
	if (distribution == m_packed.layout_mode_dist)
		return;
	m_packed.layout_mode_dist = distribution;
	InvalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::SetLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION distribution_pos)
{
	if (distribution_pos == m_packed.layout_mode_dist_pos)
		return;
	m_packed.layout_mode_dist_pos = distribution_pos;
	InvalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::SetLayoutOrder(LAYOUT_ORDER order)
{
	bool reversed = (order == LAYOUT_ORDER_TOP_TO_BOTTOM);
	if (reversed == m_packed.mode_reverse_order)
		return;
	m_packed.mode_reverse_order = reversed;
	InvalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::InvalidateLayout(INVALIDATE_LAYOUT il)
{
	m_packed.layout_is_invalid = 1;
	// Continue invalidating parents (depending on il)
	Widget::InvalidateLayout(il);
}

PreferredSize RotPreferredSize(const PreferredSize ps, AXIS axis)
{
	if (axis == AXIS_X)
		return ps;
	PreferredSize psr;
	psr.max_w = ps.max_h;
	psr.max_h = ps.max_w;
	psr.min_w = ps.min_h;
	psr.min_h = ps.min_w;
	psr.pref_w = ps.pref_h;
	psr.pref_h = ps.pref_w;
	return psr;
}

TBRect RotRect(const TBRect &rect, AXIS axis)
{
	if (axis == AXIS_X)
		return rect;
	return TBRect(rect.y, rect.x, rect.h, rect.w);
}

uint32 RotGravity(uint32 gravity, AXIS axis)
{
	if (axis == AXIS_X)
		return gravity;
	uint32 r = 0;
	r |= (gravity & WIDGET_GRAVITY_LEFT) ? WIDGET_GRAVITY_TOP : 0;
	r |= (gravity & WIDGET_GRAVITY_TOP) ? WIDGET_GRAVITY_LEFT : 0;
	r |= (gravity & WIDGET_GRAVITY_RIGHT) ? WIDGET_GRAVITY_BOTTOM : 0;
	r |= (gravity & WIDGET_GRAVITY_BOTTOM) ? WIDGET_GRAVITY_RIGHT : 0;
	return r;
}

bool TBLayout::QualifyForExpansion(WIDGET_GRAVITY gravity)
{
	if (m_packed.layout_mode_dist == LAYOUT_DISTRIBUTION_AVAILABLE)
		return true;
	if (m_packed.layout_mode_dist == LAYOUT_DISTRIBUTION_GRAVITY &&
		((gravity & WIDGET_GRAVITY_LEFT) && (gravity & WIDGET_GRAVITY_RIGHT)))
		return true;
	return false;
}

int TBLayout::GetWantedHeight(WIDGET_GRAVITY gravity, const PreferredSize &ps, int available_height)
{
	int height = 0;
	switch (m_packed.layout_mode_size)
	{
	case LAYOUT_SIZE_GRAVITY:
		height = ((gravity & WIDGET_GRAVITY_TOP) && (gravity & WIDGET_GRAVITY_BOTTOM)) ?
									available_height : MIN(available_height, ps.pref_h);
		break;
	case LAYOUT_SIZE_PREFERRED:
		height = MIN(available_height, ps.pref_h);
		break;
	case LAYOUT_SIZE_AVAILABLE:
		height = MIN(available_height, ps.max_h);
		break;
	};
	height = MIN(height, ps.max_h);
	return height;
}

Widget *TBLayout::GetFirstInLayoutOrder()
{
	return m_packed.mode_reverse_order ? GetLastChild() : GetFirstChild();
}

Widget *TBLayout::GetNextInLayoutOrder(Widget *child)
{
	return m_packed.mode_reverse_order ? child->GetPrev() : child->GetNext();
}

void TBLayout::ValidateLayout(PreferredSize *calculate_ps)
{
	// Layout notes:
	// -All layout code is written for AXIS_X layout.
	//  Instead of duplicating the layout code for both AXIS_X and AXIS_Y, we simply
	//  rotate the in data (rect, gravity, preferred size) and the outdata (rect).

	// FIX: We can cache preferred size, so we don't have to ask all children every time.
	// FIX: Overflow Wrap (multi-row/col)
	// FIX: Overflow scroll (scroll buttons)
	// FIX: ShrinkToFit (For layouts that should not grow more than its childrens preferred size,
	//      f.ex a tab bar that want size available, but not grow above that to the infinite max height)
	// FIX: Column object (optional). For column list, popup items, dialogs for multirow alignment (instead of grid).

	if (!calculate_ps)
	{
		if (!m_packed.layout_is_invalid)
			return;
		m_packed.layout_is_invalid = 0;
	}
	else
	{
		// Maximum size will grow below depending of the childrens maximum size
		calculate_ps->max_w = calculate_ps->max_h = 0;
	}

	// Get spacing from skin, if not specified
	int spacing = m_spacing;
	if (spacing == SPACING_FROM_SKIN)
	{
		TBSkinElement *e = GetSkinBgElement();
		spacing = e ? e->spacing : SKIN_DEFAULT_SPACING;
	}

	TBRect layout_rect = RotRect(GetPaddingRect(), m_axis);

	// Calculate totals for minimum and preferred width that we need for layout.
	int total_preferred_w = 0;
	int total_min_pref_diff_w = 0;
	int total_max_pref_diff_w = 0;

	bool last_child_is_collapsed = false;
	int last_added_spacing = 0;

	for (Widget *child = GetFirstInLayoutOrder(); child; child = GetNextInLayoutOrder(child))
	{
		int ending_space = GetNextInLayoutOrder(child) ? spacing : 0;
		PreferredSize ps = RotPreferredSize(child->GetPreferredSize(), m_axis);
		uint32 gravity = RotGravity(child->GetGravity(), m_axis);

		// Collapse empty widgets completly if there is other widgets.
		last_child_is_collapsed = (ps.pref_w == 0 && m_children.GetFirst() != m_children.GetLast());
		if (last_child_is_collapsed)
			continue;
		last_added_spacing = ending_space;

		total_preferred_w += ps.pref_w + ending_space;
		total_min_pref_diff_w += ps.pref_w - ps.min_w;

		if (QualifyForExpansion(gravity))
		{
			int capped_max_w = MIN(layout_rect.w, ps.max_w);
			total_max_pref_diff_w += capped_max_w - ps.pref_w;
		}

		if (calculate_ps)
		{
			calculate_ps->min_h = MAX(calculate_ps->min_h, ps.min_h);
			calculate_ps->pref_h = MAX(calculate_ps->pref_h, ps.pref_h);
			calculate_ps->min_w += ps.min_w + ending_space;
			calculate_ps->pref_w += ps.pref_w + ending_space;
			calculate_ps->max_w += ps.max_w + ending_space;

			// The widget height depends on layout and widget properties, so get what
			// it would actually use if it was given max_h as available height.
			// If we just used its max_h, that could increase the whole layout size
			// even if the widget wouldn't actually use it.
			int height = GetWantedHeight(gravity, ps, ps.max_h);
			calculate_ps->max_h = MAX(calculate_ps->max_h, height);
		}
	}

	// If the last child collapsed, we have added spacing that should not be there.
	if (last_child_is_collapsed)
	{
		total_preferred_w -= last_added_spacing;
		if (calculate_ps)
		{
			calculate_ps->min_w -= last_added_spacing;
			calculate_ps->pref_w -= last_added_spacing;
			calculate_ps->max_w -= last_added_spacing;
		}
	}
	if (calculate_ps)
	{
		// We just wanted to calculate preferred size, so return without layouting.
		*calculate_ps = RotPreferredSize(*calculate_ps, m_axis);
		return;
	}

	// Pre Layout step (calculate distribution position)
	int missing_space = MAX(total_preferred_w - layout_rect.w, 0);
	int extra_space = MAX(layout_rect.w - total_preferred_w, 0);

	int offset = layout_rect.x;
	if (extra_space && m_packed.layout_mode_dist_pos != LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP)
	{
		// To calculate the offset we need to predict the used space. We can do that by checking
		// the distribution mode and total_max_pref_diff_w. That's how much the widgets could possible
		// expand in the layout below.

		int used_space = total_preferred_w;
		if (m_packed.layout_mode_dist != LAYOUT_DISTRIBUTION_PREFERRED)
			used_space += MIN(extra_space, total_max_pref_diff_w);

		if (m_packed.layout_mode_dist_pos == LAYOUT_DISTRIBUTION_POSITION_CENTER)
			offset += (layout_rect.w - used_space) / 2;
		else // LAYOUT_DISTRIBUTION_POSITION_RIGHT_BOTTOM
			offset += layout_rect.w - used_space;
	}

	// Layout
	int used_space = 0;
	for (Widget *child = GetFirstInLayoutOrder(); child; child = GetNextInLayoutOrder(child))
	{
		int ending_space = GetNextInLayoutOrder(child) ? spacing : 0;
		PreferredSize ps = RotPreferredSize(child->GetPreferredSize(), m_axis);
		uint32 gravity = RotGravity(child->GetGravity(), m_axis);

		// Collapse empty widgets completly if there is other widgets
		if (ps.pref_w == 0 && m_children.GetFirst() != m_children.GetLast())
		{
			child->SetRect(TBRect());
			continue;
		}

		// Calculate width. May shrink if space is missing, or grow if we have extra space.
		int width = ps.pref_w;
		if (missing_space && total_min_pref_diff_w)
		{
			int diff_w = ps.pref_w - ps.min_w;
			float factor = (float)diff_w / (float)total_min_pref_diff_w;
			int removed = (int)(missing_space * factor);
			removed = MIN(removed, diff_w);
			width -= removed;

			total_min_pref_diff_w -= diff_w;
			missing_space -= removed;
		}
		else if (extra_space && total_max_pref_diff_w && QualifyForExpansion(gravity))
		{
			int capped_max_w = MIN(layout_rect.w, ps.max_w);
			int diff_w = capped_max_w - ps.pref_w;
			float factor = (float)diff_w / (float)total_max_pref_diff_w;
			int added = (int)(extra_space * factor);
			added = MIN(added, diff_w);
			width += added;

			total_max_pref_diff_w -= capped_max_w - ps.pref_w;
			extra_space -= added;
		}

		// Calculate height
		int available_height = layout_rect.h;
		int height = GetWantedHeight(gravity, ps, available_height);

		// Calculate position
		int pos = layout_rect.y;
		switch (m_packed.layout_mode_pos)
		{
		case LAYOUT_POSITION_CENTER:
			pos += (available_height - height) / 2;
			break;
		case LAYOUT_POSITION_RIGHT_BOTTOM:
			pos += available_height - height;
			break;
		case LAYOUT_POSITION_GRAVITY:
			if ((gravity & WIDGET_GRAVITY_TOP) && (gravity & WIDGET_GRAVITY_BOTTOM))
				pos += (available_height - height) / 2;
			else if (gravity & WIDGET_GRAVITY_BOTTOM)
				pos += available_height - height;
			break;
		default: // LAYOUT_POSITION_LEFT_TOP
			break;
		};

		// Done! Set rect and increase used space
		TBRect rect(used_space + offset, pos, width, height);
		used_space += width + ending_space;

		child->SetRect(RotRect(rect, m_axis));
	}
	// Update overflow and overflow scroll
	m_overflow = MAX(0, used_space - layout_rect.w);
	SetOverflowScroll(m_overflow_scroll);
}

PreferredSize TBLayout::GetPreferredContentSize()
{
	// Do a layout pass (without layouting) to check childrens preferences.
	PreferredSize ps;
	ValidateLayout(&ps);
	return ps;
}

bool TBLayout::OnEvent(const WidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_WHEEL)
	{
		int old_scroll = GetOverflowScroll();
		SetOverflowScroll(m_overflow_scroll + ev.delta * 40);
		return m_overflow_scroll != old_scroll;
	}
	return false;
}

void TBLayout::OnPaintChildren()
{
	TBRect padding_rect = GetPaddingRect();
	if (padding_rect.IsEmpty())
		return;

	// If we overflow the layout, apply clipping when painting children
	TBRect old_clip_rect;
	if (m_overflow)
	{
		// We only want clipping in one axis (the overflowing one) so we
		// don't damage any expanded skins on the other axis. Add some fluff.
		TBRect clip_rect = padding_rect;
		const int fluff = 100;

		if (m_axis == AXIS_X)
			clip_rect = clip_rect.Expand(m_overflow_scroll == 0 ? fluff : 0, fluff,
										m_overflow_scroll == m_overflow ? fluff : 0, fluff);
		else
			clip_rect = clip_rect.Expand(fluff, m_overflow_scroll == 0 ? fluff : 0,
										fluff, m_overflow_scroll == m_overflow ? fluff : 0);

		old_clip_rect = g_renderer->SetClipRect(clip_rect, true);

		TB_IF_GFX_DEBUG(g_renderer->DrawRectDebug(padding_rect, 255, 0, 0, 100));
	}

	// Paint children
	Widget::OnPaintChildren();

	// Paint fadeout image over the overflowed edges
	// to the indicate to used that it's overflowed.
	if (m_overflow && m_packed.paint_overflow_fadeout)
	{
		TBSkinElement *skin;
		if (m_axis == AXIS_X && (skin = g_tb_skin->GetSkinElement(TBID("TBLayout.fadeout_x"))))
		{
			int bw = skin->bitmap->Width();
			int bh = skin->bitmap->Height();
			if (m_overflow_scroll > 0)
				g_renderer->DrawBitmap(TBRect(padding_rect.x, padding_rect.y, bw, padding_rect.h), TBRect(0, 0, bw, bh), skin->bitmap);
			if (m_overflow_scroll < m_overflow)
				g_renderer->DrawBitmap(TBRect(padding_rect.x + padding_rect.w - bw, padding_rect.y, bw, padding_rect.h), TBRect(bw, 0, -bw, bh), skin->bitmap);
		}
		else if (m_axis == AXIS_Y && (skin = g_tb_skin->GetSkinElement(TBID("TBLayout.fadeout_y"))))
		{
			int bw = skin->bitmap->Width();
			int bh = skin->bitmap->Height();
			if (m_overflow_scroll > 0)
				g_renderer->DrawBitmap(TBRect(padding_rect.x, padding_rect.y, padding_rect.w, bh), TBRect(0, 0, bw, bh), skin->bitmap);
			if (m_overflow_scroll < m_overflow)
				g_renderer->DrawBitmap(TBRect(padding_rect.x, padding_rect.y + padding_rect.h - bh, padding_rect.w, bh), TBRect(0, bh, bw, -bh), skin->bitmap);
		}
	}

	// Restore clipping
	if (m_overflow)
		g_renderer->SetClipRect(old_clip_rect, false);
}

void TBLayout::OnResized(int old_w, int old_h)
{
	InvalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
	ValidateLayout();
}

void TBLayout::GetChildTranslation(int &x, int &y) const
{
	if (m_axis == AXIS_X)
	{
		x = -m_overflow_scroll;
		y = 0;
	}
	else
	{
		x = 0;
		y = -m_overflow_scroll;
	}
}

void TBLayout::ScrollIntoView(const TBRect &rect)
{
	TBRect visible_rect = m_axis == AXIS_X ?	TBRect(m_overflow_scroll, 0, m_rect.w, m_rect.h) :
												TBRect(0, m_overflow_scroll, m_rect.w, m_rect.h);
	int new_x = visible_rect.x;
	int new_y = visible_rect.y;
	if (rect.y < visible_rect.y)
		new_y = rect.y;
	if (rect.x < visible_rect.x)
		new_x = rect.x;
	if (rect.y + rect.h > visible_rect.y + visible_rect.h)
		new_y = rect.y + rect.h - visible_rect.h;
	if (rect.x + rect.w > visible_rect.x + visible_rect.w)
		new_x = rect.x + rect.w - visible_rect.w;

	SetOverflowScroll(m_axis == AXIS_X ? new_x : new_y);
}

void TBLayout::ScrollBy(int &dx, int &dy)
{
	int old_overflow_scroll = m_overflow_scroll;
	SetOverflowScroll(m_axis == AXIS_X ? m_overflow_scroll + dx : m_overflow_scroll + dy);
	if (m_axis == AXIS_X)
		dx -= m_overflow_scroll - old_overflow_scroll;
	else
		dy -= m_overflow_scroll - old_overflow_scroll;
}

}; // namespace tinkerbell