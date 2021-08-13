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
bool Selector::initPattern(regex &r, const string &src, double slproto) {
	if (slproto >= 4.0) {
		if ( !regex_match(src, regex("[A-Z0-9_\\?\\*]*")) )
			return false;
	}
	else {
		if ( !regex_match(src, regex("[A-Z0-9\\-\\?]*")) )
			return false;
	}

	string s = regex_replace(src, regex("\\?"), ".");

	if ( slproto >= 4.0 )
		s = regex_replace(s, regex("\\*"), ".*");
	else
		s = regex_replace(s, regex("-"), " ");

	r = regex(s);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::init(const string &selstr, double slproto) {
	string stream = "*";
	string fmt = "*";
	string s;

	if (selstr[0] == '!') {
		_neg = true;
		s = selstr.substr(1);
	}
	else {
		_neg = false;
		s = selstr;
	}

	size_t p = s.find('.');

	if ( slproto >= 4.0 ) {
		if ( p != string::npos ) {
			stream = s.substr(0, p);
			fmt = s.substr(p + 1) + "*";
		}
		else {
			stream = s;
		}
	}
	else {
		string loc = "??";
		string cha = "???";

		if ( p != string::npos ) {
			s = s.substr(0, p);
			fmt = "2" + s.substr(p + 1);

			if ( fmt.length() != 2 )
				return false;
		}

		if ( s.length() == 5 ) {
			loc = s.substr(0, 2);
			cha = s.substr(2);
		}
		else if ( s.length() == 3 ) {
			cha = s;
		}
		else if ( s.length() == 1 && selstr.length() == 1) {
			fmt = "2" + s;
		}
		else if ( s.length() != 0 ) {
			return false;
		}

		if ( cha.length() != 3 )
			throw logic_error("invalid channel length");

		stream = loc + "_" + cha[0] + "_" + cha[1] + "_" + cha[2];
	}

	return (initPattern(_rstream, stream, slproto) &&
		initPattern(_rfmt, fmt, slproto));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::match(RecordPtr rec) {
	return (regex_match(rec->stream(), _rstream) &&
		regex_match(rec->format(), _rfmt));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

