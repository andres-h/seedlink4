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

#include <map>

#define SEISCOMP_COMPONENT SEEDLINK
#include <seiscomp/logging/log.h>
#include <seiscomp/core/strings.h>
#include <seiscomp/utils/bindings.h>
#include <seiscomp/utils/files.h>

#include "app.h"
#include "settings.h"
#include "session.h"
#include "storage.h"
#include "format.h"
#include "mseed24.h"


using namespace std;
using namespace Seiscomp;


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Application::Application(int argc, char** argv)
: Client::Application(argc, argv) {
	setMessagingEnabled(false);
	setDatabaseEnabled(true, true);
	setLoadConfigModuleEnabled(true);
	setLoadStationsEnabled(true);
	bindSettings(&global);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Application::init() {
	// mode_t mask = umask(0);
	// umask(mask | 0111);

	if ( !Client::Application::init() )
		return false;

	SEISCOMP_INFO("Storage location: %s", global.filebase.c_str());
	SEISCOMP_INFO("Default number of segments per station: %d", global.segments);
	SEISCOMP_INFO("Default segment size in records: %d", global.segsize);
	SEISCOMP_INFO("Default maximum record size: %d", global.recsize);

	_server.setTriggerMode(Wired::DeviceGroup::LevelTriggered);

	try {
		StoragePtr storage = new Storage(global.filebase);

		ACL trusted(global.trusted);
		ACL defaultAccess(global.access);
		map<string, ACL> access;

		Util::BindingsPtr bindings = new Util::Bindings();
		bindings->init(configModule(), "seedlink", false);
		for ( auto i = bindings->begin(); i != bindings->end(); ++i ) {
			const Util::KeyValues* keys = i.keys();

			int segments = global.segments;
			if ( !keys->getInt(segments, "segments") )
				SEISCOMP_INFO("%s %s using default segments = %d",
						i.networkCode().c_str(),
						i.stationCode().c_str(),
						segments);

			int segsize = global.segsize;
			if ( !keys->getInt(segsize, "segsize") )
				SEISCOMP_INFO("%s %s using default segsize = %d",
						i.networkCode().c_str(),
						i.stationCode().c_str(),
						segsize);

			int recsize = global.recsize;
			if ( !keys->getInt(recsize, "recsize") )
				SEISCOMP_INFO("%s %s using default recsize = %d",
						i.networkCode().c_str(),
						i.stationCode().c_str(),
						recsize);

			RingPtr ring = storage->ring(i.networkCode() + "." + i.stationCode());

			if ( ring )
				ring->ensure(segments * segsize, recsize);
			else
				ring = storage->createRing(i.networkCode() + "." + i.stationCode(),
							   segments *
							   segsize,
							   recsize);

			bool ordered = false;
			keys->getBool(ordered, "ordered");
			ring->setOrdered(ordered);

			string accessStr = global.access;
			if ( !keys->getString(accessStr, "access") )
				SEISCOMP_INFO("%s %s using default access = %s",
						i.networkCode().c_str(),
						i.stationCode().c_str(),
						accessStr.c_str());

			if ( accessStr.length() > 0 )
				access.insert(pair<string, ACL>(i.networkCode() + "." + i.stationCode(), accessStr));
		}

		map<string, string> descriptions;
		DataModel::Inventory* inv = Client::Inventory::Instance()->inventory();
		for ( unsigned int i = 0; i < inv->networkCount(); ++i ) {
			DataModel::Network* net = inv->network(i);
			for ( unsigned int j = 0; j < net->stationCount(); ++j ) {
				DataModel::Station* sta = net->station(j);
				descriptions.insert(pair<string, string>(net->code() + "." + sta->code(),
									 sta->description()));
			}
		}

		SeedlinkListener* listener = new SeedlinkListener(storage,
								  trusted,
								  defaultAccess,
								  access,
								  descriptions);

		_server.addEndpoint(Wired::Socket::IPAddress(), global.port, false, listener);

		if ( global.sslport > 0 ) {
			if ( global.certificate.length() == 0 || !Util::fileExists(global.certificate) ) {
				SEISCOMP_ERROR("missing SSL certificate");
				return false;
			}

			if ( global.privateKey.length() == 0 || !Util::fileExists(global.privateKey) ) {
				SEISCOMP_ERROR("missing SSL private key");
				return false;
			}

			_server.setCertificate(global.certificate);
			_server.setPrivateKey(global.privateKey);
		        _server.addEndpoint(Wired::Socket::IPAddress(), global.sslport, true, listener);
		}
	}
	catch (const exception &e) {
               SEISCOMP_ERROR("%s", e.what());
               return false;
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Application::run() {
	SEISCOMP_INFO("Starting SeedLink server on port %d", global.port);

	if ( !_server.init() )
		return false;

	return _server.run();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Application::done() {
	Client::Application::done();

	SEISCOMP_INFO("Shutdown server");
	_server.shutdown();
	_server.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Application::exit(int returnCode) {
	Client::Application::exit(returnCode);
	_server.shutdown();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


}
}
}
