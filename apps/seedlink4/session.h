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
#include "acl.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {

class SeedlinkListener : public Wired::Endpoint {
	public:
		SeedlinkListener(StoragePtr storage,
				 const ACL &trusted,
				 const ACL &defaultAccess,
				 const std::map<std::string, ACL> &access,
				 const std::map<std::string, std::string> &descriptions,
				 Wired::Socket *socket = NULL);

	private:
		StoragePtr _storage;
		ACL _trusted;
		ACL _defaultAccess;
		std::map<std::string, ACL> _access;
		std::map<std::string, std::string> _descriptions;

		Wired::Session *createSession(Wired::Socket *socket) override;
};

}
}
}


#endif
