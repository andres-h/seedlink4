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

DEFINE_SMARTPOINTER(Record);
class Record : public Core::BaseObject {
	public:
		Record() {}

		Record(const std::string &station,
		       const std::string &stream,
		       const std::string &format,
		       const Core::Time &starttime,
		       const Core::Time &endtime,
		       const std::string &payload)
		: _seq(SEQ_UNSET), _station(station), _stream(stream), _format(format)
		, _starttime(starttime), _endtime(endtime), _payload(payload) {}

		void setSequence(Sequence seq) { _seq = seq; }
		Sequence sequence() { return _seq; }
		std::string station() { return _station; }
		std::string stream() { return _stream; }
		std::string format() { return _format; }
		Core::Time startTime() { return _starttime; }
		Core::Time endTime() { return _endtime; }
		size_t headerLength() { return _station.length() + _stream.length() + _format.length() + 41; }
		size_t payloadLength() { return _payload.length(); }
		const std::string &payload() { return _payload; }

		void serializeHeader(Archive &ar);
		void serializePayload(Archive &ar);

	private:
		Sequence _seq;
		std::string _station;
		std::string _stream;
		std::string _format;
		Core::Time _starttime;
		Core::Time _endtime;
		std::string _payload;
};


}
}
}

#endif
