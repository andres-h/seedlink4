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

#include "format.h"


using namespace std;
using namespace Seiscomp;


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


std::map<char, std::map<char, Format*> > Format::_instances;


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Format::Format(const string &code, const string &mimetype, const string &description)
: _code(code), _mimetype(mimetype), _description(description) {
	if ( code.length() != 2 )
		throw logic_error("invalid format code");

	std::map<char, std::map<char, Format*> >::iterator i = _instances.find(code[0]);
	if( i == _instances.end() )
		i = _instances.insert(pair<char, std::map<char, Format*> >(code[0], std::map<char, Format*>())).first;

	i->second.insert(pair<char, Format*>(code[1], this));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Format* Format::get(const string &code) {
	if ( code.length() != 2 )
		throw logic_error("invalid format code");

	auto i = _instances.find(code[0]);
	if ( i == _instances.end() )
		return NULL;

	auto j = i->second.find(code[1]);
	if ( j == i->second.end() )
		return NULL;

	return j->second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

