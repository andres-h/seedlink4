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

#include "record.h"


using namespace std;
using namespace Seiscomp;

namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Record::serializeHeader(Core::Archive &ar) {
	Core::Time ts = Core::Time(_timespan);  // (temporary?) hack to serialize Core::TimeSpan
	bool valid = true;  // marks the beginning of a valid record
	ar & NAMED_OBJECT_HINT("valid", valid, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("networkCode", _net, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("stationCode", _sta, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("locationCode", _loc, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("channelCode", _cha, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("typeCode", _type, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("startTime", _starttime, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("timeSpan", ts, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("format", _format, Core::Archive::STATIC_TYPE);
	_timespan = Core::TimeSpan(ts);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Record::serializePayload(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("payload", _payload, Core::Archive::STATIC_TYPE);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

