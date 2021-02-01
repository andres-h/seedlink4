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

#ifndef SEISCOMP_APPS_SEEDLINK_STORAGE_H__
#define SEISCOMP_APPS_SEEDLINK_STORAGE_H__

#include <cstdint>
#include <set>
#include <map>
#include <deque>
#include <regex>

#include <seiscomp/core/baseobject.h>
#include <seiscomp/core/datetime.h>

#include "record.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


typedef uint64_t Sequence;
const Sequence UNSET = Sequence(-1);


DEFINE_SMARTPOINTER(Segment);
class Segment : public Core::BaseObject {
	public:
		Segment(const std::string &path, Sequence baseseq, int nblocks, int blocksize);

		~Segment();

		void remove();
		Sequence startSeq() { return _startseq; }
		Sequence endSeq() { return _endseq; }
		Core::Time startEndTime() { return _startendtime; }
		Core::Time endtime() { return _endtime; }

		void put(RecordPtr rec, Sequence seq);
		RecordPtr get(Sequence seq);

	private:
		std::string _path;
		const Sequence _baseseq;
		const int _nblocks;
		const int _blocksize;
		Sequence _startseq;
		Sequence _endseq;
		Core::Time _startendtime;
		Core::Time _endtime;
		std::streambuf *_sb;
};


DEFINE_SMARTPOINTER(Selector);
class Selector : public Core::BaseObject {
	public:
		bool init(const std::string &selstr);
		bool match(RecordPtr rec);
		bool negative() { return _neg; }

	private:
		bool _neg;
		std::regex _rloc;
		std::regex _rcha;
		std::regex _rtype;

		bool initPattern(std::regex &r, const std::string &s, bool simple = false);
};


class CursorOwner;
class CursorClient;

DEFINE_SMARTPOINTER(Cursor);
class Cursor : public Core::BaseObject {
	public:
		Cursor(CursorOwner &owner,
		       CursorClient &client,
		       const std::string &ringName,
		       const std::deque<SegmentPtr> &segments)
		: _owner(owner), _client(client), _ringName(ringName)
		, _segments(segments), _segmentIndex(0), _seq(UNSET)
		, _dialup(false), _has_data(false), _eod(false) {}

		~Cursor();

		std::string ringName();
		void setSequence(Sequence seq, bool seq24bit = false);
		void setDialup(bool dialup);
		void setStart(Core::Time t);
		void setEnd(Core::Time t);
		bool select(const std::string &sel);
		void accept(FormatCode format);
		RecordPtr next();
		Sequence sequence();
		bool endOfData();
		void dataAvail(Sequence seq);


	private:
		CursorOwner &_owner;
		CursorClient &_client;
		const std::string _ringName;
		const std::deque<SegmentPtr> &_segments;
		unsigned int _segmentIndex;
		Sequence _seq;
		bool _dialup;
		bool _has_data;
		bool _eod;
		Core::Time _starttime;
		Core::Time _endtime;
		std::list<SelectorPtr> _selectors;
		std::set<FormatCode> _formats;

		bool match(RecordPtr rec);
};


class CursorClient {
	public:
		virtual void cursorAvail(CursorPtr c, Sequence _seq) =0;
};


class CursorOwner {
	public:
		virtual void removeCursor(Cursor* c) =0;
};


DEFINE_SMARTPOINTER(Ring);
class Ring : public Core::BaseObject, private CursorOwner {
	public:
		Ring(const std::string &path, const std::string &name, int nsegments, int segsize, int blocksize);
		Ring(const std::string &path, const std::string &name);

		bool check(int nsegments, int segsize, int blocksize);
		bool put(RecordPtr buf, Sequence seq, bool seq24bit);
		CursorPtr cursor(CursorClient &client);

	private:
		const std::string _path;
		const std::string _name;
		int _nsegments;
		int _segsize;
		int _blocksize;
		Sequence _baseseq;
		Sequence _endseq;
		std::deque<SegmentPtr> _segments;
		std::set<Cursor*> _cursors;

		void removeCursor(Cursor* c);
};


DEFINE_SMARTPOINTER(Storage);
class Storage : public Core::BaseObject {
	public:
		Storage(const std::string &path);

		RingPtr createRing(const std::string &name,
			     int nsegments,
			     int segsize,
			     int recsize);

		RingPtr ring(const std::string &name);

		std::vector<std::string> cat();

	private:
		const std::string _path;
		std::map<std::string, RingPtr> _rings;
};

}
}
}


#endif
