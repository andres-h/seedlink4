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

namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<int Version>
StreamInfo<Version>::StreamInfo(StreamPtr stream)
: _stream(stream) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<int Version>
void StreamInfo<Version>::serialize(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("location", _stream->_loc, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("seedname", _stream->_cha, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("type", _stream->_type, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("begin_time", _stream->_starttime, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("end_time", _stream->_endtime, Core::Archive::STATIC_TYPE);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<int Version>
RingInfo<Version>::RingInfo(RingPtr ring, const std::string &description, enum InfoLevel level)
: _ring(ring), _description(description), _level(level) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<int Version>
void RingInfo<Version>::serialize(Core::Archive &ar) {
	size_t sep = _ring->_name.find_first_of('.');
	if ( sep == std::string::npos )  // TODO: assert
		return;

	std::string net = _ring->_name.substr(0, sep);
	std::string sta = _ring->_name.substr(sep + 1);

	// TODO: implement uint64_t serialization
	int begin_seq = _ring->_baseseq;
	int end_seq = _ring->_endseq;

	ar & NAMED_OBJECT_HINT("name", sta, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("network", net, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("description", _description, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("begin_seq", begin_seq, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("end_seq", end_seq, Core::Archive::STATIC_TYPE);

	if ( _level >= INFO_STREAMS ) {
		std::vector<StreamInfo<Version> > streamInfo;
		streamInfo.reserve(_ring->_streams.size());
		for ( const auto &s : _ring->_streams )
			streamInfo.push_back(StreamInfo<Version>(s.second));
	
		ar & NAMED_OBJECT_HINT("stream", streamInfo, Core::Archive::STATIC_TYPE);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<int Version>
Info<Version>::Info(const std::string &software, const std::string &organization, const Core::Time &started, InfoLevel level)
: _software(software), _organization(organization), _started(started), _level(level) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<int Version>
void Info<Version>::addStation(RingPtr ring, const std::string &description) {
	_ringInfo.push_back(RingInfo<Version>(ring, description, _level));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<int Version>
void Info<Version>::serialize(Core::Archive &ar) {
	ar & NAMED_OBJECT_HINT("software", _software, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("organization", _organization, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("started", _started, Core::Archive::STATIC_TYPE);

	if ( _level >= INFO_STATIONS )
		ar & NAMED_OBJECT_HINT("station", _ringInfo, Core::Archive::STATIC_TYPE);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

