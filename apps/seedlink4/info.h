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

#ifndef SEISCOMP_APPS_SEEDLINK_INFO_H__
#define SEISCOMP_APPS_SEEDLINK_INFO_H__

#include <string>
#include <vector>

#include <seiscomp/core/baseobject.h>
#include <seiscomp/core/io.h>

#include "storage.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


enum InfoLevel {
	INFO_ID,
	INFO_DATATYPES,
	INFO_STATIONS,
	INFO_STREAMS,
	INFO_CONNECTIONS
};


template<int Version>
class StreamInfo : public Core::BaseObject {
	public:
		StreamInfo() {}
		StreamInfo(StreamPtr stream);
		void serialize(Core::Archive &ar);

	private:
		StreamPtr _stream;
};


template<int Version>
class RingInfo : public Core::BaseObject {
	public:
		RingInfo() {}
		RingInfo(RingPtr ring, const std::string &description, InfoLevel level);
		void serialize(Core::Archive &ar);

	private:
		RingPtr _ring;
		std::string _description;
		InfoLevel _level;
};


template<int Version>
class Info : public Core::BaseObject {
	public:
		Info() {}
		Info(const std::string &software, const std::string &organization, const Core::Time &started, InfoLevel level);
		void addStation(RingPtr ring, const std::string &description);
		void serialize(Core::Archive &ar);

	private:
		std::string _software;
		std::string _organization;
		Core::Time _started;
		InfoLevel _level;
		std::list<RingInfo<Version> > _ringInfo;
};


class Error : public Core::BaseObject {
	public:
		Error() {}
		Error(const std::string &code, const std::string &message);
		void serialize(Core::Archive &ar);

	private:
		std::string _code;
		std::string _message;
};


class InfoError : public Core::BaseObject {
	public:
		InfoError() {}
		InfoError(const std::string &code, const std::string &message);
		void serialize(Core::Archive &ar);

	private:
		Error _error;
};

}
}
}

#endif
