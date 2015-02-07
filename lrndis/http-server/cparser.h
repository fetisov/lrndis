/*
 * Copyright (C) 2015 by Sergey Fetisov <fsenok@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * version: 1.0 demo (7.02.2015)
 */

#ifndef CPARSER_H
#define CPARSER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct parser parser_t;

typedef size_t (*parser_read_t)(parser_t *parser, char *buffer, size_t size);
typedef char chatset_t[256];

typedef struct parser
{
    char          *buff;
    size_t         buff_size;
    void          *user_data;
    parser_read_t  read_cb;
    size_t         total;
    size_t         line;
    size_t         col;
    struct
    {
        const char *curr;
        const char *stop;
    } priv;
} parser_t;

#define VAL_FMT_0X    1    // supports 0x12AB
#define VAL_FMT_0H    2    // supports 0h12AB
#define VAL_FMT_0B    4    // supports 0b01101001
#define VAL_FMT_FLOAT 8    // supports 3.1415
#define VAL_FMT_EXP   16   // supports 0.31415e+1
#define VAL_FMT_SIGN  32   // supports +-3.1415
#define VAL_FMT_ALL   0xFF

#define STR_FMT_QUOTE 1     // supports "string"
#define STR_FMT_APOST 2     // supports 'string'
#define STR_FMT_NAKED 4     // supports  string
#define STR_FMT_ESC   8     // supports \ escaping (bfnrt'"/\)
#define STR_FMT_ESCU  16    // supports unicode escaping \u12AB (like json)
#define STR_FMT_ESCW  32    // supports web escaping %2A
#define STR_FMT_ALL   0xFF

void parser_init(parser_t *p, const char *str, int len);
void parser_init_s(parser_t *p, char *stream_buff, size_t size, parser_read_t cb);
bool parser_is_next(parser_t *p, const char *str, int len);
char parser_curr(parser_t *p);
bool parser_eof(parser_t *p);

int  parser_read(parser_t *p, const chatset_t chatset, char *dest, int dest_size);
int  parser_read_before2(parser_t *p, char *dest, int dest_size, char c1, char c2);
int  parser_read_digits(parser_t *p, char *dest, int dest_size, bool hex);
int  parser_read_value(parser_t *p, char *dest, int dest_size, int fmt);
int  parser_read_string(parser_t *p, char *dest, int dest_size, int fmt);
#define parser_read_before(p, dest, dest_size, c) \
	parser_read_before2(p, dest, dest_size, c, c);
#define parser_read_before_line_end(p, dest, dest_size) \
	parser_read_before2(p, dest, dest_size, '\r', '\n');

int  parser_skip(parser_t *p, const chatset_t chatset);
int  parser_skip_n(parser_t *p, int n);
bool parser_skip_char(parser_t *p, char c);
bool parser_skip_ws(parser_t *p);
bool parser_skip_ws_in_line(parser_t *p);
bool parser_skip_line(parser_t *p);
bool parser_skip_before(parser_t *p, char c);

#ifdef __cplusplus
}
#endif

#endif
