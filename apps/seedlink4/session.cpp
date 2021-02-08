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
#include <cerrno>

#define SEISCOMP_COMPONENT SEEDLINK
#include <seiscomp/logging/log.h>
#include <seiscomp/core/strings.h>
#include <seiscomp/client/inventory.h>
#include <seiscomp/wired/reactor.h>

#include "session.h"
#include "settings.h"
#include "version.h"


using namespace std;
using namespace Seiscomp;


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


#define TODO(cap) ""


#define CAPABILITIES "SLPROTO:4.0 " \
		     TODO("WEBSOCKET:13 ") \
		     "CAP " \
		     TODO("EXTREPLY ") \
		     "NSWILDCARD " \
		     "BATCH " \
		     "ASYNC " \
		     "AUTH:USERPASS " \
		     TODO("AUTH:TOKEN ") \
		     "MULTISTATION " \
		     "TIME " \
		     TODO("INFO ")



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DEFINE_SMARTPOINTER(Station);
class Station : public Core::BaseObject {
	public:
		void setPattern(const string &p);
		bool match(const string &s);
		void setSequence(Sequence seq, bool seq24bit = false);
		void setDialup(bool dialup);
		void setStart(Core::Time t);
		void setEnd(Core::Time t);
		bool select(const string &selstr);
		CursorPtr cursor(RingPtr ring, CursorClient &client);

	private:
		regex _regex;
		Sequence _seq;
		bool _seq24bit;
		bool _dialup;
		Core::Time _starttime;
		Core::Time _endtime;
		std::list<SelectorPtr> _selectors;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Station::setPattern(const string &p) {
	string r = regex_replace(p, regex("\\."), "\\.");
	r = regex_replace(r, regex("\\?"), ".");
	r = regex_replace(r, regex("\\*"), ".*");
	_regex = regex(r);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Station::match(const string &s) {
	return regex_match(s, _regex);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Station::setSequence(Sequence seq, bool seq24bit) {
	_seq = seq;
	_seq24bit = seq24bit;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Station::setDialup(bool dialup) {
	_dialup = dialup;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Station::setStart(Core::Time t) {
	_starttime = t;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Station::setEnd(Core::Time t) {
	_endtime = t;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Station::select(const string &selstr) {
	if ( selstr.length() == 0 ) {
		_selectors.clear();
		return true;
	}

	SelectorPtr sel = new Selector();
	if ( !sel->init(selstr) )
		return false;

	_selectors.push_back(sel);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
CursorPtr Station::cursor(RingPtr ring, CursorClient &client) {
	CursorPtr cursor = ring->cursor(client);
	cursor->setSequence(_seq, _seq24bit);
	cursor->setStart(_starttime);
	cursor->setEnd(_endtime);
	cursor->setDialup(_dialup);

	for ( auto i : _selectors )
		cursor->select(i);

	return cursor;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class SeedlinkSession : public Wired::ClientSession, private CursorClient {
	public:
		SeedlinkSession(Wired::Socket *sock,
			       	StoragePtr storage,
				const map<FormatCode, FormatPtr> &formats,
				const ACL &trusted,
				const ACL &defaultAccess,
				const map<string, ACL> &access,
				const map<string, string> &descriptions);

		~SeedlinkSession();

		void stationAvail(const string &name);


	private:
                enum SessionType {
                        Unspecific,
                        Feed,
			Client
                };

		Wired::Socket::IPAddress _ipaddress;
		string _user;
		string _useragent;
		SessionType _type;
		StoragePtr _storage;
		const map<FormatCode, FormatPtr> &_formats;
		const ACL &_trusted;
		const ACL &_defaultAccess;
		const map<string, ACL> &_access;
		const map<string, string> &_descriptions;
		list<FormatCode> _accept;
		bool _acceptAll;
		map<string, StationPtr> _stations;
		map<string, StationPtr> _wildcardStations;
		StationPtr _currentStation;
		map<string, CursorPtr> _cursors;
		set<CursorPtr> _cursorsAvail;
		set<CursorPtr>::iterator _cursorIter;
		string _buffer;
		bool _wildcard;
		bool _transfer;
		bool _batch;

		bool checkAccess(const string &station,
				 const Wired::Socket::IPAddress &ip,
				 const string &user);

		void handleReceive(const char *data, size_t len) override;
		void handleInbox(const char *data, size_t len) override;
		void handleDFT(const char *data, size_t len, int dft);
		Core::Time parseTime(const char *data, size_t len);
		void handleFeed(const char *data, size_t len);
		void sendResponse(const char *data);
		void sendResponse(const char *data, int len);
		void startTransfer();
		void stopTransfer();
		void collectData();
		void outboxFlushed() override;
		void cursorAvail(CursorPtr c, Sequence seq);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SeedlinkListener::SeedlinkListener(StoragePtr storage,
				   const map<FormatCode, FormatPtr> &formats,
				   const ACL &trusted,
				   const ACL &defaultAccess,
				   const map<string, ACL> &access,
				   const map<string, string> &descriptions,
                                   Wired::Socket *socket)
: Wired::Endpoint(socket)
, _storage(storage), _formats(formats), _trusted(trusted)
, _defaultAccess(defaultAccess), _access(access), _descriptions(descriptions) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Wired::Session *SeedlinkListener::createSession(Wired::Socket *socket) {
	socket->setMode(Wired::Socket::Read);
	return new SeedlinkSession(socket, _storage, _formats, _trusted,
				   _defaultAccess, _access, _descriptions);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SeedlinkSession::SeedlinkSession(Wired::Socket *sock,
				 StoragePtr storage,
				 const map<FormatCode, FormatPtr> &formats,
				 const ACL &trusted,
				 const ACL &defaultAccess,
				 const map<string, ACL> &access,
				 const map<string, string> &descriptions)
: Wired::ClientSession(sock, 200)
, _type(Unspecific), _storage(storage), _formats(formats), _trusted(trusted)
, _defaultAccess(defaultAccess), _access(access), _descriptions(descriptions)
, _wildcard(false), _transfer(false), _batch(false) {
	_ipaddress = sock->address();
	_currentStation = new Station();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SeedlinkSession::~SeedlinkSession() {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool SeedlinkSession::checkAccess(const string &station,
				  const Wired::Socket::IPAddress &ip,
				  const string &user) {
	if ( _trusted.check(ip, user) )
		return true;

	map<string, ACL>::const_iterator i;
	if ( (i = _access.find(station)) != _access.end() &&
	     !i->second.check(ip, user) )
		return true;

	if ( _defaultAccess.check(ip, user) )
		return true;

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::handleReceive(const char *data, size_t len) {
	if ( _type == Feed ) {
		handleFeed(data, len);
		return;
	}

	for ( ; len > 0; --len, ++data ) {
		if ( *data == '\n' ) continue;
		if ( *data == '\r' ) {
			_inbox[_inboxPos] = '\0';

			handleInbox(&_inbox[0], _inboxPos);

			_inboxPos = 0;
			_inbox[0] = '\0';
		}
		else {
			// If one line of data exceeds the maximum number of allowed
			// characters terminate the connection
			if ( _inboxPos >= _inbox.size() ) {
				handleInboxError(TooManyCharactersPerLine);
				break;
			}

			_inbox[_inboxPos] = *data;
			++_inboxPos;
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::handleInbox(const char *data, size_t len) {
	if ( len == 0 )  // empty line
		return;

	SEISCOMP_DEBUG("$ %s", data);

	stopTransfer();

	const char *tok;
	size_t tokLen;
	if ( !(tok = Core::tokenize(data, " ", len, tokLen)) ) {
		return;
	}

	if ( tokLen == 5 && strncasecmp(tok, "HELLO", tokLen) == 0 ) {
		sendResponse("SeedLink4 v4 (" SEEDLINK4_VERSION_NAME ") :: SLPROTO:4.0 CAP GETCAP\r\n");
		sendResponse(global.organization.c_str(), static_cast<int>(global.organization.size()));
		sendResponse("\r\n");
		return;
	}

	if ( tokLen == 5 && strncasecmp(tok, "BATCH", tokLen) == 0 ) {
		_batch = true;
		sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 9 && strncasecmp(tok, "USERAGENT", tokLen) == 0 ) {
		_useragent = string(data, len);
		sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 4 && strncasecmp(tok, "AUTH", tokLen) == 0 ) {
		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			sendResponse("ERROR\r\n");
			return;
		}

		if ( tokLen == 8 && strncasecmp(tok, "USERPASS", tokLen) == 0 ) {
			if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
				sendResponse("ERROR\r\n");
				return;
			}

			_user = string(tok, tokLen);

			if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
				sendResponse("ERROR\r\n");
				return;
			}

			string password(tok, tokLen);
			// TODO: check password

			sendResponse("OK\r\n");
			return;
		}

		sendResponse("ERROR\r\n");
		return;
	}

	if ( tokLen == 4 && strncasecmp(tok, "FEED", tokLen) == 0 ) {
		if ( _type != Unspecific ) {
			sendResponse("ERROR\r\n");
			return;
		}

		if ( !_trusted.check(_ipaddress, _user) ) {
			SEISCOMP_DEBUG("FEED access denied for %s (%s)",
				       Wired::toString(_ipaddress).c_str(),
				       _user.c_str());

			sendResponse("ERROR\r\n");
			return;
		}

		_type = Feed;
		sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 6 && strncasecmp(tok, "ACCEPT", tokLen) == 0 ) {
		_type = Client;

		while ( (tok = Core::tokenize(data, " ", len, tokLen)) != NULL ) {
			if (strncasecmp(tok, "*", tokLen) == 0 ) {
				_acceptAll = true;
			}
			else {
				FormatCode val;
				if ( !Core::fromString(val, string(tok, tokLen)) ) {
					sendResponse("ERROR\r\n");
					return;
				}

				_accept.push_back(val);
			}
		}
		sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 6 && strncasecmp(tok, "ENABLE", tokLen) == 0 ) {
		// TODO
		sendResponse("ERROR\r\n");
		return;
	}

	if ( tokLen == 12 && strncasecmp(tok, "CAPABILITIES", tokLen) == 0 ) {
		while ( (tok = Core::tokenize(data, " ", len, tokLen)) != NULL ) {
			// TODO
		}

		if ( !_batch ) sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 15 && strncasecmp(tok, "GETCAPABILITIES", tokLen) == 0 ) {
		sendResponse(CAPABILITIES "\r\n");
		return;
	}

	if ( tokLen == 7 && strncasecmp(tok, "STATION", tokLen) == 0 ) {
		_type = Client;

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			if ( !_batch ) sendResponse("ERROR\r\n");
			return;
		}

		string name(tok, tokLen);

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) != NULL )
			name = string(tok, tokLen) + "." + name;
		else
			name = string(global.defaultNetwork + "." + name);

		if ( !regex_match(name, regex("[A-Z0-9\\?\\*]*\\.[A-Z0-9\\?\\*]*")) ) {
			if ( !_batch ) sendResponse("ERROR\r\n");
			return;
		}

		StationPtr sta = new Station();

		if ( name.find('?') != string::npos || name.find('*') != string::npos ) {
			// TODO: limit number of wildcard stations
			sta->setPattern(name);
			_currentStation = _wildcardStations.insert(pair<string, StationPtr>(name, sta)).first->second;
			_wildcard = true;
		}
		else {
			// TODO: limit number of future stations
			_currentStation = _stations.insert(pair<string, StationPtr>(name, sta)).first->second;
			_wildcard = false;
		}

		if ( !_batch ) sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 6 && strncasecmp(tok, "SELECT", tokLen) == 0 ) {
		_type = Client;

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) != NULL ) {
			if ( !_currentStation->select(string(tok, tokLen)) ) {
				if ( !_batch ) sendResponse("ERROR\r\n");
				return;
			}
		}
		else {
			_currentStation->select("");
		}

		if ( !_batch ) sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 4 && strncasecmp(tok, "DATA", tokLen) == 0 ) {
		_type = Client;
		handleDFT(data, len, 1);
		return;
	}

	if ( tokLen == 5 && strncasecmp(tok, "FETCH", tokLen) == 0 ) {
		_type = Client;
		handleDFT(data, len, 2);
		return;
	}

	if ( tokLen == 4 && strncasecmp(tok, "TIME", tokLen) == 0 ) {
		_type = Client;
		handleDFT(data, len, 3);
		return;
	}

	if ( tokLen == 3 && strncasecmp(tok, "END", tokLen) == 0 ) {
		_type = Client;

		if ( _stations.empty() && _wildcardStations.empty() ) {
			// uni-station mode
			sendResponse("ERROR\r\n");
			return;
		}

		startTransfer();
		return;
	}

	if ( tokLen == 3 && strncasecmp(tok, "CAT", tokLen) == 0 ) {
		for ( const auto &name : _storage->cat() ) {
			if ( !checkAccess(name, _ipaddress, _user) )
				continue;

			size_t sep = name.find_first_of('.');
			if ( sep != string::npos ) {
				string net = name.substr(0, sep);
				string sta = name.substr(sep + 1);
				auto descIt = _descriptions.find(name);
				string desc = (descIt != _descriptions.end())? descIt->second: sta;
				sendResponse((net + " " + sta + " " + desc + "\r\n").c_str());
			}
		}

		sendResponse("END");
		return;
	}

	if ( tokLen == 3 && strncasecmp(tok, "BYE", tokLen) == 0 ) {
		close();
		return;
	}

	sendResponse("ERROR\r\n");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::handleDFT(const char *data, size_t len, int dft) {
	_currentStation->setSequence(UNSET);
	_currentStation->setStart(Core::Time::Null);
	_currentStation->setEnd(Core::Time::Null);
	_currentStation->setDialup(dft == 2);

	const char *tok;
	size_t tokLen;

	if ( dft == 1 || dft == 2 ) {  // "DATA" || "FETCH"
		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			if ( _stations.empty() && _wildcardStations.empty() ) {
				// uni-station mode
				startTransfer();
			}
			else if ( !_batch ) {
				sendResponse("OK\r\n");
			}

	 		return;
		}

		if ( tokLen != 2 || strncasecmp(tok, "-1", tokLen) != 0 ) {
			if ( _wildcard ) {
				// cannot use sequence number with wildcard
				if ( !_batch ) sendResponse("ERROR\r\n");
				return;
			}

			try {
				size_t end;
				unsigned long long seq = stoull(string(tok, tokLen), &end, 16);

				if ( end == tokLen && seq < UNSET) {
					bool seq24bit = (tokLen <= 6);
					_currentStation->setSequence(seq, seq24bit);
				}
				else {
					if ( !_batch ) sendResponse("ERROR\r\n");
					return;
				}
			}
			catch(const invalid_argument&) {
				if ( !_batch ) sendResponse("ERROR\r\n");
				return;
			}
		}

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			if ( _stations.empty() && _wildcardStations.empty() ) {
				// uni-station mode
				startTransfer();
			}
			else if ( !_batch ) {
				sendResponse("OK\r\n");
			}

			return;
		}
	}
	else {
		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			if ( !_batch ) sendResponse("ERROR\r\n");
			return;
		}

		_currentStation->setSequence(0);
	}

	Core::Time starttime = parseTime(tok, tokLen);
	if ( !starttime.valid() ) {
		if ( !_batch ) sendResponse("ERROR\r\n");
		return;
	}

	_currentStation->setStart(starttime);

	if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
		if ( _stations.empty() && _wildcardStations.empty() ) {
			// uni-station mode
			startTransfer();
		}
		else if ( !_batch ) {
			sendResponse("OK\r\n");
		}

		return;
	}

	Core::Time endtime = parseTime(tok, tokLen);
	if ( !endtime.valid() ) {
		if ( !_batch ) sendResponse("ERROR\r\n");
		return;
	}

	_currentStation->setEnd(endtime);

	if ( _stations.empty() && _wildcardStations.empty() ) {
		// uni-station mode
		startTransfer();
	}
	else if ( !_batch ) {
		sendResponse("OK\r\n");
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Time SeedlinkSession::parseTime(const char *data, size_t len) {
	vector<unsigned int> t;
	const char *tok;
	size_t tokLen;

	while ( (tok = Core::tokenize(data, ",", len, tokLen)) != NULL && t.size() < 8 ) {
		try {
			size_t end;
			unsigned long val = stoul(string(tok, tokLen), &end, 10);

			if ( end != tokLen)
				return Core::Time::Null;

			t.push_back(val);
		}
		catch (invalid_argument&) {
			return Core::Time::Null;
		}
	}

	if ( t.size() == 6 )
		t.push_back(0);

	if ( t.size() != 7 )
		return Core::Time::Null;

	return Core::Time(t[0], t[1], t[2], t[3], t[4], t[5], t[6] / 1000);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::handleFeed(const char *data, size_t len) {
	if ( _buffer.size() > 0 ) {
		_buffer += string(data, len);
		data = _buffer.data();
		len = _buffer.size();
	}

	while (1) {
		if ( len < 16 )
			break;

		FormatCode formatCode;
		uint32_t packetLength;
		uint32_t headerLength;
		Sequence seq;
		bool seq24bit;

		if ( data[0] == 'S' && data[1] == 'L' ) {  // legacy format
			formatCode = FMT_MSEED24;
			packetLength = 520;
			headerLength = 8;
			seq = UNSET;
			seq24bit = true;

			try {
				size_t end;
				seq = stoull(string(data + 2, 6), &end, 16);

				if ( end != 6 ) {
					SEISCOMP_ERROR("invalid sequence number");
					close();
					return;
				}
			}
			catch(const invalid_argument&) {
				SEISCOMP_ERROR("invalid sequence number");
				close();
				return;
			}
		}
		else if ( data[0] == 'S' && data[1] == 'E' ) {
			formatCode = (FormatCode)data[2];
			packetLength = *(uint32_t *)(data + 4) + 8; // TODO: byteorder
			headerLength = 16;
			seq = *(Sequence *)(data + 8);              // TODO: byteorder
			seq24bit = false;
		}
		else {
			SEISCOMP_ERROR("non-seedlink packet");
			close();
			return;
		}

		if ( len < packetLength )  // not enough data
			break;

		map<FormatCode, FormatPtr>::const_iterator it = _formats.find(formatCode);
		if ( it == _formats.end() ) {
			SEISCOMP_ERROR("unsupported format");
			close();
			return;
		}

		RecordPtr rec;
		if ( it->second->readRecord(data + headerLength, packetLength - headerLength, rec) <= 0 ) {
			SEISCOMP_ERROR("invalid data");
			data += packetLength;
			len -= packetLength;
			continue;
		}

		string ringName = rec->networkCode() + "." + rec->stationCode();

		RingPtr ring = _storage->ring(ringName);

		if ( !ring ) {
			SEISCOMP_INFO("new station %s", ringName.c_str());

			ring = _storage->createRing(ringName,
						    global.segments,
						    global.segsize,
						    global.recsize);

			if ( !ring ) {
				SEISCOMP_ERROR("could not create ring");
				close();
				return;
			}

			for ( auto i : parent()->sessions() ) {
				SeedlinkSession* s = dynamic_cast<SeedlinkSession*>(i.get());

				if ( s )
					s->stationAvail(ringName);
			}
		}

		if ( !ring->put(rec, seq, seq24bit) )
			SEISCOMP_WARNING("dropped %s.%s.%s.%s seq %0*llX",
					 rec->networkCode().c_str(),
					 rec->stationCode().c_str(),
					 rec->locationCode().c_str(),
					 rec->channelCode().c_str(),
					 seq24bit? 6: 16,
					 (unsigned long long)seq);

		data += packetLength;
		len -= packetLength;
	}

	_buffer = string(data, len);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::sendResponse(const char *data) {
	sendResponse(data, strlen(data));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::sendResponse(const char *data, int len) {
	send(data, len);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void SeedlinkSession::startTransfer() {
	_cursorsAvail.clear();

	if ( _stations.empty() && _wildcardStations.empty() ) {
		// uni-station mode
		string name = global.defaultNetwork + "." + global.defaultStation;

		RingPtr ring = _storage->ring(name);

		if ( !ring ) {
			ring = _storage->createRing(name,
						    global.segments,
						    global.segsize,
						    global.recsize);

			if ( !ring ) {
				SEISCOMP_ERROR("could not create ring");
				close();
				return;
			}
		}

		CursorPtr cursor = _currentStation->cursor(ring, *this);
		_cursors.insert(pair<string, CursorPtr>(name, cursor));
		_cursorsAvail.insert(cursor);
	}
	else {
		for ( auto i : _stations ) {
			if ( !checkAccess(i.first, _ipaddress, _user) ) {
				SEISCOMP_INFO("access to %s denied for %s (%s)",
					      i.first.c_str(),
					      Wired::toString(_ipaddress).c_str(),
					      _user.c_str());

				continue;
			}

			RingPtr ring = _storage->ring(i.first);

			if ( !ring ) continue;  // future station

			CursorPtr cursor = i.second->cursor(ring, *this);
			_cursors.insert(pair<string, CursorPtr>(i.first, cursor));
			_cursorsAvail.insert(cursor);
		}

		for ( auto i : _wildcardStations ) {
			for ( const auto &name : _storage->cat() ) {
				if ( _stations.find(name) != _stations.end() )
					continue;

				if ( !i.second->match(name) )
					continue;

				if ( !checkAccess(name, _ipaddress, _user) ) {
					SEISCOMP_DEBUG("access to %s denied for %s (%s)",
						       name.c_str(),
						       Wired::toString(_ipaddress).c_str(),
						       _user.c_str());

					continue;
				}

				RingPtr ring = _storage->ring(name);

				if ( !ring )
					throw logic_error("station " + name + " not found");

				CursorPtr cursor = i.second->cursor(ring, *this);
				_cursors.insert(pair<string, CursorPtr>(name, cursor));
				_cursorsAvail.insert(cursor);
			}
		}
	}

	for ( auto i : _cursors ) {
		if ( _accept.empty() && !_acceptAll ) {
			i.second->accept(FMT_MSEED24);  // only MS2.4 in legacy mode
		}
		else {
			for ( auto j : _accept )
				i.second->accept(j);
		}
	}

	_cursorIter = _cursorsAvail.begin();
	_transfer = true;

	if ( !inAvail() && !_cursorsAvail.empty() )
		collectData();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void SeedlinkSession::stopTransfer() {
	_transfer = false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void SeedlinkSession::collectData() {
	string buffer;
	buffer.reserve(6000);

	while ( !_cursorsAvail.empty() && buffer.size() < 5120 ) {
		while ( _cursorIter != _cursorsAvail.end() ) {
			RecordPtr rec = (*_cursorIter)->next();

			if ( rec ) {
				if ( _accept.empty() && !_acceptAll ) {  // legacy mode
					if ( rec->format() != FMT_MSEED24 )
						throw logic_error("unexpected format");

					char seqstr[30];
					snprintf(seqstr, 30, "%06llX", (unsigned long long)((*_cursorIter)->sequence() - 1) & 0xffffff);
					buffer.append("SL");
					buffer.append(seqstr);
					buffer.append(rec->payload());
				}
				else {
					Sequence seq = (*_cursorIter)->sequence() - 1;
					uint32_t length = rec->payloadLength() + 8;
					buffer.append("SE");
					buffer.push_back(char(rec->format()));
					buffer.push_back(char(0));
					buffer.append((char *)&length, 4); // TODO: byteorder
					buffer.append((char *)&seq, 8);    // TODO: byteorder
					buffer.append(rec->payload());
				}
			}
			else {
				if ( (*_cursorIter)->endOfData() )
					_cursors.erase((*_cursorIter)->ringName());

				_cursorsAvail.erase(_cursorIter++);
			}
		}

		_cursorIter = _cursorsAvail.begin();
	}

	if ( _cursors.empty() ) {
		buffer.append("END");
		_transfer = false;
	}

	send(buffer.data(), buffer.size());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::outboxFlushed() {
	if ( _transfer )
		collectData();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::cursorAvail(CursorPtr c, Sequence seq) {
	_cursorsAvail.insert(c);

	if ( _transfer && !inAvail() )
		collectData();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::stationAvail(const string &name) {
	StationPtr station;
	map<string, StationPtr>::iterator i = _stations.find(name);
	if ( i != _stations.end() ) {
		if ( !checkAccess(name, _ipaddress, _user) ) {
			SEISCOMP_INFO("access to %s denied for %s (%s)",
				      name.c_str(),
				      Wired::toString(_ipaddress).c_str(),
				      _user.c_str());

			return;
		}

		station = i->second;
	}
	else {
		for ( auto j : _wildcardStations ) {
			if ( j.second->match(name) ) {
				if ( !checkAccess(name, _ipaddress, _user) ) {
					SEISCOMP_DEBUG("access to %s denied for %s (%s)",
						       name.c_str(),
						       Wired::toString(_ipaddress).c_str(),
						       _user.c_str());

					continue;
				}

				station = j.second;
				break;
			}
		}
	}

	if ( station ) {
		RingPtr ring = _storage->ring(name);

		if ( !ring )
			throw logic_error("station " + name + " not found");

		CursorPtr cursor = station->cursor(ring, *this);

		if ( _accept.empty() && !_acceptAll ) {
			cursor->accept(FMT_MSEED24);  // only MS2.4 in legacy mode
		}
		else {
			for ( auto j : _accept )
				cursor->accept(j);
		}

		_cursors.insert(pair<string, CursorPtr>(name, cursor));
		_cursorsAvail.insert(cursor);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

