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

#include <seiscomp/client/application.h>
#include <seiscomp/wired/ipacl.h>

#include "server.h"
#include "session.h"
#include "version.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


class Application : public Seiscomp::Client::Application {
	public:
		Application(int argc, char** argv);


	protected:
		bool init();
		bool run();
		void done();
		void exit(int returnCode);

		const char *version() {
			return SEEDLINK4_VERSION_NAME;
		}

	private:
		Server _server;
};


}
}
}
