// ================================================================================
// == This file is a part of Tinkerbell UI Toolkit. (C) 2011-2012, Emil Seger�s ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "PStyleEdit.h"
#include "tb_widgets_common.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"
#include <stdio.h>
#include <assert.h>

namespace tinkerbell {

/** Word wrapping test strings
C:\cvshome\git_work\MarmaladeGit\Blind\api
http://my.opera.com/fruxo/albums/
Some things (for example parenthesis) that should behave properly, and commas and points.
There are also white-space and 
	int apa = 100;
Please enter password:
*/

/*
PRIO:
  -mer lightweight
  -import systemed g�ras om (copy/paste support osv)
  -all single line widgets ska anv�nda den f�r str�ngar.

centrering/left/h�ger ska finnas i PBlock(/paragraph). Default det som �r p� hela edit.
*/
// Ny �r PStyle *en* EMBED_CHAR_CODE. 
//    Ska det vara rich text edit, m�ste den �ven ha stopp kod.
/// Core styleedit ska ej ha Blind dependencys. En Blind del kan ha bitmap, widgetwrapper osv.
/// Add link support!
/// smart whitespace-wrap
/// line numbering
/// justify_left_right (text-align: justify)
/// gotolinenr
/// search
/// epsilon support (dotted text ending...)
/// open source
/// getcontentwidth och getactualcontentwidth (ej p�verkad av justify offsetter)

/// importern m�ste �ven vara ansvarig f�r copy'n'paste.

/// Make PElement a POD object, so any other kind of element need to be some childobject/listener or similar.
///   Then pool it.
///   Also remove x, y etc and use traverse function


/*


Vilken klass som helst med text ska kunna s�tta stilar. Kan d� ha bilder i listor osv.
Ex:
void SetText(const char *text); ///beh�vs ej. Kan finnas PPlainText ocks�.
void SetText(const PFormattedText &text)

SetText(PFormattedText("<b>groda</b>"))
SetText(PFormattedText("<myclass>groda</myclass>")) ////< Inneh�llet i <> mappas med hash till helt utbyggbart stilsystem
SetText(PFormattedText("<myclass color='#fd0'>groda</myclass>")) ////< Kan �ven ta parametrar (f�s i lista till stilen vid s�ttning)
SetText(PFormattedText("groda <img/>")) ////< Escape f�r infoga symbol font?

SetText(PFormattedText("groda \x1B,00")) ////< Escape f�r infoga symbol font? Detta �r en fontsystem feature och inte textlayoutern!?

*/


// highlighter plugin (exempel f�r c++) M�ste k�ras efter editeringar ocks�.

// baseline r�knas ut on the fly n�r man layoutar ett block fr�n b�rjan. N�r raden bryts s� vet man.
// tabort styledit ur pblock och caret.. skicka alltid med den ist�llet?

// pwd f�lt ska ha disc. ej hoppa word-by-word

// dela upp tab, enter i egna element.
// splitta block och s�ttihop dom

// m�ste anv�nda \r\n fr att det ska funka... fixa.

// enterp� sista raden utan radbrytning... krash!

// clearsimilarstyles

// empty implementations. implement!

// FIX: buflen i SetText!

// splittning av block. ihops�ttning. vid paste och alltid.

// wrap to inset of last (overflowing) row.

// dubbelbuffer i ppainter

// vid resize m�ste varje block f� m�jlighet att layoutas. (pga, justify center och horizontalline med enter efter etc..)

// �ndra till getfunktioner i style. G�r en PStyleEdit::ApplyStyleChange reflowar vad som beh�vs.
// ha SetFace, SetSize etc i PStyle s� man kan g�ra t.ex GetStyle(0)->SetSize();

// centrera i y-led ifall singleline.

// xscrollbar in PStyleEditView!

// bool layout i insertstyle etc s� man kan g�ra mycket p� en g�ng och sedan layouta en g�ng.

// import-API't

// dubbelclick ska vara oberoende av stylebyte.
// wordpadprogram.

#if 0 // Enable for some graphical debugging
#define TMPDEBUG(expr) expr
#define nTMPDEBUG(expr)
#else
#define TMPDEBUG(expr) 
#define nTMPDEBUG(expr) expr
#endif

#define ADD_THING(thing, first_thing, last_thing) \
			if (!first_thing) \
				first_thing = thing; \
			if (last_thing) \
			{ \
				last_thing->next = thing; \
				thing->prev = last_thing; \
			} \
			last_thing = thing;

#define INSERT_THING_AFTER(thing, after_this, first_thing, last_thing) \
			if (after_this == NULL) \
			{ \
				if (first_thing) \
					first_thing->prev = thing; \
				thing->next = first_thing; \
				first_thing = thing; \
				if (last_thing == NULL) \
					last_thing = thing; \
			} \
			else \
			{ \
				thing->prev = after_this; \
				thing->next = after_this->next; \
				if (after_this->next) \
				{ \
					after_this->next->prev = thing; \
				} \
				after_this->next = thing; \
				if (thing->next == NULL) \
					last_thing = thing; \
			}

#define REMOVE_THING(thing, first_thing, last_thing) \
			if (thing->prev) \
				thing->prev->next = thing->next; \
			if (thing->next) \
				thing->next->prev = thing->prev; \
			if (thing == first_thing) \
				first_thing = thing->next; \
			if (thing == last_thing) \
				last_thing = thing->prev;

const int TAB_SPACE = 4;
#ifdef _DEBUG
const int8 EMBEDDED_CHARCODE = '�'; //TEMPTEST
#else
const int8 EMBEDDED_CHARCODE = 1;
#endif
const char EMBEDDED_CHARSTR[2] = { EMBEDDED_CHARCODE, 0 };

const char special_char_newln[] = { (char)0xB6, NULL };
const char special_char_space[] = { (char)0xB7, NULL };
const char special_char_tab[] = { (char)0xBB, NULL };
const char special_char_password[] = { (char)'�', NULL };

static bool is_space(int8 c)
{
	switch(c)
	{
		case ' ':
			return true;
	}
	return false;
}

static bool is_linebreak(int8 c)
{
	switch(c)
	{
		case 10:
		case 13:
		case 0:
			return true;
	}
	return false;
}

static bool is_wordbreak(int8 c)
{
	switch(c)
	{
		case EMBEDDED_CHARCODE:
		case 0:
		case 10:
		case 13:
		case '-':
		case '\t':

		// fix: change to ascii ranges
		// fix: second degree. (Chars that break f.ex. wordstep but cannot be placed alone on a new line)

		// C/C++ section
		case '\"':
		case '(':
		case ')':
		case '/':
		case '\\':
		case '*':
		case '+':
		case ',':
		case '.':
		case ';':
		case ':':
		case '>':
		case '<':
		case '&':
		case '#':
		case '!':
		case '=':
		case '[':
		case ']':
		case '{':
		case '}':
		case '^':
			return true;
	}
	return is_space(c);
}

// Fix: anv�nd vid layout: while(!is_never_break_before) while(is_never_break_after)
static bool is_never_break_before(int8 c)
{
	switch(c)
	{
	case ' ':
	case '-':
	case '.':
	case ',':
	case ':':
	case ';':
	case '!':
	case '?':
	case ')':
	case ']':
	case '}':
		return true;
	default:
		return false;
	}
}

static bool is_never_break_after(int8 c)
{
	switch(c)
	{
	case '(':
	case '[':
	case '{':
		return true;
	default:
		return false;
	}
}

static bool GetNextWord(const char *text, int *wordlen, int *seglen)
{
	if (text[0] == EMBEDDED_CHARCODE || text[0] == '\t')
	{
		*wordlen = 1;
		*seglen = 1;
		return text[1] != 0;
	}
	else if (text[0] == 0) // happens when not setting text and maby when setting ""
	{
		*wordlen = 0;
		*seglen = 0;
		return false;
	}
	else if (text[0] == 13 || text[0] == 10)
	{
		int len = (text[0] == 13 && text[1] == 10) ? 2 : 1;
		*wordlen = len;
		*seglen = len;
		return false;
	}
	int i = 0;
	while(!is_wordbreak(text[i]) || i == 0)
		i++;
	*wordlen = i;
	while(is_space(text[i]))
		i++;
	*seglen = i;
	if (text[i] == 0)
		return false;
	return true;
}

int32 ComputeStringWidth(PStyle *style, bool password_on, const char *str, int len)
{
	if (password_on)
	{
		if (len == -1)
			len = strlen(str);
		return style->GetStringWidth(special_char_password, 1) * len;
	}
	else
		return style->GetStringWidth(str, len);
}

// ============================================================================

// == PStyleEditImport ==================================================

bool PStyleEditImport::Load(const char *filename, PStyleEdit *styledit)
{
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
		return false;
	fseek(f, 0, SEEK_END);
	uint32 num_bytes = ftell(f);
	fseek(f, 0, SEEK_SET);

	int8 *str = new int8[num_bytes + 1];
	if (str == NULL)
	{
		fclose(f);
		return false;
	}

	num_bytes = fread(str, 1, num_bytes, f);
	str[num_bytes] = 0;

	fclose(f);

	Parse(str, num_bytes, styledit);

	delete str;
	return true;
}

void PStyleEditImport::Parse(const char *buf, int32 buf_len, PStyleEdit *styledit)
{
	styledit->SetText(buf); // FIX: buflen i SetText!
}

// == PSelection ==================================================

PSelection::PSelection(PStyleEdit *styledit)
	: styledit(styledit)
	, start_block(NULL)
	, stop_block(NULL)
	, start_ofs(0)
	, stop_ofs(0)
{
}

void PSelection::CorrectOrder()
{
	if (start_block == stop_block && start_ofs == stop_ofs)
		SelectNothing();
	else
	{
		if ((start_block == stop_block && start_ofs > stop_ofs) ||
			(start_block != stop_block && start_block->ypos > stop_block->ypos))
		{
			PBlock *tmp1 = start_block; start_block = stop_block; stop_block = tmp1;
			int32 tmp2 = start_ofs; start_ofs = stop_ofs; stop_ofs = tmp2;
		}
	}
}

void PSelection::CopyToClipboard()
{
	if (IsSelected())
	{
		TBStr text;
		if (GetText(text))
			TBClipboard::SetText(text);
	}
}

void PSelection::InvalidateChanged(PBlock *start_block1, int32 start_ofs1, PBlock *stop_block1, int32 stop_ofs1,
									PBlock *start_block2, int32 start_ofs2, PBlock *stop_block2, int32 stop_ofs2)
{
	if (start_block1 == NULL || start_block2 == NULL)
	{
		if (start_block1 == NULL)
			Invalidate();
		return;
	}
	if (start_block1 != start_block2)
	{
		// FIX: this is a lazy implementation. Doesn't really work.
		start_block1->Invalidate();
		start_block2->Invalidate();
		stop_block1->Invalidate();
		stop_block2->Invalidate();
	}
}

void PSelection::Invalidate()
{
	PBlock *block = start_block;
	while(block)
	{
		block->Invalidate();
		if (block == stop_block)
			break;
		block = block->next;
	}
}

void PSelection::Select(PBlock *start_block, int32 start_ofs, PBlock *stop_block, int32 stop_ofs)
{
	Invalidate();
	this->start_block = start_block;
	this->start_ofs = start_ofs;
	this->stop_block = stop_block;
	this->stop_ofs = stop_ofs;
	CorrectOrder();
	Invalidate();
//	InvalidateChanged();
}

void PSelection::Select(const TBPoint &from, const TBPoint &to)
{
	Invalidate();
	styledit->caret.Place(from);
	start_block = styledit->caret.block;
	start_ofs = styledit->caret.ofs;
	styledit->caret.Place(to);
	stop_block = styledit->caret.block;
	stop_ofs = styledit->caret.ofs;
	CorrectOrder();
	Invalidate();
//	InvalidateChanged();
	styledit->caret.UpdateWantedX();
}

void PSelection::SelectToCaret(PBlock *old_caret_block, int32 old_caret_ofs)
{
	Invalidate();
	if (start_block == NULL)
	{
		start_block = old_caret_block;
		start_ofs = old_caret_ofs;
		stop_block = styledit->caret.block;
		stop_ofs = styledit->caret.ofs;
	}
	else
	{
		if (start_block == old_caret_block && start_ofs == old_caret_ofs)
		{
			start_block = styledit->caret.block;
			start_ofs = styledit->caret.ofs;
		}
		else
		{
			stop_block = styledit->caret.block;
			stop_ofs = styledit->caret.ofs;
		}
	}
	CorrectOrder();
	Invalidate();
//	InvalidateChanged();
}

void PSelection::SelectAll()
{
	start_block = styledit->first_block;
	start_ofs = 0;
	stop_block = styledit->last_block;
	stop_ofs = stop_block->str_len;
	Invalidate();
}

void PSelection::SelectNothing()
{
	Invalidate();
	start_block = NULL;
	stop_block = NULL;
}

bool PSelection::IsElementSelected(PElement *elm)
{
	if (!IsSelected())
		return false;
	if (start_block == stop_block)
	{
		if (elm->block != start_block)
			return false;
		if (start_ofs < elm->ofs + elm->len && stop_ofs >= elm->ofs)
			return true;
		return false;
	}
	if (elm->block->ypos > start_block->ypos && elm->block->ypos < stop_block->ypos)
		return true;
	if (elm->block->ypos == start_block->ypos && elm->ofs + elm->len > start_ofs)
		return true;
	if (elm->block->ypos == stop_block->ypos && elm->ofs < stop_ofs)
		return true;
	return false;
}

bool PSelection::IsSelected()
{
	return start_block ? true : false;
}

void PSelection::RemoveContent()
{
	if (!IsSelected())
		return;
	if (start_block == stop_block)
	{
		if (!styledit->undoredo.applying)
			styledit->undoredo.Commit(styledit, styledit->GetGlobalOfs(start_block, start_ofs), stop_ofs - start_ofs, start_block->str.CStr() + start_ofs, false);
		start_block->RemoveContent(start_ofs, stop_ofs - start_ofs);
	}
	else
	{
		// Remove text in first block
		TBStr commit_string;
		int32 start_gofs = 0;
		if (!styledit->undoredo.applying)
		{
			start_gofs = styledit->GetGlobalOfs(start_block, start_ofs);
			commit_string.Append(start_block->str.CStr() + start_ofs, start_block->str_len - start_ofs);
		}
		start_block->RemoveContent(start_ofs, start_block->str_len - start_ofs);

		// Remove text in all block in between start and stop
		PBlock *block = start_block->next;
		while(block != stop_block)
		{
			if (!styledit->undoredo.applying)
				commit_string.Append(block->str);
			PBlock *next = block->next;
			REMOVE_THING(block, styledit->first_block, styledit->last_block);
			delete block;

			block = next;
		}

		// Remove text in last block
		if (!styledit->undoredo.applying)
		{
			commit_string.Append(stop_block->str, stop_ofs);
			styledit->undoredo.Commit(styledit, start_gofs, commit_string.Length(), commit_string, false);
		}
		stop_block->RemoveContent(0, stop_ofs);

		start_block->Merge();
	}
	styledit->caret.Place(start_block, start_ofs);
	styledit->caret.UpdateWantedX();
	SelectNothing();
}

bool PSelection::GetText(TBStr &text)
{
	if (!IsSelected())
	{
		text.Clear();
		return true;
	}
	if (start_block == stop_block)
		text.Append(start_block->str.CStr() + start_ofs, stop_ofs - start_ofs);
	else
	{
		TBTempBuffer buf;
		buf.Append(start_block->str.CStr() + start_ofs, start_block->str_len - start_ofs);
		PBlock *block = start_block->next;
		while(block != stop_block)
		{
			buf.Append(block->str, block->str_len);
			block = block->next;
		}
		buf.Append(stop_block->str, stop_ofs);
		text.Set((char*)buf.GetData(), buf.GetAppendPos());
	}
	//str.ReplaceAll(EMBEDDED_CHARSTR, "");
	return true;
}

// == PStyle ======================================================

PStyle::PStyle()
/*#ifdef _DEBUG
	: font_descriptor(PGetMonospaceFont().descriptor)
#else*/
//	: font_descriptor(PGetMenuFont().descriptor)
//#endif
	: color(0)
	, ref_count(0)
{
	Update();
}

PStyle::PStyle(const PStyle &style)
	: /*font_descriptor(style.font_descriptor)
	, */color(style.color)
	, ref_count(0)
{
	Update();
}

PStyle::~PStyle()
{
}

void PStyle::IncRef()
{
	ref_count++;
}

void PStyle::DecRef()
{
	ref_count--;
	if (ref_count == 0)
		delete this;
}

void PStyle::Update()
{
	//font.Set(font_descriptor);
}

int32 PStyle::GetStringWidth(const char *str, int len)
{
	return g_renderer->GetStringWidth(str, len);
}

int32 PStyle::GetTabWidth()
{
	return g_renderer->GetStringWidth("x", 1) * TAB_SPACE;
}

int32 PStyle::GetHeight()
{
	return g_renderer->GetFontHeight();
}

int32 PStyle::GetBaseline()
{
	// FIX: font ascent!
	return (int32)(g_renderer->GetFontHeight() * 0.75f);/*font.info.ascent*/
}

// == PElement ======================================================

// ============================================================================

PCaret::PCaret(PStyleEdit *styledit)
	: styledit(styledit)
	, block(NULL)
	, x(0)
	, y(0)
	, width(2)
	, height(0)
	, ofs(0)
	, on(false)
	, wanted_x(0)
	, prefer_first(true)
{
}

void PCaret::Invalidate()
{
	if (styledit->listener)
		styledit->listener->Invalidate(TBRect(x - styledit->scroll_x, y - styledit->scroll_y, width, height));
}

void PCaret::UpdatePos()
{
	Invalidate();
	PElement *element = GetElement();
	x = element->xpos + element->GetCharX(ofs - element->ofs);
	y = element->ypos + block->ypos;
	height = element->GetHeight();
	Invalidate();
}

/// hoppa �ver embed characters och \r\n vilken det nu �r.
/// ta bort \r\n fr�n block str�ngarna!?

bool PCaret::Move(bool forward, bool word)
{
	// Make it stay on the same line if it reach the wrap point.
	prefer_first = forward;
	if (this->styledit->packed.password_on)
		word = false;

	int len = block->str_len;
	if (word && !(forward && ofs == len) && !(!forward && ofs == 0))
	{
		const char *str = block->str;
		if (forward)
		{
			while (ofs < len && !is_wordbreak(str[ofs]))
				ofs++;
			while (ofs < len && is_wordbreak(str[ofs]))
				ofs++;
		}
		else if (ofs > 0)
		{
			if (is_wordbreak(str[ofs - 1]))
				while (ofs > 0 && is_wordbreak(str[ofs - 1]))
					ofs--;
			while (ofs > 0 && !is_wordbreak(str[ofs - 1]))
				ofs--;
		}
	}
	else
	{
		// Avoid skipping the first/last character when wrapping to a new box.
		ofs += forward ? 1 : -1;
		if (ofs > block->str_len && block->next)
		{
			block = block->next;
			ofs = 0;
		}
		if (ofs < 0 && block->prev)
		{
			block = block->prev;
			ofs = block->str_len;
		}
	}
	return Place(block, ofs, true, forward);
}

bool PCaret::Place(const TBPoint &point)
{
	PBlock *block = styledit->FindBlock(point.y);
	PElement *element = block->FindElement(point.x, point.y - block->ypos);
	int ofs = element->ofs + element->GetCharOfs(point.x - element->xpos);

	if (Place(block, ofs))
	{
		if (GetElement() != element)
		{
			prefer_first = !prefer_first;
			Place(block, ofs);
		}
		return true;
	}
	return false;
}

void PCaret::Place(P_CARET_POS place)
{
	if (place == P_CARET_POS_BEGINNING)
		Place(styledit->first_block, 0);
	else if (place == P_CARET_POS_END)
		Place(styledit->last_block, styledit->last_block->str_len);
}

bool PCaret::Place(PBlock *block, int ofs, bool allow_snap, bool snap_forward)
{
	if (block)
	{
		while (block->next && ofs > block->str_len)
		{
			ofs -= block->str_len;
			block = block->next;
		}
		while (block->prev && ofs < 0)
		{
			block = block->prev;
			ofs += block->str_len;
		}
		if (ofs < 0)
			ofs = 0;
		if (ofs > block->str_len)
			ofs = block->str_len;

		// Avoid being inside linebreak
		/*if (allow_snap)
		{
			PElement *element = block->FindElement(ofs);
			if (ofs > element->ofs && element->IsBreak())
			{
				if (snap_forward && block->next)
				{
					block = block->next;
					ofs = 0;
				}
				else
					ofs = element->ofs;
			}
		}*/
	}

	bool changed = (this->block != block || this->ofs != ofs);
	this->block = block;
	this->ofs = ofs;

	if (block)
		UpdatePos();

	return changed;
}

void PCaret::AvoidLineBreak()
{
	PElement *element = GetElement();
	if (ofs > element->ofs && element->IsBreak())
		ofs = element->ofs;
	UpdatePos();
}

void PCaret::Paint(int32 translate_x, int32 translate_y)
{
//	if (on && !(styledit->select_state && styledit->selection.IsSelected()))
	if (on || styledit->select_state)
	{
		styledit->listener->DrawCaret(TBRect(translate_x + x, translate_y + y, width, height));
	}
}

void PCaret::ResetBlink()
{
	styledit->listener->CaretBlinkStop();
	on = true;
	styledit->listener->CaretBlinkStart();
}

void PCaret::UpdateWantedX()
{
	wanted_x = x;
}

PElement *PCaret::GetElement()
{
	return block->FindElement(ofs, prefer_first);
}

void PCaret::SwitchBlock(bool second)
{
}

int32 PCaret::GetGlobalOfs()
{
	return styledit->GetGlobalOfs(block, ofs);
}

void PCaret::SetGlobalOfs(int32 gofs)
{
	PBlock *b = styledit->first_block;
	while (b)
	{
		int b_len = b->str_len;
		if (gofs <= b_len)
		{
			Place(b, gofs);
			return;
		}
		gofs -= b_len;
		b = b->next;
	}
	assert(!"out of range! not a valid global offset!");
}

// ============================================================================

PBlock::PBlock(PStyleEdit *styledit)
	: styledit(styledit)
	, prev(NULL)
	, next(NULL)
	, first_element(NULL)
	, last_element(NULL)
	, special_elements()
	, ypos(0)
	, height(0)
	, align(styledit->align)
	, extra_data(0)
	, str_len(0)
{
}

PBlock::~PBlock()
{
	Clear();
	special_elements.DeleteAll();
}

void PBlock::Clear()
{
	while(first_element)
	{
		PElement *element = first_element;
		first_element = first_element->next;
		if (!element->IsEmbedded()) // Stored in the special_elements list.
			delete element;
		else
		{
			// Might be one of the elements removed, so we must clear pointers.
			// They will be updated again in Layout.
			element->prev = NULL;
			element->next = NULL;
		}
	}
	first_element = NULL;
	last_element = NULL;
}

void PBlock::Set(const char *newstr, int32 len)
{
	str.Set(newstr, len);
	str_len = len;
	Split();
	Layout(true);
}

void PBlock::SetAlign(TB_TEXT_ALIGN align)
{
	if (this->align == align)
		return;
	this->align = align;
	Layout(false);
}

int32 PBlock::InsertText(int32 ofs, const char *text, int32 len, bool allow_embeds, bool allow_line_recurse)
{
	int first_line_len = len;
	for(int i = 0; i < len; i++)
		if (text[i] == 13 || text[i] == 10)
		{
			first_line_len = i;
			// Include the line break too but not for single lines
			if (!styledit->packed.multiline_on)
				break;
			if (text[i] == 13 && text[i + 1] == 10)
				first_line_len++;
			first_line_len++;
			break;
		}

	int32 inserted_len = first_line_len;
	str.Insert(ofs, text, first_line_len);
	str_len += first_line_len;

	// Sanitize inserted text (make sure it's not using our special embed char)
	if (!allow_embeds)
	{
		for(int i = ofs; i < ofs + first_line_len; i++)
			if (str.CStr()[i] == EMBEDDED_CHARCODE)
				str.CStr()[i] = ' ';
	}

	Split();
	Layout(true);

	// Add the rest which was after the linebreak.
	if (allow_line_recurse && styledit->packed.multiline_on)
	{
		// Instead of recursively calling InsertText, we will loop through them all here
		PBlock *next_block = next;
		const char *next_line_ptr = &text[first_line_len];
		int remaining = len - first_line_len;
		while (remaining > 0)
		{
			if (!next_block)
			{
				next_block = new PBlock(styledit);
				ADD_THING(next_block, styledit->first_block, styledit->last_block);
			}
			int consumed = next_block->InsertText(0, next_line_ptr, remaining, allow_embeds, false);
			next_line_ptr += consumed;
			inserted_len += consumed;
			remaining -= consumed;
			next_block = next_block->next;
		}
	}
	return inserted_len;
}

void PBlock::InsertStyle(int32 ofs, int32 styleid)
{
	PStyleSwitcherElement *se = new PStyleSwitcherElement();
	se->styleid = styleid;
	PElement *e = new PElement(se);
	InsertEmbedded(ofs, e);
}

void PBlock::InsertEmbedded(int32 ofs, PElement *element)
{
// FIX:
/// nu blir det ett tomt textblock f�re varje embed. M�ste modifiera ::Layout
//undioredo m�ste hantera embeds
//clipboard ocks�, genom formatet
// Nu kan man inte g�ra separat traverse. Fragmenteringen, wordwrappingen och layouten h�nger ihop.

	element->Init(this, ofs, 1); // move into layout and add xpos, ypos and block and everything that needs initiation!!!!

	int i, index = 0;
	for(i = 0; i < ofs; i++)
		if (str.CStr()[i] == EMBEDDED_CHARCODE)
			index++;

	special_elements.Add(element, index);

	InsertText(ofs, EMBEDDED_CHARSTR, 1, true);
	//Layout(); InsertText will layout for us.
}

int CountSpecialBefore(const char *str, int ofs)
{
	int j, before = 0;
	for(j = 0; j < ofs; j++)
		if (str[j] == EMBEDDED_CHARCODE)
			before++;
	return before;
}

void PBlock::RemoveContent(int32 ofs, int32 len)
{
	if (special_elements.GetNumItems())
	{
		// den sista styleswitchern b�r l�ggas efter det borttagna blocket fast inte om slutet tas bort.
		// obs. om den kommer mergas med n�sta block s� "tas inte slutet bort"!
		// obs. inte vanliga ekbeddeds.
		PElement *last_style = NULL;
		int i, index = 0;
		for(i = 0; i < ofs + len; i++)
		{
			if (str.CStr()[i] == EMBEDDED_CHARCODE)
			{
				PElement *element = special_elements[index];
				if (i >= ofs)
				{
					if (element->IsStyleSwitcher())
					{
						if (last_style)
						{
							REMOVE_THING(last_style, first_element, last_element);
							delete last_style;
						}
						last_style = element;
						special_elements.Remove(index--);
					}
					else
					{
						REMOVE_THING(element, first_element, last_element);
						special_elements.Delete(index--);
					}
				}
				index++;
			}
		}
		if (last_style)
		{
			if (ofs + len >= str_len)
			{
				REMOVE_THING(last_style, first_element, last_element);
				delete last_style;
			}
			else
			{
				int before = CountSpecialBefore(str, ofs);
				special_elements.Add(last_style, before);
				str.Insert(ofs + len, EMBEDDED_CHARSTR, 1);
				str_len += 1;
			}
		}
	}
	str.Remove(ofs, len);
	str_len -= len;
//	CleanupWastedStyles(); // ALSO CALL FROM INSERTSTYLE
	Layout(true);
}

void PBlock::Split()
{
	int32 len = str_len;
	int brlen = 1; // FIX: skip ending newline element but not if there is several newlines and check for singleline newline.
	if (len > 1 && str.CStr()[len - 2] == 13 && str.CStr()[len - 1] == 10)
		brlen++;
	len -= brlen;
	for(int i = 0; i < len; i++)
	{
		if (is_linebreak(str.CStr()[i]))
		{
			PBlock *block = new PBlock(styledit);
			INSERT_THING_AFTER(block, this, styledit->first_block, styledit->last_block);

			i++;
			if (str.CStr()[i] == 10)
				i ++;

			// Move all special_elements after i, to the next block.
			int j, count = special_elements.GetNumItems();
			int before = CountSpecialBefore(str, i);
			for(j = before; j < count; j++)
			{
				PElement *element = special_elements.Remove(before);
				element->prev = NULL;
				element->next = NULL;
				block->special_elements.Add(element);
			}

			//
			len = len + brlen - i;
			block->Set(str.CStr() + i, len);
			str.Remove(i, len);
			str_len -= len;
			break;
		}
	}
}

void PBlock::Merge()
{
	// Vid borttagning av selection �ver flera block:
	//    Ta bort block "emellan"
	//    Ta bort text ur stop_block layouta ej. updatera sel_stop
	//    Ta bort text ur start_block layouta ej. nollst�ll selection.
	//    K�r Merge. Layouta.
	if (next && !last_element->IsBreak())
	{
		str.Append(next->str);
		str_len = str.Length();
		//Fix: add all embeddeds & DONT remove them from the following block when deleted!!!

		next->SetSize(0, 0, true); // Propagate height of the following, before we delete it.
		PBlock *tmp = next;
		REMOVE_THING(tmp, styledit->first_block, styledit->last_block);
		delete tmp;

		Layout(true);
	}
}

int32 GetTabWidth(int32 xpos, PStyle *style)
{
	int tabsize = style->GetTabWidth();
	int p2 = int(xpos / tabsize) * tabsize + tabsize;
	return p2 - xpos;
}

int PBlock::GetStartIndentation()
{
	// Lines beginning with whitespace or list points, should
	// indent to the same as the beginning when wrapped.
	PStyle *style = styledit->GetStyle(0);
	int indentation = 0;
	const char *start = str;
	const char *current = start;
	while (current < str.CStr() + str_len)
	{
		switch (*current)
		{
		case '\t':
			indentation += GetTabWidth(indentation, style);
			current++;
			continue;
		case ' ':
		case '-':
		case '*':
		case '�':
			indentation += ComputeStringWidth(style, styledit->packed.password_on, str, 1);
			current++;
			continue;
		};
		break;
	}
	return indentation;
}

void PBlock::AdjustElementPosition(PElement *start_element, int32 line_height, int32 line_baseline, int32 line_w)
{
	int32 xofs = 0;
	if (align == TB_TEXT_ALIGN_RIGHT)
		xofs = styledit->layout_width - line_w;
	else if (align == TB_TEXT_ALIGN_CENTER)
		xofs = (styledit->layout_width - line_w) / 2;

	while (start_element)
	{
		start_element->line_ypos = start_element->ypos;
		start_element->line_height = line_height;
		start_element->ypos += line_baseline - start_element->GetBaseline();
		start_element->xpos += xofs;
		start_element = start_element->next;
	}
}

void PBlock::Layout(bool propagate_height)
{
	// FIX: Now we're forced to layout even when styledit->layout_width is <= 0 (which it always is
	// when setting text before layout. If we created text fragments independently of layout, we could
	// fix this.
	Clear();
	PStyle *style = styledit->GetStyle(0);

	PElement *line_start_element = NULL;
	int special_index = 0;
	int ofs = 0;
	int line_ofs = 0;
	int line_w = 0;
	int xpos = 0;
	int line_w_max = 0;

	int line_height = 0;
	int line_baseline = 0;
	int line_ypos = 0;
	int first_line_indentation = 0;

	const char *text = str;
	while(true)
	{
		int wordlen, seglen;
		bool more = GetNextWord(&text[ofs], &wordlen, &seglen);
		bool is_embedded = text[ofs] == EMBEDDED_CHARCODE;
		bool is_tab = text[ofs] == '\t';
		bool is_br = text[ofs] == '\n' || text[ofs] == '\r';

		int word_w = 0, seg_w = 0, element_height = 0, element_baseline = 0;

		PElement *special_element = NULL;
		if (is_br)
		{
			word_w = seg_w = 0;
		}
		else if (is_tab)
		{
			seg_w = word_w = GetTabWidth(xpos + line_w, style);
		}
		else if (is_embedded)
		{
			special_element = special_elements[special_index++];
			word_w = special_element->GetWidth();
			seg_w = special_element->GetWidth();
		}
		else
		{
			word_w = ComputeStringWidth(style, styledit->packed.password_on, &text[ofs], wordlen);
			seg_w = ComputeStringWidth(style, styledit->packed.password_on, &text[ofs], seglen);
		}
		bool overflow = (styledit->packed.wrapping && line_w + word_w > styledit->layout_width);

		if (overflow && line_w == first_line_indentation)
			overflow = false; // Word is wider than the entire line so let it continue.
		/*if (overflow && is_never_break_before(text[ofs]))
		{
			// We should actually keep track of the last wrappable position, and when
			// we overflow, we should wrap there. We should also check is_ever_break_after.
			// This is a quick hack.
			assert(ofs > 0);
			overflow = false;
		}*/

		if (is_embedded || !more || overflow || is_tab)
		{
			// Commit everything before the last parsed element
			element_height = style->GetHeight();
			element_baseline = style->GetBaseline();
			line_height = MAX(element_height, line_height);
			line_baseline = MAX(element_baseline, line_baseline);

			// bug: flera tabbar i rad vid radslut (overflow) g�r att en ritas p� b�da st�llena!

			// blir lite fel. (ett extra element med en "\0" str�ng ifall f�rsta �r en embedded.
			int line_len = ofs - line_ofs;
			if (!special_element || line_len)
			{
				if (!more && !overflow && !is_br)
					line_len += seglen;

				PElement *element = new PElement();
				element->Init(this, line_ofs, line_len);
				element->xpos = xpos;
				element->ypos = line_ypos;

				ADD_THING(element, first_element, last_element);
			}

			xpos = line_w;
			line_ofs = ofs;

			if (is_br || is_tab)
			{
				PElement *element = new PElement();
				element->Init(this, line_ofs, seglen);
				element->xpos = xpos;
				element->ypos = line_ypos;

				ADD_THING(element, first_element, last_element);
				if (is_tab)
				{
					line_ofs += 1;
					xpos += word_w;
				}
			}
			if (overflow || !more)
			{
				AdjustElementPosition(line_start_element ? line_start_element->next : first_element,
										line_height, line_baseline, overflow ? line_w : line_w + word_w);
				line_start_element = last_element;
				line_baseline = 0;
			}

			bool first_line = (line_ypos == 0);
			if (!more && !overflow && !is_embedded)
				line_ypos += line_height;

			if (overflow)
			{
				line_ypos += line_height;
				line_height = 0;
				//if (0) // !inset_wrap
				//only if we have more, but not the incorrect more flag
				{
					if (first_line)
					{
						first_line_indentation = GetStartIndentation();
					}
					xpos = line_w = first_line_indentation;
				}
				if (!more)
				{
					// Must reprocess this segment:
					more = true;
					seg_w = 0;
					seglen = 0;
				}
			}

			// If it was a overflow, the text for the current element will be added next round, but specials
			// must be inserted directly now.
			if (is_embedded)
			{
				element_height = special_element->GetHeight();
				element_baseline = special_element->GetBaseline();

				line_height = MAX(element_height, line_height);
				line_baseline = MAX(element_baseline, line_baseline);

				ADD_THING(special_element, first_element, last_element);
				special_element->block = this;
				special_element->ofs = line_ofs;
				special_element->xpos = xpos;
				special_element->ypos = line_ypos;
				if (special_element->IsStyleSwitcher())
					style = styledit->GetStyle((static_cast<PStyleSwitcherElement *>(special_element->content))->styleid);
				line_ofs += 1;
				xpos += word_w;
			}
		}

		line_w += seg_w;
		ofs += seglen;

		int ending_w = seg_w - word_w;
		if (line_w - ending_w > line_w_max)
			line_w_max = line_w - ending_w;

		if (!more)
			break;
	}
	if (line_height == 0)
		line_height = style->GetHeight();
	ypos = prev ? prev->ypos + prev->height : 0;
	SetSize(line_w_max, line_ypos, propagate_height);

	Invalidate();
}

void PBlock::SetSize(int32 new_w, int32 new_h, bool propagate_height)
{
	// Later: could optimize with Scroll here.
	int32 dh = new_h - height;
	height = new_h;
	if (dh != 0 && propagate_height)
	{
		PBlock *block = next;
		while(block)
		{
			block->ypos = block->prev->ypos + block->prev->height;
			block->Invalidate();
			block = block->next;
		}
	}

	int dbottom = styledit->GetContentHeight() - styledit->content_height;
	if (dbottom < 0)
		styledit->listener->Invalidate(TBRect(0, styledit->last_block->ypos, styledit->layout_width, styledit->last_block->height + -dbottom));

	if (dbottom != 0 && styledit->listener)
		styledit->listener->UpdateScrollbars();

	if (!styledit->packed.wrapping && !styledit->packed.multiline_on)
		styledit->content_width = new_w;
	else if (new_w > styledit->content_width)
		styledit->content_width = new_w;
	styledit->content_height = styledit->GetContentHeight();
}

PElement *PBlock::FindElement(int32 ofs, bool prefer_first)
{
	PElement *element = first_element;
	while(element)
	{
		if (prefer_first && ofs <= element->ofs + element->len)
			return element;
		if (!prefer_first && ofs < element->ofs + element->len)
			return element;
		element = element->next;
	}
	return last_element;
}

PElement *PBlock::FindElement(int32 x, int32 y)
{
	PElement *element = first_element;
	while(element)
	{
		if (y < element->line_ypos + element->line_height)
		{
			if (x < element->xpos + element->GetWidth())
				return element;
			if (element->next && element->next->line_ypos > element->line_ypos)
				return element;
		}
		element = element->next;
	}
	return last_element;
}

void PBlock::Invalidate()
{
	if (styledit->listener)
		styledit->listener->Invalidate(TBRect(0, - styledit->scroll_y + ypos, styledit->layout_width, height));
}

void PBlock::Paint(int32 translate_x, int32 translate_y)
{
	styledit->listener->DrawBackground(TBRect(translate_x + styledit->scroll_x, translate_y + ypos, styledit->layout_width, height), this);
	TMPDEBUG(styledit->listener->DrawRect(TBRect(translate_x, translate_y + ypos, styledit->layout_width, height), 255, 200, 0, 128));
	PElement *element = first_element;
//int count = 0;
	while(element)
	{
		element->Paint(translate_x, translate_y + ypos);
		//FIX: break if we reach the edge of visibility
		//if (translate_y >
		//if (!styledit->packed.wrapping && translate_x > 
		//break;
//count++;
		element = element->next;
	}
//	TBStr tmp;
//	tmp << special_elements.GetNumItems() << " " << count << " " << str_len;
//	styledit->listener->DrawString(styledit->layout_width - 50, translate_y + ypos, tmp, tmp.Length());
}

// ============================================================================

void PElement::Init(PBlock *block, uint16 ofs, uint16 len)
{
	this->block = block; this->ofs = ofs; this->len = len;
}

void PElement::Paint(int32 translate_x, int32 translate_y)
{
	PStyleEditListener *listener = block->styledit->listener;
	PStyle *style = GetStyle();

	int x = translate_x + xpos;
	int y = translate_y + ypos;

	if (content)
	{
		content->Paint(this, translate_x, translate_y);
		if (block->styledit->selection.IsElementSelected(this))
			listener->DrawContentSelectionFg(TBRect(x, y, GetWidth(), GetHeight()));
		return;
	}
	TMPDEBUG(listener->DrawRect(TBRect(x, y, GetWidth(), style->GetHeight()), 255, 255, 255, 128));

	if (block->styledit->selection.IsElementSelected(this))
	{
		PSelection *sel = &block->styledit->selection;

		int sofs1 = sel->start_block == block ? sel->start_ofs : 0;
		int sofs2 = sel->stop_block == block ? sel->stop_ofs : block->str_len;
		sofs1 = MAX(sofs1, ofs);
		sofs2 = MIN(sofs2, ofs + len);

		int s1x = GetStringWidth(style, block->str.CStr() + ofs, sofs1 - ofs);
		int s2x = GetStringWidth(style, block->str.CStr() + sofs1, sofs2 - sofs1);

		listener->DrawTextSelectionBg(TBRect(x + s1x, y, s2x, GetHeight()));
	}

	listener->SetStyle(style);
	if (block->styledit->packed.password_on)
	{
		int cw = style->GetStringWidth(special_char_password, 1);
		for(int i = 0; i < len; i++)
			listener->DrawString(x + i * cw, y, special_char_password, 1);
	}
	else if (IsTab())
	{
		if (block->styledit->packed.show_whitespace)
			listener->DrawString(x, y, special_char_tab, 1);
	}
	else if (IsBreak())
	{
		if (block->styledit->packed.show_whitespace)
			listener->DrawString(x, y, special_char_newln, len);
	}
	else
		listener->DrawString(x, y, Str(), len);
}

void PElement::Click(int button, uint32 modifierkeys)
{
	if (content)
		content->Click(this, button, modifierkeys);
}

int32 PElement::GetWidth()
{
	if (content)
		return content->GetWidth(this);
	return GetStringWidth(GetStyle(), block->str.CStr() + ofs, len);
}

int32 PElement::GetHeight()
{
	if (content)
		return content->GetHeight(this);
	return GetStyle()->GetHeight();
}

int32 PElement::GetBaseline()
{
	if (content)
		return content->GetBaseline(this);
	return GetStyle()->GetBaseline();
}

int32 PElement::GetCharX(int32 ofs)
{
	assert(ofs >= 0 && ofs <= len);
	if (content)
		return ofs == 1 ? content->GetWidth(this) : 0;
	return GetStringWidth(GetStyle(), block->str.CStr() + this->ofs, ofs);
}

int32 PElement::GetCharOfs(int32 x)
{
	if (IsEmbedded() || IsTab())
		return x > GetWidth() / 2 ? 1 : 0;
	if (IsBreak())
		return 0;

	PStyle *style = GetStyle();
	const char *str = block->str.CStr() + ofs;
	for(int i=0; i<len; i++)
	{
		int w = GetStringWidth(style, &str[0], i);
		int cw = GetStringWidth(style, &str[i], 1);
		if (x < w + cw / 2)
			return i;
	}
	return len;
}

PStyle *PElement::GetStyle()
{
	int32 styleid = 0;
	PElement *element = prev;
	while(element)
	{
		if (element->IsStyleSwitcher())
		{
			styleid = ((PStyleSwitcherElement*)element->content)->styleid;
			break;
		}
		element = element->prev;
	}
	return block->styledit->GetStyle(styleid);
}

bool PElement::IsBreak()
{
	return Str()[0] == '\r' || Str()[0] == '\n';
}

bool PElement::IsSpace()
{
	return is_space(Str()[0]);
}

bool PElement::IsTab()
{
	return Str()[0] == '\t';
}

int32 PElement::GetStringWidth(PStyle *style, const char *str, int len)
{
	if (IsTab())
		return len == 1 ? GetTabWidth(xpos, style) : 0;
	if (IsBreak())
		return len == 0 ? 0 : 8;
	return ComputeStringWidth(style, block->styledit->packed.password_on, str, len);
}

// ============================================================================

/*PImageElement::PImageElement(PBitmap *bitmap, bool needfree)
	: bitmap(bitmap)
	, needfree(needfree)
{
}

PImageElement::~PImageElement()
{
	if (needfree)
		delete bitmap;
}

void PImageElement::Paint(PElement *element, int32 translate_x, int32 translate_y)
{
	int x = translate_x + element->xpos;
	int y = translate_y + element->ypos;
	PStyleEditListener *listener = element->block->styledit->listener;
	listener->DrawBitmap(x, y, bitmap);
}

int32 PImageElement::GetWidth(PElement *element) { return bitmap->width; }

int32 PImageElement::GetHeight(PElement *element) { return bitmap->height; }*/

// ============================================================================

/*PHorizontalLineElement::PHorizontalLineElement(int32 width_in_percent, int32 height, uint32 color)
	: width_in_percent(width_in_percent)
	, height(height)
	, color(color)
{
}

PHorizontalLineElement::~PHorizontalLineElement() {}

void PHorizontalLineElement::Paint(PElement *element, int32 translate_x, int32 translate_y)
{
	int x = translate_x + element->xpos;
	int y = translate_y + element->ypos;

	int w = element->block->styledit->layout_width * width_in_percent / 100;
	x += (element->block->styledit->layout_width - w) / 2;

	PStyleEditListener *listener = element->block->styledit->listener;
	listener->DrawRect(TBRect(x, y, w, height), color);
}

int32 PHorizontalLineElement::GetWidth(PElement *element) { return element->block->styledit->layout_width; }

int32 PHorizontalLineElement::GetHeight(PElement *element) { return height; }*/

// ============================================================================

PStyleEdit::PStyleEdit()
	: listener(NULL)
	, layout_width(0)
	, layout_height(0)
	, content_width(0)
	, content_height(0)
	, caret(this)
	, selection(this)
	, select_state(0)
	, mousedown_element(NULL)
	, scroll_x(0)
	, scroll_y(0)
	, align(TB_TEXT_ALIGN_LEFT)
	, packed_init(0)
	, on_enter_message(0)
	, on_change_message(0)
	, first_block(NULL)
	, last_block(NULL)
{
	packed.enabled = true;
	packed.multiline_on = false;
	packed.wrapping = false;
#ifdef _DEBUG
	packed.show_whitespace = true;
#endif

	PStyle* style = new PStyle();
	style->IncRef();
	styles.Add(style);

	Clear();
}

PStyleEdit::~PStyleEdit()
{
	listener->CaretBlinkStop();
	Clear(false);
	for(int i = 0; i < styles.GetNumItems(); i++)
		GetStyle(i)->DecRef();
}

void PStyleEdit::SetListener(PStyleEditListener *listener)
{
	this->listener = listener;
}

void PStyleEdit::Clear(bool init_new)
{
	undoredo.Clear(true, true);
	selection.SelectNothing();

	while(first_block)
	{
		PBlock *block = first_block;
		first_block = first_block->next;
		block->Invalidate();
		delete block;
	}

	first_block = NULL;
	last_block = NULL;

	if (init_new)
	{
		first_block = new PBlock(this);
		last_block = first_block;
		first_block->Set("", 0);
	}

	caret.Place(first_block, 0);
	caret.UpdateWantedX();
}

void PStyleEdit::ScrollIfNeeded(bool x, bool y)
{
	int32 newx = scroll_x, newy = scroll_y;
	if (x)
	{
		if (caret.x - scroll_x < 0)
			newx = caret.x;
		if (caret.x + caret.width - scroll_x > layout_width)
			newx = caret.x + caret.width - layout_width;
	}
	if (y)
	{
		if (caret.y - scroll_y < 0)
			newy = caret.y;
		if (caret.y + caret.height - scroll_y > layout_height)
			newy = caret.y + caret.height - layout_height;
	}
	SetScrollPos(newx, newy);
}

void PStyleEdit::SetScrollPos(int32 x, int32 y)
{
	x = MIN(x, GetContentWidth() - layout_width);
	y = MIN(y, GetContentHeight() - layout_height);
	x = MAX(x, 0);
	y = MAX(y, 0);
	if (!packed.multiline_on)
		y = 0;
	int dx = scroll_x - x;
	int dy = scroll_y - y;
	if (dx || dy)
	{
		scroll_x = x;
		scroll_y = y;
		listener->Scroll(dx, dy);
	}
}

void PStyleEdit::SetLayoutSize(int32 width, int32 height)
{
	if (width == layout_width && height == layout_height)
		return;

	bool reformat = layout_width != width;
	layout_width = width;
	layout_height = height;

	if (reformat && (packed.wrapping || align != TB_TEXT_ALIGN_LEFT))
		Reformat();

	caret.UpdatePos();
	caret.UpdateWantedX();

	SetScrollPos(scroll_x, scroll_y); ///< Trig a bounds check (scroll if outside)
}

void PStyleEdit::Reformat()
{
	PBlock *block = first_block;
	while(block)
	{
		block->Layout(false);
		block = block->next;
	}

	// Magic to propagate heights.
	int h = first_block->height;
	first_block->SetSize(0, 0, false);
	first_block->SetSize(0, h, true);

	int end_y = last_block->ypos + last_block->height - scroll_y;
	int tmp = MAX(0, layout_height - end_y);  // FIX THERE IS A BUG!
	listener->Invalidate(TBRect(0, end_y, layout_width, tmp));
}

int32 PStyleEdit::GetContentWidth()
{
	return content_width;
}

int32 PStyleEdit::GetContentHeight()
{
	return last_block->ypos + last_block->height;
}

void PStyleEdit::Paint(const TBRect &rect)
{
	PBlock *block = first_block;

	while(block)
	{
		if (block->ypos - scroll_y > rect.y + rect.h)
			break;
		if (block->ypos + block->height - scroll_y >= 0)
			block->Paint(-scroll_x, -scroll_y);

		block = block->next;
	}
	if (!block)
	{
		int end_y = last_block->ypos + last_block->height - scroll_y;
		listener->DrawBackground(TBRect(0, end_y, layout_width, layout_height - end_y), NULL);
	}

	caret.Paint(- scroll_x, - scroll_y);
}

void PStyleEdit::InsertText(const char *text, int32 len, bool after_last, bool clear_undo_redo)
{
	if (len == -1)
		len = strlen(text);

	selection.RemoveContent();

	if (after_last)
		caret.Place(last_block, last_block->str_len, false);

	int32 len_inserted = caret.block->InsertText(caret.ofs, text, len, false);
	if (clear_undo_redo)
		undoredo.Clear(true, true);
	else
		undoredo.Commit(this, caret.GetGlobalOfs(), len_inserted, text, true);

	caret.Place(caret.block, caret.ofs + len);
	caret.UpdatePos();
	caret.UpdateWantedX();
}

void PStyleEdit::InsertStyle(PStyle *style, bool after_last)
{
	int styleid = styles.GetNumItems();
	for(int i = 0; i < styles.GetNumItems(); i++)
		if (GetStyle(i) == style)
		{
			styleid = i;
			break;
		}
	if (styleid == styles.GetNumItems())
	{
		style->IncRef();
		styles.Add(style);
	}

	if (after_last)
		caret.Place(last_block, last_block->str_len, false);

	// Se till att s�kra selection och caret pekare/offsetter n�r block �ndras eller tas bort!
	// G�r s� caret endast modifierar start och stop selection till samma ist�llet!
	// must first ensure that start is before end. Ha anchor och caret, och en getfirstpoint och getlastpoint.
	//
	if (selection.IsSelected())
	{
		// Ha en Block::GetStyle(ofs, preferfirst)
		// FIX: iterera �ver alla block mellan start och stop.
		//      m�ste �ven ta h�nsyn till andra stilar av samma sort
		// FIX: Style m�ste vara inkrementell! dvs. pusha poppa antingen font, f�rg, b�datv� osv. S� man kan ha <b>foo <i>bar</i> far</b>
		caret.Place(selection.stop_block, selection.stop_ofs);
		caret.block->InsertStyle(caret.ofs, 0);
		caret.Place(selection.start_block, selection.start_ofs);
		caret.block->InsertStyle(caret.ofs, styleid);
		selection.stop_ofs++;
		selection.stop_ofs++;
	}
	else
	{
		caret.block->InsertStyle(caret.ofs, styleid);

		caret.Place(caret.block, caret.ofs + 1);
		caret.UpdatePos();
		caret.UpdateWantedX();
	}
}

void PStyleEdit::InsertEmbedded(PElementContent *content, bool after_last)
{
	selection.RemoveContent();

	if (after_last)
		caret.Place(last_block, last_block->str_len, false);

	PElement *element = new PElement(content);
	caret.block->InsertEmbedded(caret.ofs, element);
	caret.Move(true, false);
}

PBlock *PStyleEdit::FindBlock(int32 y)
{
	PBlock *block = first_block;
	while(block)
	{
		if (y < block->ypos + block->height)
			return block;
		block = block->next;
	}
	return last_block;
}

uint32 PStyleEdit::GetGlobalOfs(PBlock *block, int32 ofs)
{
	int32 gofs = 0;
	PBlock *b = first_block;
	while (b && b != block)
	{
		gofs += b->str_len;
		b = b->next;
	}
	gofs += ofs;
	return gofs;
}

int8 toupr(int8 ascii)
{
	// Shotcuts checks below are upper case
	if (ascii >= 'a' && ascii <= 'z')
		return ascii + 'A' - 'a';
	return ascii;
}

bool PStyleEdit::KeyDown(int8 ascii, uint16 function, uint32 modifierkeys)
{
	if (select_state)
		return false;

	bool handled = true;
	bool move_caret = function == TB_KEY_LEFT || function == TB_KEY_RIGHT || function == TB_KEY_UP || function == TB_KEY_DOWN ||
					function == TB_KEY_HOME || function == TB_KEY_END || function == TB_KEY_PAGE_UP || function == TB_KEY_PAGE_DOWN;

	if (!(modifierkeys & TB_SHIFT) && move_caret)
		selection.SelectNothing();

	int32 old_caret_ofs = caret.ofs;
	PBlock *old_caret_block = caret.block;
	PElement *old_caret_elm = caret.GetElement();

	if ((function == TB_KEY_UP || function == TB_KEY_DOWN) && (modifierkeys & TB_CTRL))
	{
		int32 line_height = GetStyle(0)->GetHeight();
		int32 new_y = scroll_y + (function == TB_KEY_UP ? -line_height : line_height);
		SetScrollPos(scroll_x, new_y);
	}
	else if (function == TB_KEY_LEFT)
		caret.Move(false, modifierkeys & TB_CTRL);
	else if (function == TB_KEY_RIGHT)
		caret.Move(true, modifierkeys & TB_CTRL);
	else if (function == TB_KEY_UP)
		handled = caret.Place(TBPoint(caret.wanted_x, old_caret_block->ypos + old_caret_elm->line_ypos - 1));
	else if (function == TB_KEY_DOWN)
		handled = caret.Place(TBPoint(caret.wanted_x, old_caret_block->ypos + old_caret_elm->line_ypos + old_caret_elm->line_height));
	else if (function == TB_KEY_PAGE_UP)
		caret.Place(TBPoint(caret.wanted_x, caret.y - layout_height));
	else if (function == TB_KEY_PAGE_DOWN)
		caret.Place(TBPoint(caret.wanted_x, caret.y + layout_height + old_caret_elm->line_height));
	else if (function == TB_KEY_HOME && modifierkeys & TB_CTRL)
		caret.Place(TBPoint(0, 0));
	else if (function == TB_KEY_END && modifierkeys & TB_CTRL)
		caret.Place(TBPoint(32000, last_block->ypos + last_block->height));
	else if (function == TB_KEY_HOME)
		caret.Place(TBPoint(0, caret.y));
	else if (function == TB_KEY_END)
		caret.Place(TBPoint(32000, caret.y));
	else if (toupr(ascii) == '8' && (modifierkeys & TB_CTRL))
	{
		packed.show_whitespace = !packed.show_whitespace;
		listener->Invalidate(TBRect(0, 0, layout_width, layout_height));
	}
	else if (toupr(ascii) == 'A' && (modifierkeys & TB_CTRL))
		selection.SelectAll();
	else if (!packed.read_only && (function == TB_KEY_DELETE || function == TB_KEY_BACKSPACE))
	{
		if (!selection.IsSelected())
		{
			caret.Move(function == TB_KEY_DELETE, modifierkeys & TB_CTRL);
			selection.SelectToCaret(old_caret_block, old_caret_ofs);
		}
		selection.RemoveContent();
	}
	else if ((toupr(ascii) == 'Z' && (modifierkeys & TB_CTRL)) ||
			(toupr(ascii) == 'Y' && (modifierkeys & TB_CTRL)))
	{
		if (!packed.read_only)
		{
			bool undo = toupr(ascii) == 'Z';
			if (modifierkeys & TB_SHIFT)
				undo = !undo;
			if (undo)
				undoredo.Undo(this);
			else
				undoredo.Redo(this);
		}
	}
	else if (!packed.read_only && (toupr(ascii) == 'X' && (modifierkeys & TB_CTRL)))
	{
		Cut();
	}
	else if ((toupr(ascii) == 'C' || function == TB_KEY_INSERT) && (modifierkeys & TB_CTRL))
	{
		Copy();
	}
	else if (!packed.read_only && ((toupr(ascii) == 'V' && (modifierkeys & TB_CTRL)) ||
								(function == TB_KEY_INSERT && (modifierkeys & TB_SHIFT))))
	{
		Paste();
	}
	else if (!packed.read_only && !(modifierkeys & TB_SHIFT) && (function == TB_KEY_TAB && packed.multiline_on))
		InsertText("\t", 1);
	else if (!packed.read_only && (function == TB_KEY_ENTER && packed.multiline_on) && !(modifierkeys & TB_CTRL))
	{
		if (caret.ofs == caret.block->str_len && !caret.block->last_element->IsBreak())
			InsertText("\r\n", 2);
		InsertText("\r\n", 2);
		caret.AvoidLineBreak();
		if (caret.block->next)
			caret.Place(caret.block->next, 0);
	}
	else if (!packed.read_only && (ascii && !(modifierkeys & TB_CTRL)) && function != TB_KEY_ENTER)
		InsertText(&ascii, 1);
	else
		handled = false;

	if ((modifierkeys & TB_SHIFT) && move_caret)
	{
		selection.SelectToCaret(old_caret_block, old_caret_ofs);
	}

	if(!(function == TB_KEY_UP || function == TB_KEY_DOWN || function == TB_KEY_PAGE_UP || function == TB_KEY_PAGE_DOWN))
		caret.UpdateWantedX();

	caret.ResetBlink();

	// Hooks
	if (!move_caret)
		listener->OnChange();
	/*if (function == TB_KEY_ENTER && !(modifierkeys & TB_CTRL) && on_enter_receiver)
	{
		on_enter_receiver->PostMessage(on_enter_message);
		handled = true;
	}*/
	if (handled)
		ScrollIfNeeded();

	return handled;
}

void PStyleEdit::Cut()
{
	if (packed.password_on)
		return;
	Copy();
	KeyDown(0, TB_KEY_DELETE, 0);
}

void PStyleEdit::Copy()
{
	if (packed.password_on)
		return;
	selection.CopyToClipboard();
}

void PStyleEdit::Paste()
{
	TBStr text;
	if (TBClipboard::HasText() && TBClipboard::GetText(text))
	{
		InsertText(text, text.Length());
		listener->OnChange();
	}
}

void PStyleEdit::Delete()
{
	if (selection.IsSelected())
	{
		selection.RemoveContent();
		listener->OnChange();
	}
}

void PStyleEdit::MouseDown(const TBPoint &point, int button, int clicks, uint32 modifierkeys)
{
	if (button == 1)
	{
		//if (modifierkeys & P_SHIFT) // Select to new caretpos
		//{
		//}
		//else // Start selection
		{
			mousedown_point = TBPoint(point.x + scroll_x, point.y + scroll_y);
			selection.SelectNothing();

			// clicks is 1 to infinite, and here we support only doubleclick, so make it either single or double.
			select_state = ((clicks - 1) % 2) + 1;

			MouseMove(point);

			if (caret.block)
				mousedown_element = caret.block->FindElement(mousedown_point.x, mousedown_point.y - caret.block->ypos);
		}
		caret.ResetBlink();
	}
}

void PStyleEdit::MouseUp(const TBPoint &point, int button, uint32 modifierkeys)
{
	select_state = 0;
	if (caret.block)
	{
		PElement *element = caret.block->FindElement(point.x + scroll_x, point.y + scroll_y - caret.block->ypos);
		if (element && element == mousedown_element)
			element->Click(button, modifierkeys);
	}
}

void PStyleEdit::MouseMove(const TBPoint &point)
{
	if (select_state)
	{
		TBPoint p(point.x + scroll_x, point.y + scroll_y);
		selection.Select(mousedown_point, p);

		if (select_state == 2)
		{
			bool has_initial_selection = selection.IsSelected();

			if (has_initial_selection)
				caret.Place(selection.start_block, selection.start_ofs);
			caret.Move(false, true);
			selection.start_block = caret.block;
			selection.start_ofs = caret.ofs;

			if (has_initial_selection)
				caret.Place(selection.stop_block, selection.stop_ofs);
			caret.Move(true, true);
			selection.stop_block = caret.block;
			selection.stop_ofs = caret.ofs;

			selection.CorrectOrder();
			caret.UpdateWantedX();
		}
	}
}

void PStyleEdit::Focus(bool focus)
{
	if (focus)
		listener->CaretBlinkStart();
	else
		listener->CaretBlinkStop();

	caret.on = focus;
	caret.Invalidate();
	selection.Invalidate();
}

void PStyleEdit::SetStyle(PStyle *style)
{
	style->IncRef();
	GetStyle(0)->DecRef();
	styles.Set(style, 0);
}

/*void PStyleEdit::SetFont(const PFont &font)
{
//	assert(NULL);
}*/

bool PStyleEdit::SetText(const char *text, PStyleEditImport* importer, bool place_caret_at_end)
{
	if (text == NULL || text[0] == 0)
	{
		Clear(true);
		return true;
	}

	if (importer)
	{
		Clear(true);
		importer->Parse(text, strlen(text), this);
		return true; //fix
	}

	//fix: nu kan len l�ggas till i settext!
	Clear(true);
	int len = strlen(text);
	first_block->InsertText(0, text, len, false);

	caret.Place(first_block, 0);
	caret.UpdateWantedX();
	ScrollIfNeeded(true, false);

	if (place_caret_at_end)
		caret.Place(last_block, last_block->str_len);
	return true;
}

bool PStyleEdit::Load(const char *filename, PStyleEditImport* importer)
{
	if (importer)
		return importer->Load(filename, this);
	PStyleEditImport plain_import;
	return plain_import.Load(filename, this);
}

bool PStyleEdit::GetText(TBStr &text)
{
	PSelection tmp_selection(this);
	tmp_selection.SelectAll();
	return tmp_selection.GetText(text);
}

bool PStyleEdit::IsEmpty()
{
	return first_block == last_block && first_block->str.IsEmpty();
}

void PStyleEdit::SetAlign(TB_TEXT_ALIGN align)
{
	if (this->align == align)
		return;
	this->align = align;
	caret.block->SetAlign(align);
}

void PStyleEdit::SetMultiline(bool multiline)
{
	if (packed.multiline_on == multiline)
		return;
	packed.multiline_on = multiline;
}

void PStyleEdit::SetReadOnly(bool readonly)
{
	if (packed.read_only == readonly)
		return;
	packed.read_only = readonly;
}

void PStyleEdit::SetPassword(bool password)
{
	if (packed.password_on == password)
		return;
	packed.password_on = password;
	Reformat();
}

void PStyleEdit::SetWrapping(bool wrapping)
{
	if (packed.wrapping == wrapping)
		return;
	packed.wrapping = wrapping;
	Reformat();
}

void PStyleEdit::SetEnabled(bool enabled)
{
	packed.enabled = enabled;
	SetReadOnly(!enabled);
}

// == PUndoRedoStack ==================================================

PUndoRedoStack::~PUndoRedoStack()
{
	Clear(true, true);
}

void PUndoRedoStack::Undo(PStyleEdit *styledit)
{
	if (!undos.GetNumItems())
		return;
	PUndoEvent *e = undos.Remove(undos.GetNumItems() - 1);
	redos.Add(e);
	Apply(styledit, e, true);
}

void PUndoRedoStack::Redo(PStyleEdit *styledit)
{
	if (!redos.GetNumItems())
		return;
	PUndoEvent *e = redos.Remove(redos.GetNumItems() - 1);
	undos.Add(e);
	Apply(styledit, e, false);
}

void PUndoRedoStack::Apply(PStyleEdit *styledit, PUndoEvent *e, bool reverse)
{
	// FIX: Call onchange
	applying = true;
	if (e->insert == reverse)
	{
		styledit->caret.SetGlobalOfs(e->gofs);
		PBlock *start_block = styledit->caret.block;
		int32 start_ofs = styledit->caret.ofs;
		styledit->caret.SetGlobalOfs(e->gofs + e->text.Length());
		styledit->selection.Select(start_block, start_ofs, styledit->caret.block, styledit->caret.ofs);
		styledit->selection.RemoveContent();
	}
	else
	{
		styledit->caret.SetGlobalOfs(e->gofs);
		styledit->InsertText(e->text);
	}
	applying = false;
}

void PUndoRedoStack::Clear(bool clear_undo, bool clear_redo)
{
	assert(!applying);
	if (clear_undo)
		undos.DeleteAll();
	if (clear_redo)
		redos.DeleteAll();
}

PUndoEvent *PUndoRedoStack::Commit(PStyleEdit *styledit, int32 gofs, int32 len, const char *text, bool insert)
{
	if (applying || styledit->packed.read_only)
		return NULL;
	Clear(false, true);

#ifdef _DEBUG
	for(int i = 0; i < len; i++)
		if (text[i] == EMBEDDED_CHARCODE)
		{
			assert(!"The undo redo stack does not yet handle styles and embeds! It may crash soon!");
		}
#endif

	// If we're inserting a single character, check if we want to append it to the previous event.
	if (insert && len == 1 && undos.GetNumItems())
	{
		PUndoEvent *e = undos[undos.GetNumItems() - 1];
		if (e->insert && e->gofs + e->text.Length() == gofs)
		{
			// Appending a space to other space(s) should append
			if ((text[0] == ' ' && !strpbrk(e->text.CStr(), "\n\r")) ||
				// But non spaces should not
				!strpbrk(e->text.CStr(), " \n\r"))
			{
				e->text.Append(text, len);
				return e;
			}
		}
	}

	// Create a new event
	PUndoEvent *e = new PUndoEvent();
	e->gofs = gofs;
	e->text.Set(text, len);
	e->insert = insert;
	undos.Add(e);
	return e;
}

}; // namespace tinkerbell