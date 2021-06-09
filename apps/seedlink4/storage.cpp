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
Segment::Segment(const string &path, Sequence baseseq, int nblocks, int blocksize)
: _baseseq(baseseq), _nblocks(nblocks), _blocksize(blocksize)
, _startseq(UNSET), _endseq(UNSET), _sb(NULL) {
	char fname[30];
	snprintf(fname, 30, "%016llX", (unsigned long long)baseseq);
	_path = path + "/" + fname;
	boost::iostreams::mapped_file_params p;
	p.path = _path;
	p.flags = boost::iostreams::mapped_file::readwrite;

	if ( !Util::fileExists(_path) ) {
		p.new_file_size = nblocks * blocksize;
		_sb = new boost::iostreams::stream_buffer<boost::iostreams::mapped_file>(p);
		return;
	}

	_sb = new boost::iostreams::stream_buffer<boost::iostreams::mapped_file>(p);

	int i;
	// assume that records are sorted by end time
	for ( i = 0; i < nblocks; ++i ) {
		_sb->pubseekoff(i * blocksize, ios_base::beg);

		if ( _sb->sgetc() ) {
			IO::BinaryArchive ar(_sb);
			RecordPtr rec = new Record();
			rec->serializeHeader(ar);

			if ( !ar.success() )
				throw runtime_error(path + " corrupt at block " + to_string(i));

			_startseq = baseseq + i;
			_startendtime = rec->startTime() + rec->timeSpan();
			break;
		}
	}

	if ( i != nblocks ) {
		for ( i = nblocks - 1; i >= 0; --i ) {
			_sb->pubseekoff(i * blocksize, ios_base::beg);

			if ( _sb->sgetc() ) {
				IO::BinaryArchive ar(_sb);
				RecordPtr rec = new Record();
				rec->serializeHeader(ar);

				if ( !ar.success() )
					throw runtime_error(path + " corrupt at block " + to_string(i));

				_endseq = baseseq + i + 1;
				_endtime = rec->startTime() + rec->timeSpan();
				break;
			}
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Segment::~Segment() {
	if ( _sb ) delete _sb;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Segment::remove() {
	if ( Util::pathExists(_path) && ::remove(_path.c_str()) < 0)
		throw std::system_error(errno, std::generic_category(), _path);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Segment::put(RecordPtr rec, Sequence seq) {
	if ( seq < _baseseq || seq >= _baseseq + _nblocks )
		throw logic_error("invalid seq " + to_string(seq));

	_sb->pubseekoff((seq - _baseseq) * _blocksize, ios_base::beg);

	IO::BinaryArchive ar;
	ar.create(_sb);

	rec->serializeHeader(ar);
	rec->serializePayload(ar);

	if ( !ar.success() )
		throw logic_error("could not serialize record");

	if ( _startseq == UNSET ) {
		_startseq = seq;
		_endseq = seq + 1;
		_startendtime = _endtime = rec->startTime() + rec->timeSpan();

	}
       	else if ( seq < _startseq ) {
		_startseq = seq;
		_startendtime = rec->startTime() + rec->timeSpan();

	}
       	else if ( seq >= _endseq ) {
		_endseq = seq + 1;
		_endtime = rec->startTime() + rec->timeSpan();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RecordPtr Segment::get(Sequence seq) {
	if ( seq < _baseseq || seq >= _baseseq + _nblocks )
		throw logic_error("invalid seq " + to_string(seq));

	if ( _startseq == UNSET || seq < _startseq || seq >= _endseq )
		return NULL;

	_sb->pubseekoff((seq - _baseseq) * _blocksize, ios_base::beg);

	if ( !_sb->sgetc() )
		return NULL;

	IO::BinaryArchive ar;
	ar.open(_sb);

	RecordPtr rec = new Record();
	rec->serializeHeader(ar);
	rec->serializePayload(ar);

	if ( !ar.success() )
		throw runtime_error("could not de-serialize record");

	return rec;
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
void Cursor::setSequence(Sequence seq, bool seq24bit) {
	Sequence endseq = 0;
	deque<SegmentPtr>::const_reverse_iterator i;
	for ( i = _segments.rbegin(); i != _segments.rend(); ++i ) {
		if ( *i ) {
			endseq = (*i)->endSeq();
			break;
		}
	}

	if ( seq24bit ) {
		_seq = (endseq & ~0xffffffULL) | (seq & 0xffffff);

		if ( _seq > endseq )
			_seq -= 0x1000000;
	}
	else {
		_seq = seq;

		if ( _seq > endseq )
			_seq = endseq;
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
	if ( _segmentIndex >= _segments.size() )
		_segmentIndex = _segments.size() - 1;

	SegmentPtr s = _segments[_segmentIndex];

	if ( !s || _seq < s->startSeq() || _seq >= s->endSeq() ) {
		for ( _segmentIndex = 0; _segmentIndex < _segments.size(); ++_segmentIndex ) {
			s = _segments[_segmentIndex];
			if ( s && s->endSeq() > _seq &&
			     (_starttime == Core::Time::Null || s->startEndTime() >= _starttime) )
				break;
		}
	}

	if ( !s )
		return NULL;

	RecordPtr rec;

	for ( ; _segmentIndex < _segments.size(); ++_segmentIndex ) {
		s = _segments[_segmentIndex];

		if ( !s ) continue;

		if ( _seq < s->startSeq() )
			_seq = s->startSeq();

		while ( _seq < s->endSeq() ) {
			rec = s->get(_seq++);

			if ( rec &&
			     (_starttime == Core::Time::Null || rec->startTime() + rec->timeSpan() >= _starttime) ) {
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
	}

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
	if ( _seq == UNSET )
		_seq = seq;

       	_client.cursorAvail(this, seq);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Stream::Stream(const string &loc,
	       const string &cha,
	       const string &type,
	       const Core::Time &starttime,
	       const Core::Time &endtime)
: _loc(loc), _cha(cha), _type(type), _starttime(starttime), _endtime(endtime) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Ring::Ring(const string &path, const string &name, int nsegments, int segsize, int blocksize)
: _path(path + "/" + name), _name(name), _nsegments(nsegments), _segsize(segsize), _blocksize(blocksize)
, _baseseq(0), _endseq(0), _segments(nsegments) {
	const string segpath = _path + "/segments";
	const string jsonfile = _path + "/ring.json";

	if ( !Util::pathExists(segpath) && !Util::createPath(segpath) )
		throw runtime_error("could not create " + segpath);

	IO::JSONArchive ar;

	if ( !ar.create(jsonfile.c_str(), false) )
		throw runtime_error("could not create " + jsonfile);

	ar & NAMED_OBJECT_HINT("nsegments", _nsegments, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("segsize", _segsize, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("blocksize", _blocksize, Core::Archive::STATIC_TYPE);
	ar.close();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Ring::Ring(const string &path, const string &name)
: _path(path + "/" + name), _name(name), _nsegments(0), _segsize(0), _blocksize(0)
, _baseseq(UNSET), _endseq(0), _ordered(false) {
	// TODO: Quick save/restore ring state.
	// TODO: Reduce the number of file descriptors needed (eg., caching,
	// perhaps using a single circular buffer instead of multiple segments).
	const string segpath = _path + "/segments";
	const string jsonfile = _path + "/ring.json";

	if ( !Util::pathExists(segpath) )
		throw runtime_error(segpath + " does not exist");

	IO::JSONArchive ar;

	if ( !ar.open(jsonfile.c_str()) )
		throw runtime_error("could not open " + jsonfile);

	ar & NAMED_OBJECT_HINT("nsegments", _nsegments, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("segsize", _segsize, Core::Archive::STATIC_TYPE);
	ar & NAMED_OBJECT_HINT("blocksize", _blocksize, Core::Archive::STATIC_TYPE);
	ar.close();

	_segments.resize(_nsegments);

	set<Sequence> seqs;
	boost::filesystem::path p(segpath);
	boost::filesystem::directory_iterator end;
	for ( boost::filesystem::directory_iterator i(p); i != end; ++i ) {
		try {
			string fname = i->path().filename().generic_string();
			size_t end;
			unsigned long long seq = stoull(fname, &end, 16);

			if ( end != fname.length() || seq == UNSET)
				throw runtime_error("invalid ringbuffer (file name) at " + segpath);

			if ( boost::filesystem::file_size(i->path()) != size_t(_segsize * _blocksize) )
				throw runtime_error("invalid ringbuffer (file size) at " + segpath);

			if ( seq < _baseseq )
				_baseseq = seq;

			seqs.insert(seq);
		}
		catch(const invalid_argument&) {
			throw runtime_error("invalid ringbuffer (file name) at " + segpath);
		}
	}

	for ( int n = 0; n <_nsegments; ++n ) {
		set<Sequence>::iterator i = seqs.find(_baseseq + n * _segsize);
		if ( i != seqs.end() ) {
			seqs.erase(i);
			SegmentPtr s = new Segment(segpath, _baseseq + n * _segsize, _segsize, _blocksize);

			if ( s->endSeq() != UNSET ) {
				_endseq = s->endSeq();
				_segments[n] = s;
			}
		}
	}

	if ( !seqs.empty() )
		throw runtime_error("invalid ringbuffer (unexpected file) at " + segpath);

	if ( _baseseq == UNSET )
		_baseseq = 0;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool Ring::check(int nsegments, int segsize, int blocksize) {
	bool result = true;

	if ( _nsegments != nsegments ) {
		SEISCOMP_WARNING("found existing ring %s with number of segments = %d (requested %d)",
				_name.c_str(),
				_nsegments,
				nsegments);

		result = false;
	}

	if ( _segsize != segsize ) {
		SEISCOMP_WARNING("found existing ring %s with segment size = %d (requested %d)",
				_name.c_str(),
				_segsize,
				segsize);

		result = false;
	}

	if ( _blocksize != blocksize ) {
		SEISCOMP_WARNING("found existing ring %s with block size = %d (requested %d)",
				_name.c_str(),
				_blocksize,
				blocksize);

		result = false;
	}

	return result;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Ring::setOrdered(bool ordered) {
	_ordered = ordered;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool Ring::put(RecordPtr rec, Sequence seq, bool seq24bit) {
	if ( seq == UNSET )
		seq = _endseq;
	else if ( seq24bit )
		seq = (_endseq & ~0xffffffULL) | (seq & 0xffffff);

	if ( seq < _baseseq )
		return false;

	int shift = (seq - _baseseq) / _segsize - _nsegments + 1;
	int n;

	if ( shift > _nsegments ) {
		deque<SegmentPtr>::iterator i = _segments.begin();
		while ( i < _segments.end() ) {
			if ( *i )
				(*i)->remove();

			_segments.erase(i++);
		}

		_segments.resize(_nsegments);
		_baseseq = seq - (_nsegments - 1) * _segsize;
		n = _nsegments - 1;

	}
       	else if ( shift > 0 ) {
		deque<SegmentPtr>::iterator i = _segments.begin();
		while ( i < _segments.begin() + shift ) {
			if ( *i )
				(*i)->remove();

			_segments.erase(i++);
		}

		_segments.resize(_nsegments);
		_baseseq += shift * _segsize;
		n = _nsegments - 1;

	}
       	else {
		n = (seq - _baseseq) / _segsize;
	}

	SegmentPtr s = _segments[n];

	if ( !s ) {
		s = new Segment(_path + "/segments", _baseseq + n * _segsize, _segsize, _blocksize);
		_segments[n] = s;
	}

	s->put(rec, seq);

	for ( auto i : _cursors )
		i->dataAvail(seq);

	if ( _endseq < seq + 1 )
		_endseq = seq + 1;

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
CursorPtr Ring::cursor(CursorClient &client) {
	Cursor* c = new Cursor(*this, client, _name, _segments);
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
			_rings.insert(pair<string, RingPtr>(name, ring));
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RingPtr Storage::createRing(const string &name,
			    int nsegments,
			    int segsize,
			    int blocksize) {
	RingPtr ring = new Ring(_path, name, nsegments, segsize, blocksize);
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

