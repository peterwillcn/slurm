/*****************************************************************************\
 *  http_url.c - Slurm REST API http urls
 *****************************************************************************
 *  Copyright (C) 2019-2020 SchedMD LLC.
 *  Written by Nathan Rini <nate@schedmd.com>
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include "config.h"

#include <ctype.h>
#include <unistd.h>

#include "slurm/slurm.h"

#include "src/common/log.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/xassert.h"
#include "src/common/xmalloc.h"
#include "src/common/xstring.h"

#include "src/slurmrestd/http_url.h"

/*
 * chars that can pass without decoding.
 * rfc3986: unreserved characters.
 */
static bool _is_valid_url_char(char buffer)
{
	return (isxdigit(buffer) || buffer == '~' || buffer == '-' ||
		buffer == '.' || buffer == '_');
}

/*
 * decodes % sequence.
 * IN ptr pointing to % character
 * RET \0 on error or decoded character
 */
static unsigned char _decode_seq(const char *ptr)
{
	if (isxdigit(*(ptr + 1)) && isxdigit(*(ptr + 2))) {
		/* using unsigned char to avoid any rollover */
		unsigned char high = *(ptr + 1);
		unsigned char low = *(ptr + 2);
		unsigned char decoded = (slurm_char_to_hex(high) << 4) +
					slurm_char_to_hex(low);

		//TODO: find more invalid characters?
		if (decoded == '\0') {
			error("%s: invalid URL escape sequence for 0x00",
			      __func__);
			return '\0';
		} else if (decoded == 0xff) {
			error("%s: invalid URL escape sequence for 0xff",
			      __func__);
			return '\0';
		}

		debug5("%s: URL decoded: 0x%c%c -> %c",
		       __func__, high, low, decoded);

		return decoded;
	} else {
		debug("%s: invalid URL escape sequence: %s", __func__, ptr);
		return '\0';
	}
}

static int _add_path(data_t *d, char **buffer, bool convert_types)
{
	if (!xstrcasecmp(*buffer, ".")) {
		debug5("%s: ignoring path . entry", __func__);
	} else if (!xstrcasecmp(*buffer, "..")) {
		//TODO: pop last directory off sequence instead of fail
		debug5("%s: rejecting path .. entry", __func__);
		return SLURM_ERROR;
	} else {
		data_t *c = data_list_append(d);
		data_set_string(c, *buffer);

		if (convert_types)
			data_convert_type(c, DATA_TYPE_NONE);

		xfree(*buffer);
	}

	return SLURM_SUCCESS;
}

extern data_t *parse_url_path(const char *path, bool convert_types,
			      bool allow_templates)
{
	int rc = SLURM_SUCCESS;
	data_t *d = data_set_list(data_new());
	char *buffer = NULL;

	/* extract each word */
	for (const char *ptr = path; !rc && *ptr != '\0'; ++ptr) {
		if (_is_valid_url_char(*ptr)) {
			xstrcatchar(buffer, *ptr);
			continue;
		}

		switch (*ptr) {
		case '{': /* OASv3.0.3 section 4.7.8.2 template variable */
			if (!allow_templates) {
				debug("%s: unexpected OAS template character: %c",
				      __func__, *ptr);
				rc = SLURM_ERROR;
				break;
			} else {
				/* find end of template */
				char *end = xstrstr(ptr, "}");

				if (!end) {
					debug("%s: missing terminated OAS template character: }",
					      __func__);
					rc = SLURM_ERROR;
					break;
				}

				xstrncat(buffer, ptr, (end - ptr + 1));
				ptr = end;
				break;
			}
		case '%': /* rfc3986 */
		{
			const char c = _decode_seq(ptr);
			if (c != '\0') {
				/* shift past the hex value */
				ptr += 2;

				xstrcatchar(buffer, c);
			} else {
				debug("%s: invalid URL escape sequence: %s",
				      __func__, ptr);
				rc = SLURM_ERROR;
				break;
			}
			break;
		}
		case '/': /* rfc3986 */
			if (buffer != NULL)
				rc = _add_path(d, &buffer, convert_types);
			break;
		default:
			debug("%s: unexpected URL character: %c",
			      __func__, *ptr);
			rc = SLURM_ERROR;
		}
	}

	/* last part of path */
	if (!rc && buffer != NULL)
		rc = _add_path(d, &buffer, convert_types);

	if (rc) {
		FREE_NULL_DATA(d);
		return NULL;
	}

	return d;
}
