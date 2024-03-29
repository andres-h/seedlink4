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
	friend class FormatsInfo;
	friend class FormatInfo;
	friend class SubformatInfo;

	public:
		Format(const std::string &code, const std::string &mimetype, const std::string &description);

		virtual ssize_t readRecord(const void *buf, size_t len, RecordPtr &rec) =0;

		static Format* get(const std::string &code);

	private:
		std::string _code;
		std::string _mimetype;
		std::string _description;

		static std::map<char, std::map<char, Format*> > _instances;

	protected:
		std::string formatCode() { return _code; }
};

}
}
}

#endif
