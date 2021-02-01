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
		     TODO("NSWILDCARD ") \
		     TODO("BATCH ") \
		     "ASYNC " \
		     "AUTH:USERPASS " \
		     TODO("AUTH:TOKEN ") \
		     "MULTISTATION " \
		     "TIME " \
		     TODO("INFO ")



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
		map<string, CursorPtr> _stations;
		set<CursorPtr> _cursorsAvail;
		set<CursorPtr>::iterator _cursorIter;
		CursorPtr _cursor;
		string _buffer;
		bool _wildcard;
		bool _transfer;

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
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SeedlinkListener::SeedlinkListener(const Wired::IPACL &allowedIPs,
                                 const Wired::IPACL &deniedIPs,
				 StoragePtr storage,
				 const map<FormatCode, FormatPtr> &formats,
				 const ACL &trusted,
				 const ACL &defaultAccess,
				 const map<string, ACL> &access,
                                 Wired::Socket *socket)
: Wired::AccessControlledEndpoint(socket, allowedIPs, deniedIPs)
, _storage(storage), _formats(formats), _trusted(trusted)
, _defaultAccess(defaultAccess), _access(access) {
	DataModel::Inventory* inv = Client::Inventory::Instance()->inventory();
	for ( unsigned int i = 0; i < inv->networkCount(); ++i ) {
		DataModel::Network* net = inv->network(i);
		for ( unsigned int j = 0; j < net->stationCount(); ++j ) {
			DataModel::Station* sta = net->station(j);
			_descriptions.insert(pair<string, string>(
						net->code() + "." + sta->code(),
						sta->description()));
		}
	}
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
, _wildcard(false), _transfer(false) {
	_ipaddress = sock->address();
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
	     !i->second.check(_ipaddress, _user) )
		return true;

	if ( _defaultAccess.check(_ipaddress, _user) )
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
		if ( _trusted.check(_ipaddress, _user) && _type == Unspecific ) {
			sendResponse("OK\r\n");
			_type = Feed;
		}
		else {
			sendResponse("ERROR\r\n");
		}

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

		sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 15 && strncasecmp(tok, "GETCAPABILITIES", tokLen) == 0 ) {
		sendResponse(CAPABILITIES "\r\n");
		return;
	}

	if ( tokLen == 7 && strncasecmp(tok, "STATION", tokLen) == 0 ) {
		_type = Client;

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			sendResponse("ERROR\r\n");
			return;
		}

		string name(tok, tokLen);

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) != NULL )
			name = string(tok, tokLen) + "." + name;
		else
			name = string(/* TODO: default net */) + "." + name;

		if ( !checkAccess(name, _ipaddress, _user) ) {
			SEISCOMP_INFO("access to %s denied for %s (%s)",
				      name.c_str(),
				      Wired::toString(_ipaddress).c_str(),
				      _user.c_str());
			sendResponse("ERROR\r\n");
			return;
		}

		// TODO: wildcards
		_wildcard = false;

		CursorPtr cursor;
		map<string, CursorPtr>::iterator i;
		if ( (i = _stations.find(name)) == _stations.end() ) {
			RingPtr ring = _storage->ring(name);
			if ( !ring ) {
				sendResponse("ERROR\r\n");
				return;
			}

			i = _stations.insert(pair<string,CursorPtr>(name, ring->cursor(*this))).first;
		}

		_cursor = i->second;
		sendResponse("OK\r\n");
		return;
	}

	if ( tokLen == 6 && strncasecmp(tok, "SELECT", tokLen) == 0 ) {
		_type = Client;

		if ( !_cursor) {
			// TODO: uni-station
			sendResponse("ERROR\r\n");
			return;
		}

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) != NULL ) {
			if ( !_cursor->select(string(tok, tokLen)) ) {
				sendResponse("ERROR\r\n");
				return;
			}
		}
		else {
			_cursor->select("");
		}

		sendResponse("OK\r\n");
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

		if ( !_cursor) {
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
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SeedlinkSession::handleDFT(const char *data, size_t len, int dft) {
	if ( !_cursor) {
		// TODO: uni-station
		sendResponse("ERROR\r\n");
		return;
	}

	_cursor->setSequence(UNSET);
	_cursor->setStart(Core::Time::Null);
	_cursor->setEnd(Core::Time::Null);
	_cursor->setDialup(dft == 2);

	const char *tok;
	size_t tokLen;

	if ( dft == 1 || dft == 2 ) {  // "DATA" || "FETCH"
		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			sendResponse("OK\r\n");
	 		return;
		}

		if ( tokLen != 2 || strncasecmp(tok, "-1", tokLen) != 0 ) {
			if ( _wildcard ) {
				// cannot use sequence number with wildcard
				sendResponse("ERROR\r\n");
				return;
			}

			try {
				size_t end;
				unsigned long long seq = stoull(string(tok, tokLen), &end, 16);

				if ( end == tokLen && seq < UNSET) {
					bool seq24bit = (tokLen <= 6);
					_cursor->setSequence(seq, seq24bit);
				}
				else {
					sendResponse("ERROR\r\n");
					return;
				}
			}
			catch(const invalid_argument&) {
				sendResponse("ERROR\r\n");
				return;
			}
		}

		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			sendResponse("OK\r\n");
			return;
		}
	}
	else {
		if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
			sendResponse("ERROR\r\n");
			return;
		}

		_cursor->setSequence(0);
	}

	Core::Time starttime = parseTime(tok, tokLen);
	if ( !starttime.valid() ) {
		sendResponse("ERROR\r\n");
		return;
	}

	_cursor->setStart(starttime);

	if ( (tok = Core::tokenize(data, " ", len, tokLen)) == NULL ) {
		sendResponse("OK\r\n");
		return;
	}

	Core::Time endtime = parseTime(tok, tokLen);
	if ( !endtime.valid() ) {
		sendResponse("ERROR\r\n");
		return;
	}

	_cursor->setEnd(endtime);

	sendResponse("OK\r\n");
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

			// TODO: check if the new station matches any standing wildcard requests
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
	map<string, CursorPtr>::iterator i;
	for ( i = _stations.begin(); i != _stations.end(); ++i ) {
		i->second->accept(0);  // reset accept list

		if ( _accept.empty() && !_acceptAll ) {
			i->second->accept(FMT_MSEED24);  // only MS2.4 in legacy mode
		}
		else {
			list<FormatCode>::iterator j;
			for ( j = _accept.begin(); j != _accept.end(); ++j ) {
				i->second->accept(*j);
			}
		}

		_cursorsAvail.insert(i->second);
	}

	_cursorIter = _cursorsAvail.begin();
	_transfer = true;

	if ( !inAvail() )
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

	while ( _cursorsAvail.size() > 0 && buffer.size() < 5120 ) {
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
					_stations.erase((*_cursorIter)->ringName());

				_cursorsAvail.erase(_cursorIter++);
			}
		}

		_cursorIter = _cursorsAvail.begin();
	}

	if ( _stations.empty() )
		buffer.append("END");

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

}
}
}

