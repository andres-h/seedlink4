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
	if ( _slproto < 4.0 ) {
		size_t sep = _stream->_name.find('_');
		if ( sep == string::npos )
			return;

		string location = _stream->_name.substr(0, sep);
		string ch = _stream->_name.substr(sep + 1);

		if ( ch.length() != 5 || ch[1] != '_' || ch[3] != '_' )
			return;

		string seedname = ch.substr(0, 1) + ch.substr(2, 1) + ch.substr(4, 1);
		string type(_stream->_format, 1, 1);
		string starttime = _stream->_starttime.toString("%F %T.%4f");
		string endtime = _stream->_endtime.toString("%F %T.%4f");
		ar & NAMED_OBJECT_HINT("location", location, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("seedname", seedname, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("type", type, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("begin_time", starttime, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("end_time", endtime, ARCHIVE_FLAGS);
	}
	else {
		string format(_stream->_format, 0, 1);
		string subformat(_stream->_format, 1, 1);
		ar & NAMED_OBJECT_HINT("id", _stream->_name, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("format", format, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("subformat", subformat, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("start_time", _stream->_starttime, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("end_time", _stream->_endtime, ARCHIVE_FLAGS);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RingInfo::RingInfo(double slproto, RingPtr ring, const string &description,
		   const regex &streamRegex, enum InfoLevel level)
: _slproto(slproto), _ring(ring), _description(description)
, _streamRegex(streamRegex), _level(level) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RingInfo::serialize(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("description", _description, ARCHIVE_FLAGS);

	if ( _slproto < 4.0 ) {
		size_t sep = _ring->_name.find('_');
		if ( sep == string::npos )
			return;

		string net = _ring->_name.substr(0, sep);
		string sta = _ring->_name.substr(sep + 1);

		string baseseq;
		baseseq.resize(7);
		snprintf(&baseseq[0], 7, "%06llX", (long long unsigned int)_ring->_baseseq);

		string endseq;
		endseq.resize(7);
		snprintf(&endseq[0], 7, "%06llX", (long long unsigned int)_ring->_endseq);

		string enabled = "enabled";

		ar & NAMED_OBJECT_HINT("name", sta, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("network", net, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("begin_seq", baseseq, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("end_seq", endseq, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("stream_check", enabled, ARCHIVE_FLAGS);
	}
	else {
		ar & NAMED_OBJECT_HINT("id", _ring->_name, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("start_seq", (int64_t&)_ring->_baseseq, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("end_seq", (int64_t&)_ring->_endseq, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("backfill", _ring->_backfill, ARCHIVE_FLAGS);
	}

	if ( _level >= INFO_STREAMS ) {
		vector<StreamInfoPtr> streams;
		streams.reserve(_ring->_streams.size());
		for ( const auto &s : _ring->_streams ) {
			if ( regex_match(s.second->id(), _streamRegex) )
				streams.push_back(new StreamInfo(_slproto, s.second));
		}

		ar & NAMED_OBJECT_HINT("stream", streams, ARCHIVE_FLAGS);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SubformatInfo::SubformatInfo(double slproto, const map<char, Format*> &subformats)
: _slproto(slproto), _subformats(subformats) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SubformatInfo::serialize(Core::Archive &ar) {
	for ( const auto &f : _subformats ) {
		ar & NAMED_OBJECT_HINT(string(1, f.first).c_str(), f.second->_description, ARCHIVE_FLAGS);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FormatInfo::FormatInfo(double slproto, const map<char, Format*> &subformats)
: _slproto(slproto), _subformats(subformats) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void FormatInfo::serialize(Core::Archive &ar) {
	std::map<char, Format*>::iterator it = _subformats.begin();

	if ( it != _subformats.end() ) {
		SubformatInfoPtr subformatInfo = new SubformatInfo(_slproto, _subformats);
		ar & NAMED_OBJECT_HINT("mimetype", it->second->_mimetype, ARCHIVE_FLAGS);
		ar & NAMED_OBJECT_HINT("subformat", subformatInfo, ARCHIVE_FLAGS);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FormatsInfo::FormatsInfo(double slproto)
: _slproto(slproto) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void FormatsInfo::serialize(Core::Archive &ar) {
	for ( const auto &f : Format::_instances ) {
		FormatInfoPtr formatInfo = new FormatInfo(_slproto, f.second);
		ar & NAMED_OBJECT_HINT(string(1, f.first).c_str(), formatInfo, ARCHIVE_FLAGS);
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
void Info::addStation(RingPtr ring, const string &description, const regex &streamRegex) {
	_stations.push_back(new RingInfo(_slproto, ring, description, streamRegex, _level));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Info::serialize(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("software", _software, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("organization", _organization, ARCHIVE_FLAGS);
	ar & NAMED_OBJECT_HINT("started", _started, Core::Archive::STATIC_TYPE);

	if ( _level >= INFO_FORMATS && _slproto >= 4.0 ) {
		FormatsInfoPtr formatsInfo = new FormatsInfo(_slproto);
		ar & NAMED_OBJECT_HINT("format", formatsInfo, ARCHIVE_FLAGS);
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

