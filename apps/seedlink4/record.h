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

#ifndef SEISCOMP_APPS_SEEDLINK_RECORD_H__
#define SEISCOMP_APPS_SEEDLINK_RECORD_H__

#include <string>

#include <seiscomp/core/baseobject.h>
#include <seiscomp/core/io.h>


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


typedef uint8_t FormatCode;
const FormatCode FMT_MSEED24 = '2';
const FormatCode FMT_MSEED30 = '3';


DEFINE_SMARTPOINTER(Record);
class Record : public Core::BaseObject {
	public:
		Record(): _format(0) {}

		Record(const std::string &net,
		       const std::string &sta,
		       const std::string &loc,
		       const std::string &cha,
		       const std::string &type,
		       const Core::Time &starttime,
		       const Core::TimeSpan &timespan,
		       FormatCode format,
		       const std::string &payload)
		: _net(net), _sta(sta), _loc(loc), _cha(cha), _type(type)
		, _starttime(starttime), _timespan(timespan)
		, _format(format), _payload(payload) {}

		std::string networkCode() { return _net; }
		std::string stationCode() { return _sta; }
		std::string locationCode() { return _loc; }
		std::string channelCode() { return _cha; }
		std::string typeCode() { return _type; }
		Core::Time startTime() { return _starttime; }
		Core::TimeSpan timeSpan() { return _timespan; }
		FormatCode format() { return _format; }
		const std::string &payload() { return _payload; }

		void serializeHeader(Core::Archive &ar);
		void serializePayload(Core::Archive &ar);

	private:
		std::string _net;
		std::string _sta;
		std::string _loc;
		std::string _cha;
		std::string _type;
		Core::Time _starttime;
		Core::TimeSpan _timespan;
		int _format;
		std::string _payload;
};


}
}
}

#endif
