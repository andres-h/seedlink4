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
	if ( src.length() == 0 ) {
		r = regex(".*");
		return true;
	}

	if (slproto >= 4.0) {
		if ( !regex_match(src, regex("[A-Z0-9\\-\\?\\*]*")) )
			return false;
	}
	else {
		if ( !regex_match(src, regex("[A-Z0-9\\-\\?]*")) )
			return false;
	}

	string s = regex_replace(src, regex("-"), " ");
	s = regex_replace(s, regex("\\?"), ".");

	if ( slproto >= 4.0 )
		s = regex_replace(s, regex("\\*"), ".*");

	r = regex(s);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::init(const string &selstr, double slproto) {
	string loc = "";
	string cha = "";
	string fmt = "";
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

	if ( slproto >= 4.0 ) {
		if ( p == string::npos )
			return false;

		loc = s.substr(0, p);
		s = s.substr(p + 1);
		p = s.find_first_of('.');

		if ( p != string::npos ) {
			cha = s.substr(0, p);
			fmt = s.substr(p + 1);
		}
		else {
			cha = s;
		}
	}
	else {
		char type = 0;

		if ( p != string::npos ) {
			if ( s.length() != p + 2 )
				return false;

			type = s[p + 1];
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
			type = s[0];
		}
		else if ( s.length() != 0 ) {
			return false;
		}

		switch ( type ) {
			case 'D':
				fmt = string(1, FMT_MSEED24);
				break;

			case 'E':
				fmt = string(1, FMT_MSEED24_EVENT);
				break;

			case 'C':
				fmt = string(1, FMT_MSEED24_CALIBRATION);
				break;

			case 'T':
				fmt = string(1, FMT_MSEED24_TIMING);
				break;

			case 'O':
				fmt = string(1, FMT_MSEED24_OPAQUE);
				break;

			case 'L':
				fmt = string(1, FMT_MSEED24_LOG);
				break;

			case 0:
				break;

			default:
				return false;
		}
	}

	return (initPattern(_rloc, loc, slproto) &&
		initPattern(_rcha, cha, slproto) &&
		initPattern(_rfmt, fmt, slproto));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Selector::match(RecordPtr rec) {
	return (regex_match(rec->location(), _rloc) &&
		regex_match(rec->channel(), _rcha) &&
		regex_match(string(1, rec->format()), _rfmt));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
}

