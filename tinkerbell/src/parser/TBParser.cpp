// ================================================================================
// == This file is a part of Tinkerbell UI Toolkit. (C) 2011-2012, Emil Seger�s ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "parser/TBParser.h"
#include "tb_tempbuffer.h"
#include <assert.h>

/* TEST
FIX: Put theese in a _DEBUG section selftest

testA: text1: " foo bar : , ", text2: "      \"        "  , text3:   ""
testB: text1: "\\", text2: "\"", text3: "\"", text3: "\\\\\\\\", text3: "\\\\\""
*/

namespace tinkerbell {

Parser::STATUS Parser::Read(ParserStream *stream, ParserTarget *target)
{
	TBTempBuffer line, work;
	if (!line.Reserve(1024) || !work.Reserve(1024))
		return STATUS_NO_MEMORY;

	current_indent = 0;
	current_line_nr = 1;

	while (int read_len = stream->GetMoreData((char *)work.GetData(), work.GetCapacity()))
	{
		char *buf = work.GetData();
		int line_pos = 0;
		while (true)
		{
			// Find line end
			int line_start = line_pos;
			while (line_pos < read_len && buf[line_pos] != '\n')
				line_pos++;

			if (line_pos < read_len)
			{
				// We have a line
				// Skip preceding \r (if we have one)
				int line_len = line_pos - line_start;
				if (!line.Append(buf + line_start, line_len))
					return STATUS_NO_MEMORY;

				// Strip away trailing '\r' if the line has it
				char *buf = line.GetData();
				int buf_len = line.GetAppendPos();
				if (buf_len > 0 && buf[buf_len - 1] == '\r')
					buf[buf_len - 1] = 0;

				// Terminate the line string
				if (!line.Append("", 1))
					return STATUS_NO_MEMORY;

				// Handle line
				OnLine(line.GetData(), target);
				current_line_nr++;

				line.ResetAppendPos();
				line_pos++; // Skip this \n
				// Find next line
				continue;
			}
			// No more lines here so push the rest and break for more data
			if (!line.Append(buf + line_start, read_len - line_start))
				return STATUS_NO_MEMORY;
			break;
		}
	}
	if (line.GetAppendPos())
	{
		if (!line.Append("", 1))
			return STATUS_NO_MEMORY;
		OnLine(line.GetData(), target);
		current_line_nr++;
	}
	return STATUS_OK;
}

void UnescapeString(char *str)
{
	char *dst = str, *src = str;
	while(*src)
	{
		if (*src == '\\')
		{
			if (src[1] == 'n')
				*dst = '\n';
			else if (src[1] == 't')
				*dst = '\t';
			else if (src[1] == '\"')
				*dst = '\"';
			src += 2;
			dst++;
			continue;
		}
		*dst = *src;
		dst++;
		src++;
	}
	*dst = 0;
}

void Parser::OnLine(char *line, ParserTarget *target)
{
	if (*line == '#')
	{
		target->OnComment(line + 1);
		return;
	}

	// Check indent
	int indent = 0;
	while (line[indent] == '\t' && line[indent] != 0)
		indent++;
	line += indent;

	if (indent - current_indent > 1)
	{
		target->OnError(current_line_nr, "Indentation error. (Line skipped)");
		return;
	}

	if (indent > current_indent)
	{
		// FIX: om den �r mer �n 1 h�gre �r det indentation error!
		assert(indent - current_indent == 1);
		target->Enter();
		current_indent++;
	}
	else if (indent < current_indent)
	{
		while (indent < current_indent)
		{
			target->Leave();
			current_indent--;
		}
	}

	if (*line == 0)
		return;
	else
	{
		char *token = line;
		// Read line while consuming it and copy over to token buf
		while (*line != ' ' && *line != 0)
			line++;
		int token_len = line - token;
		// Consume any white space after the token
		while (*line == ' ')
			line++;

		bool is_compact_line = token_len && token[token_len - 1] == ':';

		TBValue value;
		if (is_compact_line)
		{
			token_len--;
			token[token_len] = 0;
		}
		else if (token[token_len])
		{
			token[token_len] = 0;
			UnescapeString(line);
			value.SetFromStringAuto(line, TBValue::SET_AS_STATIC);
		}
		target->OnToken(token, value);

		if (is_compact_line)
			OnCompactLine(line, target);
	}
}

/** Check if buf is pointing at a end quote. It may need to iterate
	buf backwards toward buf_start to check if any preceding backslashes
	make it a escaped quote (which should not be the end quote) */
bool IsEndQuote(const char *buf_start, const char *buf)
{
	if (*buf != '\"')
		return false;
	int num_backslashes = 0;
	while (buf_start < buf && *(buf-- - 1) == '\\')
		num_backslashes++;
	return !(num_backslashes & 1);
}

void Parser::OnCompactLine(char *line, ParserTarget *target)
{
	target->Enter();
	while (*line)
	{
		// consume any whitespace
		while (*line == ' ')
			line++;

		// Find token
		char *token = line;
		while (*line != ':' && *line != 0)
			line++;
		if (!*line)
			break; // Syntax error, expected token
		*line++ = 0;

		// consume any whitespace
		while (*line == ' ')
			line++;

		TBValue v;

		// Find value (As quoted string, or as auto)
		char *value = line;
		if (*line == '\"')
		{
			// Consume starting quote
			line++;
			value++;
			// Find ending quote or end
			while (!IsEndQuote(value, line) && *line != 0)
				line++;
			// Terminate away the quote
			if (*line == '\"')
				*line++ = 0;

			// consume any whitespace
			while (*line == ' ')
				line++;
			// consume any comma
			if (*line == ',')
				line++;

			UnescapeString(value);
			v.SetString(value, TBValue::SET_AS_STATIC);
		}
		else
		{
			// Find next comma or end
			while (*line != ',' && *line != 0)
				line++;
			// Terminate away the comma
			if (*line == ',')
				*line++ = 0;

			UnescapeString(value);
			v.SetFromStringAuto(value, TBValue::SET_AS_STATIC);
		}

		// Ready
		target->OnToken(token, v);
	}

	target->Leave();
}

}; // namespace tinkerbell