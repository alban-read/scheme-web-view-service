
#include <scheme/scheme.h>
#include <string>

#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)

namespace Assoc
{

	int utf8_string_length(const char* s)
	{
		auto i = 0, j = 0;
		while (s[i])
		{
			if ((s[i] & 0xc0) != 0x80)
				j++;
			i++;
		}
		return j;
	}

	static const unsigned char total_bytes[256] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
	};

	static const char trailing_bytes_for_utf8[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
	};


	ptr constUTF8toSstring(const char* s)
	{
		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		ptr ss = Sstring_of_length("", ll);

		auto i = 0;

		while (byte != 0 && i < ll)
		{
			// ascii
			if (byte < 0x80)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll)
			{
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}

		return ss;
	}

	ptr constUTF8toSstring(std::string s)
	{
		return constUTF8toSstring(s.c_str());
	}


	const char* Sstring_to_charptr(ptr sparam)
	{
		if (sparam == Snil || !Sstringp(sparam))
		{
			return _strdup("");
		}

		auto bytes = CALL1("string->utf8", sparam);
		const auto len = Sbytevector_length(bytes);
		const unsigned char* data = Sbytevector_data(bytes);
		const std::string text((char*)data, len);
		bytes = Snil;
		return _strdup(text.c_str());
	}



	ptr sstring(const char* symbol, const char* value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), constUTF8toSstring(value));
		return a;
	}

	ptr sflonum(const char* symbol, const float value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sflonum(value));
		return a;
	}

	ptr sfixnum(const char* symbol, const int value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
		return a;
	}

	ptr sptr(const char* symbol, ptr value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), value);
		return a;
	}

	ptr cons_sstring(const char* symbol, const char* value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), constUTF8toSstring(value));
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sbool(const char* symbol, bool value, ptr l)
	{
		ptr a = Snil;
		if (value) {
			a = CALL2("cons", Sstring_to_symbol(symbol), Strue);
		}
		else
		{
			a = CALL2("cons", Sstring_to_symbol(symbol), Sfalse);
		}
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sptr(const char* symbol, ptr value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), value);
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sfixnum(const char* symbol, const int value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sflonum(const char* symbol, const float value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sflonum(value));
		l = CALL2("cons", a, l);
		return l;
	}
} // namespace Assoc



