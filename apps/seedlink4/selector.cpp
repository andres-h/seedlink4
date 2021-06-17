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
bool Selector::initPattern(regex &r, const string &src, bool ext) {
	if ( src.length() == 0 ) {
		r = regex(".*");
		return true;
	}

	if (ext) {
		if ( !regex_match(src, regex("[A-Z0-9\\-\\?\\*]*")) )
			return false;
	}
	else {
		if ( !regex_match(src, regex("[A-Z0-9\\-\\?]*")) )
			return false;
	}

	string s = regex_replace(src, regex("-"), " ");
	s = regex_replace(s, regex("\\?"), ".");

	if ( ext )
		s = regex_replace(s, regex("\\*"), ".*");

	r = regex(s);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::init(const string &selstr, bool ext) {
	string loc = "";
	string cha = "";
	string type = "";
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

	if ( ext ) {
		if ( p == string::npos )
			return false;

		loc = s.substr(0, p);
		s = s.substr(p + 1);

		p = s.find_first_of('.');

		if ( p != string::npos ) {
			cha = s.substr(0, p);
			type = s.substr(p + 1);
		}
		else {
			cha = s;
		}
	}
	else {
		// legacy format

		if ( p != string::npos ) {
			type = s.substr(p + 1);
			s = s.substr(0, p);
		}

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

	return (initPattern(_rloc, loc, ext) &&
		initPattern(_rcha, cha, ext) &&
		initPattern(_rtype, type, ext) );
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::match(RecordPtr rec) {
	return (regex_match(rec->location(), _rloc) &&
		regex_match(rec->channel(), _rcha) &&
		regex_match(string(1, rec->type()), _rtype));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

