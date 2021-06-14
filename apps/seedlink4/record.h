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


typedef uint64_t Sequence;
const Sequence SEQ_UNSET = Sequence(-1);


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
		       const Core::Time &endtime,
		       FormatCode format,
		       const std::string &payload)
		: _seq(SEQ_UNSET), _net(net), _sta(sta), _loc(loc), _cha(cha)
		, _type(type) , _starttime(starttime), _endtime(endtime)
		, _format(format), _payload(payload) {}

		void setSequence(Sequence seq) { _seq = seq; }
		Sequence sequence() { return _seq; }
		std::string network() { return _net; }
		std::string station() { return _sta; }
		std::string location() { return _loc; }
		std::string channel() { return _cha; }
		std::string type() { return _type; }
		Core::Time startTime() { return _starttime; }
		Core::Time endTime() { return _endtime; }
		FormatCode format() { return _format; }
		std::string stream() { return _loc + "." + _cha + "." + _type + "." + std::string(1, _format); }
		const std::string &payload() { return _payload; }
		size_t payloadLength() { return _payload.length(); }

		void serializeHeader(Archive &ar);
		void serializePayload(Archive &ar);

	private:
		Sequence _seq;
		std::string _net;
		std::string _sta;
		std::string _loc;
		std::string _cha;
		std::string _type;
		Core::Time _starttime;
		Core::Time _endtime;
		int _format;
		std::string _payload;
};


}
}
}

#endif
