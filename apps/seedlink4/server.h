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

#ifndef SEISCOMP_APPS_SEEDLINK_SERVER_H__
#define SEISCOMP_APPS_SEEDLINK_SERVER_H__


#include <seiscomp/wired/server.h>


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


class Server : public Seiscomp::Wired::Server {
	public:
		Server();
		~Server();

	protected:
		virtual void sessionRemoved(Seiscomp::Wired::Session *session);
};


}
}
}

#endif
