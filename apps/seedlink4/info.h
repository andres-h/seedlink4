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
#include <regex>

#include <seiscomp/core/baseobject.h>
#include <seiscomp/core/io.h>

#include "storage.h"
#include "format.h"


namespace Seiscomp {
namespace Applications {
namespace Seedlink {


enum InfoLevel {
	INFO_ID,
	INFO_FORMATS,
	INFO_STATIONS,
	INFO_STREAMS,
	INFO_CONNECTIONS
};

DEFINE_SMARTPOINTER(StreamInfo);
class StreamInfo : public Core::BaseObject {
	public:
		StreamInfo(double slproto, StreamPtr stream);

		void serialize(Core::Archive &ar);

	private:
		double _slproto;
		StreamPtr _stream;
};


DEFINE_SMARTPOINTER(RingInfo);
class RingInfo : public Core::BaseObject {
	public:
		RingInfo(double slproto, RingPtr ring, const std::string &description,
			 const std::regex &streamRegex, InfoLevel level);

		void serialize(Core::Archive &ar);

	private:
		double _slproto;
		RingPtr _ring;
		std::string _description;
		std::regex _streamRegex;
		InfoLevel _level;
};


DEFINE_SMARTPOINTER(SubformatInfo);
class SubformatInfo : public Core::BaseObject {
	public:
		SubformatInfo(double slproto, const std::map<char, Format*> &subformats);

		void serialize(Core::Archive &ar);

	private:
		double _slproto;
		std::map<char, Format*> _subformats;
};


DEFINE_SMARTPOINTER(FormatInfo);
class FormatInfo : public Core::BaseObject {
	public:
		FormatInfo(double slproto, const std::map<char, Format*> &subformats);

		void serialize(Core::Archive &ar);

	private:
		double _slproto;
		std::map<char, Format*> _subformats;
};


DEFINE_SMARTPOINTER(FormatsInfo);
class FormatsInfo : public Core::BaseObject {
	public:
		FormatsInfo(double slproto);

		void serialize(Core::Archive &ar);

	private:
		double _slproto;
};


DEFINE_SMARTPOINTER(Info);
class Info : public Core::BaseObject {
	public:
		Info(double slproto, const std::string &software, const std::string &organization,
		     const Core::Time &started, InfoLevel level);

		void addStation(RingPtr ring, const std::string &description, const std::regex &streamRegex);
		void serialize(Core::Archive &ar);

	private:
		double _slproto;
		std::string _software;
		std::string _organization;
		Core::Time _started;
		InfoLevel _level;
		std::list<RingInfoPtr> _stations;
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


class InfoError : public Info {
	public:
		InfoError(double slproto, const std::string &software, const std::string &organization,
			  const Core::Time &started, const std::string &code, const std::string &message);

		void serialize(Core::Archive &ar);

	private:
		double _slproto;
		Error _error;
};

}
}
}

#endif
