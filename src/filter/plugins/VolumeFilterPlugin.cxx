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
#include "VolumeFilterPlugin.hxx"
#include "filter/FilterPlugin.hxx"
#include "filter/FilterInternal.hxx"
#include "filter/FilterRegistry.hxx"
#include "pcm/Volume.hxx"
#include "AudioFormat.hxx"
#include "util/ConstBuffer.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"

#include <assert.h>
#include <string.h>

class VolumeFilter final : public Filter {
	PcmVolume pv;

public:
	unsigned GetVolume() const {
		return pv.GetVolume();
	}

	void SetVolume(unsigned _volume) {
		pv.SetVolume(_volume);
	}

	virtual AudioFormat Open(AudioFormat &af, Error &error) override;
	virtual void Close();
	virtual const void *FilterPCM(const void *src, size_t src_size,
				      size_t *dest_size_r, Error &error);
};

static constexpr Domain volume_domain("pcm_volume");

static Filter *
volume_filter_init(gcc_unused const config_param &param,
		   gcc_unused Error &error)
{
	return new VolumeFilter();
}

AudioFormat
VolumeFilter::Open(AudioFormat &audio_format, Error &error)
{
	if (!pv.Open(audio_format.format, error))
		return AudioFormat::Undefined();

	return audio_format;
}

void
VolumeFilter::Close()
{
	pv.Close();
}

const void *
VolumeFilter::FilterPCM(const void *src, size_t src_size,
			size_t *dest_size_r, gcc_unused Error &error)
{
	const auto dest = pv.Apply({src, src_size});
	*dest_size_r = dest.size;
	return dest.data;
}

const struct filter_plugin volume_filter_plugin = {
	"volume",
	volume_filter_init,
};

unsigned
volume_filter_get(const Filter *_filter)
{
	const VolumeFilter *filter =
		(const VolumeFilter *)_filter;

	return filter->GetVolume();
}

void
volume_filter_set(Filter *_filter, unsigned volume)
{
	VolumeFilter *filter = (VolumeFilter *)_filter;

	filter->SetVolume(volume);
}

