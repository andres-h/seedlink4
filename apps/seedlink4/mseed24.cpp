/***************************************************************************
 * (C) 2020 Helmholtz-Zentrum Potsdam - Deutsches GeoForschungsZentrum GFZ *
 * All rights reserved.                                                    *
 *                                                                         *
 * This file may be used under the terms of the GNU Affero                 *
 * Public License version 3.0 as published by the Free Software Foundation *
 * and appearing in the file LICENSE included in the packaging of this     *
 * file. Please review the following information to ensure the GNU Affero  *
 * Public License version 3.0 requirements will be met:                    *
 * https://www.gnu.org/licenses/agpl-3.0.html.                             *
 ***************************************************************************/

#include <string>
#include <arpa/inet.h>

#include "mseed24.h"

using namespace std;
using namespace Seiscomp;


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// TODO: use libmseed


struct ms_btime
  {
    u_int16_t year;
    u_int16_t yday;
    u_int8_t hour;
    u_int8_t min;
    u_int8_t sec;
    u_int8_t reserved;
    u_int16_t sfrac;
  } __attribute__ ((packed));

struct ms_datahead
  {
/*  Fixed Section of Data Header (48 bytes) */

    char seq[8];
    char station_id[5];
    char location_id[2];
    char channel_id[3];
    char network_id[2];
    struct ms_btime record_start_time;
    u_int16_t number_of_samples;
    int16_t sample_rate_factor;
    int16_t sample_rate_multiplier;
    u_int8_t activity_flags;
    u_int8_t io_and_clock_flags;
    u_int8_t data_quality_flags;
    u_int8_t number_of_blockettes;
    int32_t time_correction;
    u_int16_t beginning_of_data;
    u_int16_t first_blockette;

/*  Data Only SEED Blockette (8 bytes) */

    u_int16_t blockette_type;
    u_int16_t next_blockette;
    u_int8_t encoding_format;
    u_int8_t word_order;
    u_int8_t data_record_length;
    u_int8_t reserved1;

    u_int8_t reserved2[8];

  } __attribute__ ((packed));


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ssize_t Mseed24Format::readRecord(const void *buf, size_t len, RecordPtr &rec) {
	ms_datahead* head = (ms_datahead *)buf;

	if ( len < sizeof(ms_datahead) || len < size_t(1 << head->data_record_length) )
		return 0;

	string net = string(head->network_id, 2);
	string sta = string(head->station_id, 5);
	string loc = string(head->location_id, 2);
	string cha = string(head->channel_id, 3);

	net.erase(net.find_last_not_of(" ") + 1);
	sta.erase(sta.find_last_not_of(" ") + 1);
	loc.erase(loc.find_last_not_of(" ") + 1);
	cha.erase(cha.find_last_not_of(" ") + 1);

	int year = ntohs(head->record_start_time.year);
	int yday = ntohs(head->record_start_time.yday);
	int month = 0;
	int day = 0;
	int hour = head->record_start_time.hour;
	int min = head->record_start_time.min;
	int sec = head->record_start_time.sec;
	int usec = ntohs(head->record_start_time.sfrac) * 100;

	Core::Time starttime = Core::Time::FromYearDay(year, yday);
	starttime.get(&year, &month, &day);
	starttime.set(year, month, day, hour, min, sec, usec);

	Core::Time endtime;
	char type;

	if ( head->sample_rate_factor && head->sample_rate_multiplier ) {
		double p1 = ((int16_t)ntohs(head->sample_rate_factor) > 0)?
			(1.0 / (int16_t)ntohs(head->sample_rate_factor)):
			-(int16_t)ntohs(head->sample_rate_factor);

		double p2 = ((int16_t)ntohs(head->sample_rate_multiplier) > 0)?
			(1.0 / (int16_t)ntohs(head->sample_rate_multiplier)):
			-(int16_t)ntohs(head->sample_rate_multiplier);

		endtime = starttime + Core::TimeSpan(ntohs(head->number_of_samples) * p1 * p2);
		type = 'D';
	}
	else {
		// This includes E, T, C and O types
		endtime = starttime;
		type = 'L';
	}

	string payload((char *)buf, (1 << head->data_record_length));
	rec = new Record(net, sta, loc, cha, type, formatCode(), starttime, endtime, payload);
	return payload.size();
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

namespace {
Mseed24Format mseed24format;
}

}
}
}

