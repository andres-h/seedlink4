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

#include "selector.h"
#include "record.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


class CursorOwner;
class CursorClient;

DEFINE_SMARTPOINTER(Cursor);
class Cursor : public Core::BaseObject {
	public:
		Cursor(CursorOwner &owner,
		       CursorClient &client,
		       const std::string &ringName);

		~Cursor();

		std::string ringName();
		void setSequence(Sequence seq, double slproto);
		void setDialup(bool dialup);
		void setStart(Core::Time t);
		void setEnd(Core::Time t);
		void select(SelectorPtr sel);
		void accept(FormatCode format);
		RecordPtr next();
		Sequence sequence();
		bool endOfData();
		void dataAvail(Sequence seq);


	private:
		CursorOwner &_owner;
		CursorClient &_client;
		const std::string _ringName;
		Sequence _seq;
		bool _seq24bit;
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
		virtual RecordPtr get(Sequence seq) =0;
		virtual Sequence sequence() =0;
};


DEFINE_SMARTPOINTER(Stream);
class Stream : public Core::BaseObject {
	DECLARE_SC_CLASS(Stream)
	friend class StreamInfo;

	public:
		Stream(): _format(0) {}
		Stream(const std::string &loc,
		       const std::string &cha,
		       FormatCode format,
		       const Core::Time &starttime,
		       const Core::Time &endtime);

		std::string id();
		FormatCode format();

		Core::Time startTime();
		Core::Time endTime();

		void setStartTime(const Core::Time &starttime);
		void setEndTime(const Core::Time &endtime);

		void serialize(Core::Archive &ar);

	private:
		std::string _loc;
		std::string _cha;
		FormatCode _format;
		Core::Time _starttime;
		Core::Time _endtime;
};


DEFINE_SMARTPOINTER(Ring);
class Ring : public Core::BaseObject, private CursorOwner {
	friend class RingInfo;

	public:
		Ring(const std::string &path, const std::string &name,
		     int nblocks=0, int blocksize=0);

		~Ring();

		void setOrdered(bool ordered);
		void serialize(Core::Archive &ar);
		bool load();
		void save();
		bool ensure(int nblocks, int blocksize);
		bool put(RecordPtr buf, Sequence seq);
		Sequence sequence();
		CursorPtr cursor(CursorClient &client);

	private:
		const std::string _path;
		const std::string _name;
		int _nblocks;
		int _blocksize;
		int _shift;
		Sequence _baseseq;
		Sequence _startseq;
		Sequence _endseq;
		bool _ordered;
		std::streambuf *_sb;
		std::map<std::string, StreamPtr> _streams;
		std::set<Cursor*> _cursors;

		void removeCursor(Cursor* c);
		RecordPtr get(Sequence seq);
};


DEFINE_SMARTPOINTER(Storage);
class Storage : public Core::BaseObject {
	public:
		Storage(const std::string &path);

		RingPtr createRing(const std::string &name,
				   int nblocks,
				   int blocksize);

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
