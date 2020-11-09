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
	setDatabaseEnabled(false, false);
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
	SEISCOMP_INFO("Number of segments per station: %d", global.segments);
	SEISCOMP_INFO("Segment size in records: %d", global.segsize);
	SEISCOMP_INFO("Maximum record size: %d", global.recsize);

	_server.setTriggerMode(Wired::DeviceGroup::LevelTriggered);

	Wired::IPACL globalClientAllow, globalClientDeny;

	try {
		StoragePtr storage = new Storage(global.filebase);
		map<FormatCode, FormatPtr> formats;
		formats.insert(pair<FormatCode, FormatPtr>(FMT_MSEED24, new Mseed24Format()));

		_server.addEndpoint(Wired::Socket::IPAddress(), global.port, false,
				    new SeedlinkListener(globalClientAllow, globalClientDeny,
							 storage, formats));
	}
	catch (const exception &e) {
		SEISCOMP_ERROR(e.what());
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
