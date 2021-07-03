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

#include "info.h"

using namespace std;
using namespace Seiscomp;

namespace Seiscomp {
namespace Applications {
namespace Seedlink {


#define ARCHIVE_FLAGS Core::Archive::STATIC_TYPE|Core::Archive::XML_MANDATORY


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
StreamInfo::StreamInfo(double slproto, StreamPtr stream)
: _slproto(slproto), _stream(stream) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StreamInfo::serialize(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("location", _stream->_loc, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("seedname", _stream->_cha, ARCHIVE_FLAGS);

	if ( _slproto < 4.0 ) {
		string type;

		switch ( _stream->_format ) {
			case FMT_MSEED24:
				type = "D";
				break;

			case FMT_MSEED24_EVENT:
				type = "E";
				break;

			case FMT_MSEED24_CALIBRATION:
				type = "C";
				break;

			case FMT_MSEED24_TIMING:
				type = "T";
				break;

			case FMT_MSEED24_OPAQUE:
				type = "O";
				break;

			case FMT_MSEED24_LOG:
				type = "L";
				break;

			default:
				throw logic_error("unexpected format");
		}

		ar & NAMED_OBJECT_HINT("type", type, ARCHIVE_FLAGS);
		string starttime = _stream->_starttime.toString("%F %T.%4f");
		string endtime = _stream->_endtime.toString("%F %T.%4f");
		ar & NAMED_OBJECT_HINT("begin_time", starttime, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("end_time", endtime, ARCHIVE_FLAGS);
	}
	else {
		string format(1, _stream->_format);
		ar & NAMED_OBJECT_HINT("format", format, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("begin_time", _stream->_starttime, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("end_time", _stream->_endtime, ARCHIVE_FLAGS);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RingInfo::RingInfo(double slproto, RingPtr ring, const string &description, enum InfoLevel level)
: _slproto(slproto), _ring(ring), _description(description), _level(level) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RingInfo::serialize(Core::Archive &ar) {
	size_t sep = _ring->_name.find_first_of('.');
	if ( sep == string::npos )  // TODO: assert
		return;

	string net = _ring->_name.substr(0, sep);
	string sta = _ring->_name.substr(sep + 1);
	string enabled = "enabled";

	ar & NAMED_OBJECT_HINT("name", sta, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("network", net, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("description", _description, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("stream_check", enabled, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("ordered", _ring->_ordered, ARCHIVE_FLAGS);

	if ( _slproto < 4.0 ) {
		string baseseq;
		baseseq.resize(7);
		snprintf(&baseseq[0], 7, "%06llX", (long long unsigned int)_ring->_baseseq);
		ar & NAMED_OBJECT_HINT("begin_seq", baseseq, ARCHIVE_FLAGS);

		string endseq;
		endseq.resize(7);
		snprintf(&endseq[0], 7, "%06llX", (long long unsigned int)_ring->_endseq);
		ar & NAMED_OBJECT_HINT("end_seq", endseq, ARCHIVE_FLAGS);
	}
	else {
		ar & NAMED_OBJECT_HINT("begin_seq", (int64_t&)_ring->_baseseq, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("end_seq", (int64_t&)_ring->_endseq, ARCHIVE_FLAGS);
	}

	if ( _level >= INFO_STREAMS ) {
		vector<StreamInfoPtr> streams;
		streams.reserve(_ring->_streams.size());
		for ( const auto &s : _ring->_streams ) {
			if ( _slproto >= 4.0 ||
			     s.second->format() == FMT_MSEED24 ||
			     s.second->format() == FMT_MSEED24_EVENT ||
			     s.second->format() == FMT_MSEED24_CALIBRATION ||
			     s.second->format() == FMT_MSEED24_TIMING ||
			     s.second->format() == FMT_MSEED24_OPAQUE ||
			     s.second->format() == FMT_MSEED24_LOG )
				streams.push_back(new StreamInfo(_slproto, s.second));
		}

		ar & NAMED_OBJECT_HINT("stream", streams, ARCHIVE_FLAGS);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FormatInfo::FormatInfo(double slproto)
: _slproto(slproto) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void FormatInfo::serialize(Core::Archive &ar) {
	for ( const auto &f : Format::_instances ) {
		string code(1, f.second->_code);
		ar & NAMED_OBJECT_HINT(code.c_str(), f.second->_mimetype, ARCHIVE_FLAGS);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Info::Info(double slproto, const string &software, const string &organization,
	   const Core::Time &started, InfoLevel level)
: _slproto(slproto), _software(software), _organization(organization), _started(started)
, _level(level) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Info::addStation(RingPtr ring, const string &description) {
	_stations.push_back(new RingInfo(_slproto, ring, description, _level));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Info::serialize(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("software", _software, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("organization", _organization, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("started", _started, Core::Archive::STATIC_TYPE);

	if ( _level >= INFO_FORMATS && _slproto >= 4.0 ) {
		FormatInfoPtr formatInfo = new FormatInfo(_slproto);
		ar & NAMED_OBJECT_HINT("format", formatInfo, ARCHIVE_FLAGS);
	}

	if ( _level >= INFO_STATIONS )
		ar & NAMED_OBJECT_HINT("station", _stations, ARCHIVE_FLAGS);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Error::Error(const string &code, const string &message)
: _code(code), _message(message) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Error::serialize(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("code", _code, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("message", _message, ARCHIVE_FLAGS);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
InfoError::InfoError(double slproto, const string &software, const string &organization,
		     const Core::Time &started, const string &code, const string &message)
: Info(slproto, software, organization, started, INFO_ID), _slproto(slproto), _error(code, message) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void InfoError::serialize(Core::Archive &ar) {
	Info::serialize(ar);
	ar & NAMED_OBJECT_HINT("error", _error, ARCHIVE_FLAGS);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

