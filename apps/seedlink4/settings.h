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

#ifndef SEISCOMP_APPS_SEEDLINK_SETTINGS_H__
#define SEISCOMP_APPS_SEEDLINK_SETTINGS_H__


#include <seiscomp/system/application.h>
#include <seiscomp/system/environment.h>


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// Define default configuration
struct Settings : System::Application::AbstractSettings {
	Settings(): port(18000), sslport(0), segments(10), segsize(1000), recsize(10240) {
		filebase = Environment::Instance()->installDir() + "/var/lib/seedlink4";
		organization = "Unconfigured";
		trusted = "127.0.0.1/8";
	}

	int port;
	int sslport;
	int segments;
	int segsize;
	int recsize;
	std::string certificate;
	std::string privateKey;
	std::string filebase;
	std::string organization;
	std::string trusted;
	std::string access;

	virtual void accept(System::Application::SettingsLinker &linker) {
		linker
		& cfg(port, "port")
		& cli(port, "Server", "port",
		      "TCP port to for data requests with SeedLink protocol",
		      true)
		& cfg(port, "sslport")
		& cli(port, "Server", "sslport",
		      "TCP port to for data requests with SeedLink protocol using SSL",
		      true)
		& cfg(certificate, "certificate")
		& cli(certificate, "Server", "certificate",
		      "Location of SSL certificate file")
		& cfg(privateKey, "privateKey")
		& cli(privateKey, "Server", "privateKey",
		      "Location of SSL private key file")
		& cfgAsPath(filebase, "filebase")
		& cliAsPath(filebase, "Server", "filebase",
			    "Location of storage",
			    true)
		& cfg(segments, "segments")
		& cli(segments, "Server", "segments",
		      "Number of segments per station",
		      true)
		& cfg(segsize, "segsize")
		& cli(segsize, "Server", "segsize",
		      "Segment size in records",
		      true)
		& cfg(recsize, "recsize")
		& cli(recsize, "Server", "recsize",
		      "Maximum record size, including metadata header",
		      true)
		& cfg(organization, "organization")
		& cfg(trusted, "trusted")
		& cfg(access, "access");

	}
};


extern Settings global;


}
}
}


#endif
