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

#ifndef SEISCOMP_APPS_SEEDLINK_ACCESS_H__
#define SEISCOMP_APPS_SEEDLINK_ACCESS_H__

#include <set>
#include <string>

#include <seiscomp/wired/ipacl.h>
#include <seiscomp/wired/devices/socket.h>


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


class ACL {
	public:
		ACL();
		ACL(const std::string &acl);
		bool set(const std::string &acl);
		bool add(const std::string &ip_user);
		bool check(const Wired::Socket::IPAddress &ip, const std::string &user) const;
		bool empty();
		void clear();

	private:
		Wired::IPACL _ipACL;
		std::set<std::string> _userACL;
		bool _empty;
};


}
}
}

#endif
