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

#ifndef SEISCOMP_APPS_SEEDLINK_SELECTOR_H__
#define SEISCOMP_APPS_SEEDLINK_SELECTOR_H__

#include <string>
#include <regex>

#include <seiscomp/core/baseobject.h>

#include "record.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


DEFINE_SMARTPOINTER(Selector);
class Selector : public Core::BaseObject {
	public:
		bool init(const std::string &selstr);
		bool match(RecordPtr rec);
		bool negative() { return _neg; }

	private:
		bool _neg;
		std::regex _rloc;
		std::regex _rcha;
		std::regex _rtype;

		bool initPattern(std::regex &r, const std::string &s, bool simple = false);
};

}
}
}


#endif
