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

#include <exception>
#include <cstdio>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>

#define SEISCOMP_COMPONENT SEEDLINK
#include <seiscomp/logging/log.h>
#include <seiscomp/utils/files.h>
#include <seiscomp/io/archive/binarchive.h>
#include <seiscomp/io/archive/jsonarchive.h>

#include "storage.h"


using namespace std;
using namespace Seiscomp;


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Cursor::Cursor(CursorOwner &owner, CursorClient &client, const std::string &ringName)
: _owner(owner), _client(client), _ringName(ringName)
, _seq(SEQ_UNSET), _dialup(false), _has_data(false), _eod(false) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Cursor::~Cursor() {
	_owner.removeCursor(this);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
string Cursor::ringName() {
	return _ringName;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Cursor::setSequence(Sequence seq, double slproto) {
	if ( seq == SEQ_UNSET ) {
		_seq = _owner.sequence();
	}
	else {
		if ( slproto < 4.0 ) {  // 24-bit sequence with wrap
			_seq = (_owner.sequence() & ~0xffffffULL) | (seq & 0xffffff);

			if ( _seq > _owner.sequence() )
				_seq -= 0x1000000;
		}
		else if ( seq > _owner.sequence() ) {
				_seq = _owner.sequence();
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Cursor::setDialup(bool dialup) {
	_dialup = dialup;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Cursor::setStart(Core::Time t) {
	_starttime = t;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Cursor::setEnd(Core::Time t) {
	_endtime = t;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Cursor::select(SelectorPtr sel) {
	if ( !sel )
		_selectors.clear();
	else
		_selectors.push_back(sel);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Cursor::accept(FormatCode format) {
	if ( format == 0 )
		_formats.clear();
	else
		_formats.insert(format);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Cursor::match(RecordPtr rec) {
	if ( _formats.size() != 0 && _formats.find(rec->format()) == _formats.end() )
		return false;

	bool default_rule = true, result = false;
	for ( auto i : _selectors ) {
		if ( i->negative() ) {
			if ( i->match(rec) ) return false;
		}
		else {
			default_rule = false;
			result |= i->match(rec);
		}
	}

	return (default_rule || result);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RecordPtr Cursor::next() {
	RecordPtr rec;

	while ( (rec = _owner.get(_seq)) != NULL ) {
		_seq = rec->sequence() + 1;

		if ( _starttime == Core::Time::Null || rec->endTime() >= _starttime ) {
			if ( _endtime != Core::Time::Null && rec->startTime() > _endtime ) {
				_eod = true;
				_owner.removeCursor(this);
				return NULL;
			}

			if ( !match(rec) )
				continue;

			_has_data = true;
			return rec;
		}
	}

	_seq = _owner.sequence();

	if ( _dialup && _has_data ) {
		_eod = true;
		_owner.removeCursor(this);
	}

	return NULL;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Sequence Cursor::sequence() {
	return _seq;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Cursor::endOfData() {
	return _eod;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Cursor::dataAvail(Sequence seq) {
	if ( _seq == SEQ_UNSET )
		_seq = seq;

       	_client.cursorAvail(this, seq);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
IMPLEMENT_SC_CLASS(Stream, "Seiscomp::Applications::Seedlink::Stream");
Stream::Stream(const string &loc,
	       const string &cha,
	       FormatCode format,
	       const Core::Time &starttime,
	       const Core::Time &endtime)
: _loc(loc), _cha(cha), _format(format), _starttime(starttime), _endtime(endtime) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
string Stream::id() {
	return _loc + "." + _cha + "." + string(1, _format);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FormatCode Stream::format() {
	return _format;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Time Stream::startTime() {
	return _starttime;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Time Stream::endTime() {
	return _endtime;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Stream::setStartTime(const Core::Time &starttime) {
	_starttime = starttime;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Stream::setEndTime(const Core::Time &endtime) {
	_endtime = endtime;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Stream::serialize(Core::Archive &ar) {
	string format(1, _format);

	ar & NAMED_OBJECT_HINT("location", _loc, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("channel", _cha, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("format", format, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("startTime", _starttime, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("endTime", _endtime, Core::Archive::STATIC_TYPE);

	_format = format.c_str()[0];
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Ring::Ring(const string &path, const string &name, int nblocks, int blocksize)
: _path(path + "/" + name), _name(name), _nblocks(nblocks), _blocksize(blocksize)
, _shift(0), _baseseq(0), _startseq(SEQ_UNSET), _endseq(0), _ordered(false) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Ring::~Ring() {
	// TODO: call this from somewhere else
	save();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Ring::setOrdered(bool ordered) {
	_ordered = ordered;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Ring::serialize(Core::Archive &ar) {
	std::vector<StreamPtr> streams;

	if ( !ar.isReading() ) {
		streams.reserve(_streams.size());
		for ( const auto &s : _streams )
			streams.push_back(s.second);
	}

	ar & NAMED_OBJECT_HINT("nblocks", _nblocks, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("blocksize", _blocksize, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("shift", _shift, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("baseseq", (int64_t&)_baseseq, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("startseq", (int64_t&)_startseq, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("endseq", (int64_t&)_endseq, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("streams", streams, Core::Archive::STATIC_TYPE);

	if ( ar.isReading() ) {
		for ( const auto &s : streams )
			_streams.insert(pair<string, StreamPtr>(s->id(), s));
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Ring::load() {
	if ( !Util::pathExists(_path) && !Util::createPath(_path) )
		throw runtime_error("could not create " + _path);

	const string datafile = _path + "/ring.dat";
	const string jsonfile = _path + "/ring.json";
	const string jsonbak = _path + "/ring.json.bak";
	bool recover = false;

	boost::iostreams::mapped_file_params p;
	p.path = datafile;
	p.flags = boost::iostreams::mapped_file::readwrite;

	if ( !_nblocks ) {
		if ( Util::fileExists(jsonfile) ) {
			IO::JSONArchive ar;

			if ( !ar.open(jsonfile.c_str()) )
				throw runtime_error("could not open " + jsonfile);

			serialize(ar);

			if ( !ar.success() )
				throw runtime_error("could not parse " + jsonfile);

			ar.close();

			if ( Util::fileExists(jsonbak) && ::remove(jsonbak.c_str()) < 0 )
				throw std::system_error(errno, std::generic_category(), jsonbak);

			if ( ::rename(jsonfile.c_str(), jsonbak.c_str()) < 0 )
				throw std::system_error(errno, std::generic_category(), _path);
		}
		else if ( Util::fileExists(jsonbak) ) {
			recover = true;

			IO::JSONArchive ar;

			if ( !ar.open(jsonbak.c_str()) )
				throw runtime_error("could not open " + jsonbak);

			serialize(ar);

			if ( !ar.success() )
				throw runtime_error("could not parse " + jsonbak);

			ar.close();
		}
	}
	else {
		if ( Util::fileExists(datafile) && ::remove(datafile.c_str()) < 0 )
			throw std::system_error(errno, std::generic_category(), datafile);

		if ( Util::fileExists(jsonfile) && ::remove(jsonfile.c_str()) < 0 )
			throw std::system_error(errno, std::generic_category(), jsonfile);

		if ( Util::fileExists(jsonbak) && ::remove(jsonbak.c_str()) < 0 )
			throw std::system_error(errno, std::generic_category(), jsonbak);
	}

	if ( !_nblocks || !_blocksize )
		return false;

	if ( !Util::fileExists(datafile) ) {
		_shift = 0;
		_baseseq = 0;
		_startseq = SEQ_UNSET;
		_endseq = 0;
		_streams.clear();

		IO::JSONArchive ar;

		if ( !ar.create(jsonbak.c_str(), false) )
			throw runtime_error("could not create " + jsonfile);

		serialize(ar);

		if ( !ar.success() )
			throw runtime_error("could not write " + jsonbak);

		ar.close();

		p.new_file_size = _nblocks * _blocksize;
		_sb = new boost::iostreams::stream_buffer<boost::iostreams::mapped_file>(p);
		return true;
	}

	_sb = new boost::iostreams::stream_buffer<boost::iostreams::mapped_file>(p);

	if ( recover ) {
		_shift = 0;
		_baseseq = 0;
		_startseq = SEQ_UNSET;
		_endseq = 0;
		_streams.clear();

		for ( int i = 0; i < _nblocks; ++i ) {
			_sb->pubseekoff(i * _blocksize, ios_base::beg);

			if ( !_sb->sgetc() )
				continue;

			IO::BinaryArchive ar(_sb);
			RecordPtr rec = new Record();
			rec->serializeHeader(ar);

			if ( !ar.success() )
				throw runtime_error(datafile + " corrupt at block " + to_string(i));

			ar.close();

			if ( rec->sequence() < _startseq ) {
				_startseq = rec->sequence();
			}

			if ( rec->sequence() >= _endseq ) {
				_endseq = rec->sequence() + 1;

				if ( _endseq > (unsigned int)_nblocks ) {
					_shift = (i + 1) % _nblocks;
					_baseseq = _endseq - _nblocks;
				}
				else {
					_shift = (i + 1 + _nblocks - _endseq) % _nblocks;
					_baseseq = 0;
				}
			}

			map<string, StreamPtr>::iterator it = _streams.find(rec->stream());
			if ( it == _streams.end() ) {
				StreamPtr s = new Stream(rec->location(),
							 rec->channel(),
							 rec->format(),
							 rec->startTime(),
							 rec->endTime());

				_streams.insert(pair<string, StreamPtr>(s->id(), s));
			}
			else {
				StreamPtr s = it->second;

				if ( rec->startTime() < s->startTime() )
					s->setStartTime(rec->startTime());

				if ( rec->endTime() > s->endTime() )
					s->setEndTime(rec->endTime());
			}
		}
	}
	else {
		// TODO: check
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Ring::save() {
	const string jsonfile = _path + "/ring.json";
	const string jsonbak = _path + "/ring.json.bak";

	IO::JSONArchive ar;

	if ( !ar.create(jsonfile.c_str(), false) )
		throw runtime_error("could not create " + jsonfile);

	serialize(ar);

	if ( !ar.success() )
		throw runtime_error("could not write " + jsonfile);

	ar.close();

	if ( Util::fileExists(jsonbak) && ::remove(jsonbak.c_str()) < 0 )
		throw std::system_error(errno, std::generic_category(), jsonbak);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool Ring::ensure(int nblocks, int blocksize) {
	bool recreate = false;

	if ( _nblocks != nblocks ) {
		SEISCOMP_WARNING("found existing ring %s with number of segments = %d (requested %d)",
				_name.c_str(),
				_nblocks,
				nblocks);
		_nblocks = nblocks;
		recreate = true;
	}

	if ( _blocksize != blocksize ) {
		SEISCOMP_WARNING("found existing ring %s with block size = %d (requested %d)",
				_name.c_str(),
				_blocksize,
				blocksize);

		_blocksize = blocksize;
		recreate = true;
	}

	if ( recreate ) {
		if ( !load() )
			throw logic_error("could not recreate " + _path);
	}

	return !recreate;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool Ring::put(RecordPtr rec, Sequence seq) {
	const string datafile = _path + "/ring.dat";

	if ( seq < _baseseq ) {
		SEISCOMP_ERROR("record sequece number is too low (%lu < %lu)",
			       seq,
			       _baseseq);
		return false;
	}

	if ( rec->headerLength() + rec->payloadLength() > (size_t)_blocksize ) {
		SEISCOMP_ERROR("record is larger than blocksize (%lu > %lu)",
			       rec->headerLength() + rec->payloadLength(),
			       (size_t)_blocksize);
		return false;
	}

	if ( seq >= _baseseq + 2 * _nblocks ) {
		for ( int i = 0; i < _nblocks; ++i ) {
			_sb->pubseekoff(i * _blocksize, ios_base::beg);
			_sb->sputc(0);
		}

		_shift = _nblocks - 1;
		_baseseq = seq - _nblocks + 1;
		_streams.clear();
	}
	else {
		while ( seq >= _baseseq + _nblocks ) {
			_sb->pubseekoff(_shift * _blocksize, ios_base::beg);

			if ( _sb->sgetc() ) {
				IO::BinaryArchive ar(_sb);
				RecordPtr rec = new Record();
				rec->serializeHeader(ar);

				if ( !ar.success() )
					throw runtime_error(datafile + " corrupt at block " + to_string(_shift));

				ar.close();

				if ( rec->sequence() != _baseseq )
					throw runtime_error(datafile + " invalid sequence number at block " + to_string(_shift));

				_sb->pubseekoff(_shift * _blocksize, ios_base::beg);
				_sb->sputc(0);

				map<string, StreamPtr>::iterator it = _streams.find(rec->stream());
				if ( it != _streams.end() ) {
					StreamPtr s = it->second;
					s->setStartTime(rec->endTime());
				}
			}

			_shift = (_shift + 1) % _nblocks;
			++_baseseq;

			if ( _startseq < _baseseq )
				_startseq = _baseseq;
		}
	}

	_sb->pubseekoff(((seq - _baseseq + _shift) % _nblocks) * _blocksize, ios_base::beg);

	IO::BinaryArchive ar;
	ar.create(_sb);

	rec->setSequence(seq);
	rec->serializeHeader(ar);
	rec->serializePayload(ar);

	if ( !ar.success() )
		throw logic_error("could not serialize record");

	ar.close();

	map<string, StreamPtr>::iterator it = _streams.find(rec->stream());
	if ( it == _streams.end() ) {
		StreamPtr s = new Stream(rec->location(), rec->channel(), rec->format(), rec->startTime(), rec->endTime());
		_streams.insert(pair<string, StreamPtr>(s->id(), s));
	}
	else {
		StreamPtr s = it->second;
		s->setEndTime(rec->endTime());
	}

	if ( _startseq > seq )
		_startseq = seq;

	if ( _endseq < seq + 1 )
		_endseq = seq + 1;

	// dataAvail() may delete the current item
	set<Cursor*>::iterator i = _cursors.begin();
	while ( i != _cursors.end() )
		(*i++)->dataAvail(seq);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RecordPtr Ring::get(Sequence seq) {
	if ( seq < _startseq )
		seq = _startseq;

	while ( seq < _endseq ) {
		_sb->pubseekoff(((seq - _baseseq + _shift) % _nblocks) * _blocksize, ios_base::beg);

		if ( !_sb->sgetc() ) {
			++seq;
			continue;
		}

		IO::BinaryArchive ar;
		ar.open(_sb);

		RecordPtr rec = new Record();
		rec->serializeHeader(ar);
		rec->serializePayload(ar);

		if ( !ar.success() )
			throw runtime_error("could not de-serialize record");

		ar.close();
		return rec;
	}

	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Sequence Ring::sequence() {
	return _endseq;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
CursorPtr Ring::cursor(CursorClient &client) {
	Cursor* c = new Cursor(*this, client, _name);
	_cursors.insert(c);
	return c;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Ring::removeCursor(Cursor* c) {
	_cursors.erase(c);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Storage::Storage(const string &path)
: _path(path) {
	boost::filesystem::path p(_path);
	boost::filesystem::directory_iterator end;
	for ( boost::filesystem::directory_iterator i(p); i != end; ++i ) {
		if ( boost::filesystem::is_directory(i->path()) ) {
			string name = i->path().filename().generic_string();
			RingPtr ring = new Ring(_path, name);

			if ( !ring->load() ) {
				SEISCOMP_WARNING("invalid ring at %s/%s", _path.c_str(), name.c_str());
				continue;
			}

			_rings.insert(pair<string, RingPtr>(name, ring));
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RingPtr Storage::createRing(const string &name,
			    int nblocks,
			    int blocksize) {
	RingPtr ring = new Ring(_path, name, nblocks, blocksize);

	if ( !ring->load() )
		throw runtime_error("could not initialize ring at " + _path + "/" + name);

	_rings.insert(pair<string, RingPtr>(name, ring));
	return ring;

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RingPtr Storage::ring(const string &name) {
	map<string, RingPtr>::iterator i = _rings.find(name);
	if ( i != _rings.end() )
		return i->second;

	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
vector<string> Storage::cat() {
	vector<string> result;
	result.reserve(_rings.size());
	for ( auto i : _rings )
		result.push_back(i.first);

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

