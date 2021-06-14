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

#ifndef SEISCOMP_APPS_SEEDLINK_MSEED24_H__
#define SEISCOMP_APPS_SEEDLINK_MSEED24_H__

#include "format.h"

namespace Seiscomp {
namespace Applications {
namespace Seedlink {

class Mseed24Format : public Format {
	public:
		Mseed24Format(): Format(FMT_MSEED24, "application/vnd.fdsn.mseed") {}

		ssize_t readRecord(const void *buf, size_t len, RecordPtr &rec);
};

}
}
}

#endif
