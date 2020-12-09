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

#ifndef SEISCOMP_APPS_SEEDLINK_SESSION_H__
#define SEISCOMP_APPS_SEEDLINK_SESSION_H__

#include <seiscomp/wired/clientsession.h>
#include <seiscomp/wired/endpoint.h>
#include <seiscomp/wired/ipacl.h>

#include <map>
#include <list>
#include <vector>
#include <set>
#include <cstdint>

#include "storage.h"
#include "format.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {

class SeedlinkListener : public Wired::AccessControlledEndpoint {
	public:
		SeedlinkListener(const Wired::IPACL &allowedIPs,
		               const Wired::IPACL &deniedIPs,
			       StoragePtr storage,
                               const std::map<FormatCode, FormatPtr> &formats,
		               Wired::Socket *socket = NULL);

	private:
		StoragePtr _storage;
		std::map<std::string, std::string> _descriptions;
		std::map<FormatCode, FormatPtr> _formats;

		Wired::Session *createSession(Wired::Socket *socket) override;
};

}
}
}


#endif
