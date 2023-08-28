#pragma once

#include <iostream>
#include <fstream>

inline char directorySeparator()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

class ifstreamoffset : public std::ifstream
{
	//Pretty much identical to ifstream for the most part
	//However it includes an offset (named superOffset) that can be used to reposition the "start" of the stream
	//E.g. If you have a filestream, but everything before 0x1000 is unneeded, you can make 0x1000 the offset
	//This would allow you to make arbitrary movements from the beginning of the stream without needing to constantly reuse the offset

public:
	using std::ifstream::ifstream;

	unsigned int superOffset;

	basic_istream& __CLR_OR_THIS_CALL seekg(off_type _Off, ios_base::seekdir _Way, bool offsetUsed = true)
	{
		unsigned int offset = 0;
		if (offsetUsed && _Way == ios_base::beg) { offset = superOffset; }

		ios_base::iostate _State = ios_base::goodbit;
		ios_base::iostate _Oldstate = _Myios::rdstate();
		_Myios::clear(_Oldstate & ~ios_base::eofbit);
		const sentry _Ok(*this, true);

		if (!this->fail())
		{
			_TRY_IO_BEGIN
				if (static_cast<off_type>(_Myios::rdbuf()->pubseekoff(_Off + offset, _Way, ios_base::in)) == -1)
				{
					_State |= ios_base::failbit;
				}
			_CATCH_IO_END
		}

		_Myios::setstate(_State);
		return *this;
	}

	pos_type __CLR_OR_THIS_CALL tellg(bool offsetUsed = true)
	{
		unsigned int offset = 0;
		if (offsetUsed) { offset = superOffset; }

		const sentry _Ok(*this, true);

		if (!this->fail()) {
			_TRY_IO_BEGIN
				return (int)_Myios::rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in) - offset;
			_CATCH_IO_END
		}

		return pos_type(-1);
	}
};