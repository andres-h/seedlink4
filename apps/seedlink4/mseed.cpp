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

#define SEISCOMP_COMPONENT SEEDLINK
#include <seiscomp/logging/log.h>

#include <libmseed.h>

#include "mseed.h"

using namespace std;
using namespace Seiscomp;


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MseedFormat::MseedFormat(FormatCode code, const std::string &mimetype, uint8_t version)
: Format(code, mimetype), _version(version){
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ssize_t MseedFormat::readRecord(const void *buf, size_t len, RecordPtr &rec) {
	MS3Record *msr = NULL;
	int err;

	// FIXME: should accept const buffer
	if ( (err = msr3_parse((char *)buf, len, &msr, 0, 0)) < 0 ) {
		SEISCOMP_ERROR("MiniSEED parse error: %s", ms_errorstr(err));
		return -1;
	}

	if ( msr->formatversion != _version ) {
		SEISCOMP_ERROR("MiniSEED version (%d) does not match format code (%c)",
			       msr->formatversion,
			       formatCode());
		return -1;
	}

	if ( (size_t)msr->reclen > len ) {
		SEISCOMP_ERROR("invalid record length (%lu > %lu)",
			       (size_t)msr->reclen, len);
		return -1;
	}

	if ( (size_t)msr->reclen < len )
		SEISCOMP_WARNING("record is shorter than expected (%lu < %lu)",
				 (size_t)msr->reclen, len);

	char net[LM_SIDLEN];
	char sta[LM_SIDLEN];
	char loc[LM_SIDLEN];
	char cha[LM_SIDLEN];

	if ( ms_sid2nslc(msr->sid, net, sta, loc, cha) < 0 ) {
		SEISCOMP_ERROR("unsupported source identifier: %.*s", LM_SIDLEN, msr->sid);
		return -1;
	}

	char type = 'D';

	if ( mseh_exists(msr, "FDSN.Event.Detection" ) ) {
		type = 'E';
	}
	else if ( mseh_exists(msr, "FDSN.Calibration.Sequence" ) ) {
		type = 'C';
	}
	else if ( mseh_exists(msr, "FDSN.Time.Exception" ) ) {
		type = 'T';
	}
	else if ( msr->samprate == 0.0 ) {
		if ( msr->samplecnt != 0 )
			type = 'L';  // log
		else
			type = 'O';  // opaque
	}
	
	long sec = msr->starttime / NSTMODULUS;
	long usec = (msr->starttime % NSTMODULUS) / 1000;

	Core::Time starttime(sec, usec);
	Core::Time endtime = starttime;

	if ( msr->samprate > 0.0 )
		endtime += Core::TimeSpan(msr->samplecnt / msr->samprate);

	string payload((const char *)buf, msr->reclen);

	msr3_free(&msr);

	rec = new Record(net, sta, loc, cha, type, formatCode(), starttime, endtime, payload);
	return payload.size();
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

namespace {
MseedFormat mseed24(FMT_MSEED24, "application/vnd.fdsn.mseed", 2);
MseedFormat mseed30(FMT_MSEED30, "application/vnd.fdsn.mseed3", 3);
}

}
}
}

