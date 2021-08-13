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
MseedFormat::MseedFormat(const string &code, const string &mimetype, const string &description, uint8_t version)
: Format(code, mimetype, description), _version(version){
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
		SEISCOMP_ERROR("MiniSEED version (%d) does not match format code (%s)",
			       msr->formatversion,
			       formatCode().c_str());
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

	string station, stream;

	if ( !strncmp(msr->sid, "FDSN:", 5) ) {
		char* psta = strchr(&msr->sid[5], '_');
		if ( psta == NULL ) {
			SEISCOMP_ERROR("invalid FDSN source identifier: %.*s", LM_SIDLEN, msr->sid);
			return -1;
		}

		char* pstream = strchr(psta + 1, '_');
		if ( pstream == NULL ) {
			SEISCOMP_ERROR("invalid FDSN source identifier: %.*s", LM_SIDLEN, msr->sid);
			return -1;
		}

		station = string(&msr->sid[5], pstream);
		stream = string(pstream + 1);
	}
	else if ( !strncmp(msr->sid, "XFDSN:", 6) ) {
		char* psta = strchr(&msr->sid[6], '_');
		if ( psta == NULL ) {
			SEISCOMP_ERROR("invalid XFDSN source identifier: %.*s", LM_SIDLEN, msr->sid);
			return -1;
		}

		char* pstream = strchr(psta + 1, '_');
		if ( pstream == NULL ) {
			SEISCOMP_ERROR("invalid XFDSN source identifier: %.*s", LM_SIDLEN, msr->sid);
			return -1;
		}

		station = string(&msr->sid[6], pstream);
		stream = string(pstream + 1);
	}
	else {
		SEISCOMP_ERROR("unsupported source identifier: %.*s", LM_SIDLEN, msr->sid);
		return -1;
	}

	long sec = msr->starttime / NSTMODULUS;
	long usec = (msr->starttime % NSTMODULUS) / 1000;

	Core::Time starttime(sec, usec);
	Core::Time endtime = starttime;

	if ( msr->samprate > 0.0 )
		endtime += Core::TimeSpan(msr->samplecnt / msr->samprate);

	string payload((const char *)buf, msr->reclen);

	msr3_free(&msr);

	rec = new Record(station, stream, formatCode(), starttime, endtime, payload);
	return payload.size();
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

namespace {
MseedFormat mseed2d("2D", "application/vnd.fdsn.mseed", "data/generic", 2);
MseedFormat mseed2e("2E", "application/vnd.fdsn.mseed", "event", 2);
MseedFormat mseed2c("2C", "application/vnd.fdsn.mseed", "calibration", 2);
MseedFormat mseed2t("2T", "application/vnd.fdsn.mseed", "timing", 2);
MseedFormat mseed2o("2O", "application/vnd.fdsn.mseed", "opaque", 2);
MseedFormat mseed2l("2L", "application/vnd.fdsn.mseed", "log", 2);
MseedFormat mseed3("3D", "application/vnd.fdsn.mseed3", "data/generic", 3);
}

}
}
}

