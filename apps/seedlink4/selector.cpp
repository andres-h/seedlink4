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

#include "storage.h"


using namespace std;
using namespace Seiscomp;


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::initPattern(regex &r, const string &src, bool simple) {
	if ( src.length() == 0 ) {
		r = regex(".*");
		return true;
	}

	if (simple) {
		if ( !regex_match(src, regex("[A-Z0-9\\-\\?]*")) )
			return false;
	}
	else {
		if ( !regex_match(src, regex("[A-Z0-9\\-\\?\\*]*")) )
			return false;
	}

	string s = regex_replace(src, regex("-"), " ");
	s = regex_replace(s, regex("\\?"), ".");

	if ( !simple )
		s = regex_replace(s, regex("\\*"), ".*");

	r = regex(s);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::init(const string &selstr) {
	string loc = "";
	string cha = "";
	string type = "";
	bool simple = false;
	string s;

	if (selstr[0] == '!') {
		_neg = true;
		s = selstr.substr(1);
	}
	else {
		_neg = false;
		s = selstr;
	}

	size_t p = s.find_first_of('.');

	if ( p != string::npos ) {
		type = s.substr(p + 1);
		s = s.substr(0, p);
	}

	p = s.find_first_of(':');

	if ( p != string::npos ) {
		loc = s.substr(0, p);
		cha = s.substr(p + 1);
	}
	else {
		// legacy format
		simple = true;

		if ( s.length() == 5 ) {
			loc = s.substr(0, 2);
			cha = s.substr(2);
		}
		else if ( s.length() == 3 ) {
			cha = s;
		}
		else if ( s.length() == 1 && selstr.length() == 1) {
			type = s;
		}
		else if ( s.length() != 0 ) {
			return false;
		}
	}

	return (initPattern(_rloc, loc, simple) &&
		initPattern(_rcha, cha, simple) &&
		initPattern(_rtype, type, simple) );
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::match(RecordPtr rec) {
	return (regex_match(rec->locationCode(), _rloc) &&
		regex_match(rec->channelCode(), _rcha) &&
		regex_match(rec->typeCode(), _rtype));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

