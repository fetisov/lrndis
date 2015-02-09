/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * version: 1.0 demo (7.02.2015)
 */

#include "cparser.h"

void parser_init(parser_t *p, const char *str, int len)
{
	p->buff = NULL;
	p->buff_size = 0;
	p->user_data = NULL;
	p->read_cb = NULL;
	p->total = 0;
	p->line = 0;
	p->col = 0;
	p->priv.curr = str;
	p->priv.stop = str + len;
}

void parser_init_s(parser_t *p, char *stream_buff, size_t size, parser_read_t cb)
{
	p->buff = stream_buff;
	p->buff_size = size;
	p->user_data = NULL;
	p->read_cb = cb;
	p->total = 0;
	p->line = 0;
	p->col = 0;
	p->priv.curr = NULL;
	p->priv.stop = NULL;
}

bool parser_provide_space(parser_t *p, int size)
{
	int allow;
	allow = p->priv.stop - p->priv.curr;
	if (allow >= size) return true;
	if (p->buff == NULL || p->buff_size < size) return false;
	memmove(p->buff, p->priv.stop - allow, allow);
	size = p->read_cb(p, p->buff + allow, p->buff_size - allow);
	p->priv.curr = p->buff;
	p->priv.stop = p->buff + allow + size;
	return true;
}

bool parser_is_next(parser_t *p, const char *str, int len)
{
	if (!parser_provide_space(p, len)) return false;
	return memcmp(p->priv.curr, str, len) == 0;
}

static bool query_data(parser_t *p)
{
	size_t size = p->read_cb(p, p->buff, p->buff_size);
	p->priv.curr = p->buff;
	p->priv.stop = p->buff + size;
	return size > 0;
}

#define INC_POS                 \
	p->total++;                 \
	p->col++;                   \
	if (*p->priv.curr == '\n')  \
	{                           \
		p->line++;              \
		p->col = 0;             \
	}                           \
	p->priv.curr++;

#define CHECK_STATE()                                       \
	eof = false;                                            \
	if (p->priv.curr == p->priv.stop)                       \
		eof = p->buff == NULL ? true : !query_data(p);

char parser_curr(parser_t *p)
{
	bool eof;
	CHECK_STATE();
	return eof ? 0 : *p->priv.curr;
}

bool parser_eof(parser_t *p)
{
	bool eof;
	CHECK_STATE();
	return eof;
}

int parser_read(parser_t *p, const chatset_t chatset, char *dest, int dest_size)
{
	bool eof;
	register int c;
	int res = 0;
	while (true)
	{
		CHECK_STATE();
		if (eof) break;
		c = (unsigned)*p->priv.curr;
		if (!chatset[c]) break;
		if (res < dest_size - 1)
			dest[res] = c;
		res++;
		INC_POS;
	}
	if (res >= dest_size)
		dest[dest_size - 1] = 0; else
		dest[res] = 0;
	return res;
}

int parser_read_before2(parser_t *p, char *dest, int dest_size, char c1, char c2)
{
	bool eof;
	register int cc;
	int res = 0;
	while (true)
	{
		CHECK_STATE();
		if (eof) break;
		cc = *p->priv.curr;
		if (cc == c1 || cc == c2) break;
		if (res < dest_size)
			dest[res] = cc;
		res++;
		INC_POS;
	}
	if (res >= dest_size)
		dest[dest_size - 1] = 0; else
		dest[res] = 0;
	return res;
}

int parser_read_digits(parser_t *p, char *dest, int dest_size, bool hex)
{
	bool eof;
	register int c;
	int res = 0;
	while (true)
	{
		bool done;
		CHECK_STATE();
		if (eof) break;
		c = (unsigned)*p->priv.curr;
		done = false;
		done |= c >= '0' && c <= '9';
		done |= hex && (c >= 'a' && c <= 'f');
		done |= hex && (c >= 'A' && c <= 'F');
		if (!done) break;
		if (res < dest_size - 1)
			dest[res] = c;
		res++;
		INC_POS;
	}
	if (res >= dest_size)
		dest[dest_size - 1] = 0; else
		dest[res] = 0;
	return res;
}

int parser_read_value(parser_t *p, char *dest, int dest_size, int fmt)
{
// state:     0    1   2    3   4   5   6     7
//         1234567
//         1234567          e       2
//         1234567          e   -   2
//             345 . 1234   e   +   4
//               0 . 1234   e   +   4
//               0 x                   ABCD
//               0 h                   ABCD
//               0 b                         0110

	bool eof;
	register int c;
	int res;
	int state;
	char last;

	res = 0;
	state = 0;

	while (true)
	{
		CHECK_STATE();
		if (eof) break;
		c = *p->priv.curr;

		if (state == 0 || state == 2 || state == 5)
		{
			if (c >= '0' && c <= '9')
			{
				if (res < dest_size) dest[res] = c;
				last = c;
				INC_POS;
				res++;
				continue;
			}
			if (state == 0 && c == '.' && (fmt & VAL_FMT_FLOAT))
			{
				if (res < dest_size) dest[res] = c;
				INC_POS;
				res++;
				state = 2;
				continue;
			}
			if ((state == 0 || state == 2) && (c == 'e' || c == 'E') && (fmt & VAL_FMT_FLOAT))
			{
				if (res < dest_size) dest[res] = c;
				INC_POS;
				res++;
				state = 4;
				continue;
			}
			if (state == 0 && last == '0' && res == 1)
			{
				if (((fmt & VAL_FMT_0H) && (c == 'H' || c == 'h')) || ((c == 'x' || c == 'X') && (fmt & VAL_FMT_0X)))
				{
					if (res < dest_size) dest[res] = c;
					INC_POS;
					res++;
					state = 6;
					continue;
				}
				if ((fmt & VAL_FMT_0B) && (c == 'b' || c == 'B'))
				{
					if (res < dest_size) dest[res] = c;
					INC_POS;
					res++;
					state = 7;
					continue;
				}
			}
			if ((fmt & VAL_FMT_SIGN) && res == 0 && (c == '+' || c == '-'))
			{
				if (res < dest_size) dest[res] = c;
				INC_POS;
				res++;
				continue;
			}
		}
		if (state == 4)
		{
			if (c == '+' || c == '-')
			{
				if (res < dest_size) dest[res] = c;
				INC_POS;
				res++;
			}
			state = 5;
			continue;
		}
		if (state == 6)
		{
			if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
			{
				if (res < dest_size) dest[res] = c;
				INC_POS;
				res++;
				continue;
			}
		}
		if (state == 7)
		{
			if (c == '0' || c == '1')
			{
				if (res < dest_size) dest[res] = c;
				INC_POS;
				res++;
				continue;
			}
		}
		break;
	}
	if (res >= dest_size)
		dest[dest_size - 1] = 0; else
		dest[res] = 0;
	return res;
}

#define is_ident_char(c)      \
	(c == '_' ||              \
	(c >= '0' && c <= '9') || \
	(c >= 'a' && c <= 'z') || \
	(c >= 'a' && c <= 'z'))

int parser_read_string(parser_t *p, char *dest, int dest_size, int fmt)
{
// state:     0     1    2      3         4     5
//               string
//            '  string                         '
//            "  string                         "
//            '  str     \                      '
//            '  str     \    rntbf\/"          '
//            '  str     \    u                 '
//            '  str     \    u                 '
//            '  str     \    u         12AB    '
	bool eof;
	register int c;
	int res;
	int state;
	char first;

	res = 0;
	state = 0;
	first = 0;

	while (true)
	{
		CHECK_STATE();
		if (eof) break;
		c = *p->priv.curr;

		if (state == 0 && (((fmt & STR_FMT_QUOTE) && c == '"') || ((fmt & STR_FMT_APOST) && c == '\'')))
		{
			first = c;
			state = 1;
			INC_POS;
			continue;
		}

		if (state == 0 && (fmt & STR_FMT_NAKED))
		{
			state = 1;
			continue;
		}

		if (state == 1 && c == first)
		{
			INC_POS;
			break;
		}

		if (state == 1 && first == 0 && is_ident_char(c)) // reading naked
		{
			if (res < dest_size) dest[res] = c;
			INC_POS;
			res++;
			continue;
		}

		if (state == 1 && (fmt & STR_FMT_ESC) && c == '\\')
		{
			state = 3;
			INC_POS;
			continue;
		}

		if (state == 1 && first != 0)
		{
			if (res < dest_size) dest[res] = c;
			INC_POS;
			res++;
			continue;
		}

		if (state == 3 && (fmt & STR_FMT_ESCU) && c == 'u')
		{
			state = 4;
			INC_POS;
			continue;
		}

		if (state == 3)
		{
			char e;
			switch (c)
			{
			case 'r': e = '\r'; break;
			case 'n': e = '\n'; break;
			case 't': e = '\t'; break;
			case 'b': e = '\b'; break;
			case 'f': e = '\f'; break;
			case '\\': e = '\\'; break;
			case '/': e = '/'; break;
			case '"': e = '"'; break;
			default:
				if (res < dest_size) dest[res] = '\\';
				res++;
				e = c;
				break;
			}
			state = 1;
			if (res < dest_size) dest[res] = e;
			INC_POS;
			res++;
			continue;
		}

		if (state == 4)
		{
		}

		break;
	}
	if (res >= dest_size)
		dest[dest_size - 1] = 0; else
		dest[res] = 0;
	return res;
}

int parser_skip(parser_t *p, const chatset_t chatset)
{
	bool eof;
	register int c;
	int res = 0;
	while (true)
	{
		CHECK_STATE();
		if (eof) break;
		c = (unsigned)*p->priv.curr;
		if (!chatset[(uint8_t)c]) break;
		res++;
		INC_POS;
	}
	return res;
}

int parser_skip_n(parser_t *p, int n)
{
	bool eof;
	int i;
	for (i = 0; i < n; i++)
	{
		CHECK_STATE();
		if (eof) return i;
		INC_POS;
	}
	return n;
}

bool parser_skip_char(parser_t *p, char c)
{
	bool eof;
	CHECK_STATE();
	if (*p->priv.curr != c || eof) return false;
	INC_POS;
	return true;
}

bool parser_skip_ws(parser_t *p)
{
	bool eof;
	bool res = false;
	while (true)
	{
		CHECK_STATE();
		if (eof) return res;
		if (*p->priv.curr > 0x20) return res;
		res = true;
		INC_POS;
	}
}

bool parser_skip_ws_in_line(parser_t *p)
{
	bool eof;
	bool res = false;
	while (true)
	{
		CHECK_STATE();
		if (eof) return res;
		if (*p->priv.curr > 0x20 || *p->priv.curr == '\r' || *p->priv.curr == '\n')
			return res;
		res = true;
		INC_POS;
	}
}

bool parser_skip_line(parser_t *p)
{
	bool eof;
	while (true)
	{
		CHECK_STATE();
		if (eof) return false;
		if (*p->priv.curr == '\n')
		{
			INC_POS;
			return true;
		}
		INC_POS;
	}
}

bool parser_skip_before(parser_t *p, char c)
{
	bool eof;
	while (true)
	{
		CHECK_STATE();
		if (eof) return false;
		if (*p->priv.curr == c) return true;
		INC_POS;
	}
}

