/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "MmsInputPlugin.hxx"
#include "input/ThreadInputStream.hxx"
#include "input/InputPlugin.hxx"
#include "util/StringUtil.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"

#include <libmms/mmsx.h>

static constexpr size_t MMS_BUFFER_SIZE = 256 * 1024;

class MmsInputStream final : public ThreadInputStream {
	mmsx_t *mms;

public:
	MmsInputStream(const char *uri, Mutex &mutex, Cond &cond)
		:ThreadInputStream(input_plugin_mms, uri, mutex, cond,
				   MMS_BUFFER_SIZE) {
	}

protected:
	virtual bool Open(gcc_unused Error &error) override;
	virtual size_t Read(void *ptr, size_t size, Error &error) override;

	virtual void Close() {
		mmsx_close(mms);
	}
};

static constexpr Domain mms_domain("mms");

bool
MmsInputStream::Open(Error &error)
{
	Unlock();

	mms = mmsx_connect(nullptr, nullptr, GetURI(), 128 * 1024);
	if (mms == nullptr) {
		Lock();
		error.Set(mms_domain, "mmsx_connect() failed");
		return false;
	}

	Lock();

	/* TODO: is this correct?  at least this selects the ffmpeg
	   decoder, which seems to work fine */
	SetMimeType("audio/x-ms-wma");
	return true;
}

static InputStream *
input_mms_open(const char *url,
	       Mutex &mutex, Cond &cond,
	       Error &error)
{
	if (!StringStartsWith(url, "mms://") &&
	    !StringStartsWith(url, "mmsh://") &&
	    !StringStartsWith(url, "mmst://") &&
	    !StringStartsWith(url, "mmsu://"))
		return nullptr;

	auto m = new MmsInputStream(url, mutex, cond);
	auto is = m->Start(error);
	if (is == nullptr)
		delete m;

	return is;
}

size_t
MmsInputStream::Read(void *ptr, size_t size, Error &error)
{
	int nbytes = mmsx_read(nullptr, mms, (char *)ptr, size);
	if (nbytes <= 0) {
		if (nbytes < 0)
			error.SetErrno("mmsx_read() failed");
		return 0;
	}

	return (size_t)nbytes;
}

const InputPlugin input_plugin_mms = {
	"mms",
	nullptr,
	nullptr,
	input_mms_open,
	ThreadInputStream::Close,
	ThreadInputStream::Check,
	nullptr,
	nullptr,
	ThreadInputStream::Available,
	ThreadInputStream::Read,
	ThreadInputStream::IsEOF,
	nullptr,
};
