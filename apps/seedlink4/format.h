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

#ifndef SEISCOMP_APPS_SEEDLINK_FORMAT_H__
#define SEISCOMP_APPS_SEEDLINK_FORMAT_H__

#include <unistd.h>
#include <seiscomp/core/baseobject.h>

#include "record.h"

namespace Seiscomp {
namespace Applications {
namespace Seedlink {


class Format : public Core::BaseObject {
	friend class FormatInfo;

	public:
		Format(FormatCode code, const std::string &mimetype);

		virtual ssize_t readRecord(const void *buf, size_t len, RecordPtr &rec) =0;

		static Format* get(FormatCode code);

	private:
		FormatCode _code;
		std::string _mimetype;

		static std::map<FormatCode, Format*> _instances;

	protected:
		FormatCode formatCode() { return _code; }
};

}
}
}

#endif
