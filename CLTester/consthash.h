#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

namespace {

	template< typename resulttype, typename constanttype, size_t cur = 0, size_t maxloop = sizeof(resulttype) / sizeof(constanttype) >
		struct Replicator {
		static constexpr const resulttype next(const constanttype c, resulttype r = 0) {
			return Replicator< resulttype, constanttype, cur + 1 >::next(c, (r << ((resulttype)cur) * 8) | ((resulttype)c));
		}
	};

	template< typename resulttype, typename constanttype, size_t maxloop >
	struct Replicator< resulttype, constanttype, maxloop, maxloop > {
		static constexpr const resulttype next(const constanttype c, resulttype r) {
			return r;
		}
	};

	template< typename hashtype >
	constexpr const hashtype HashFunc(const uint8_t current, const hashtype last) {
		return (current != 0) ? (((last * Replicator< hashtype, uint32_t >::next(0x01000193L)) & hashtype(~1)) ^ ((hashtype)current)) : last;
	}

	template< typename hashtype, size_t index >
	struct HashPipe {
		static constexpr const hashtype next(const char *_str, const hashtype last) {
			return HashPipe< hashtype, index - 1 >::next(_str, HashFunc((const uint8_t)_str[index - 1], last));
		}
	};

	template< typename hashtype >
	struct HashPipe< hashtype, 0 > {
		static constexpr const hashtype next(const char *_str, const hashtype last) { return last; }
	};

	template< typename hashtype, typename type, int size >
	struct HashSwitch {
		static constexpr const hashtype next(const type _int, const hashtype _last = 1) {
			return HashFunc((const uint8_t)_int, HashSwitch< hashtype, type, size - 1>::next((_int >> 8), _last));
		}
	};

	template< typename hashtype, typename type >
	struct HashSwitch<hashtype, type, 0> {
		static constexpr const hashtype next(const type _int, const hashtype _last = 1) {
			return _last;
		}
	};

	template< typename hashtype, typename type >
	struct HashSwitch<hashtype, type, 1> {
		static constexpr const hashtype next(const type _int, const hashtype _last = 1) {
			return HashFunc((const uint8_t)_int, _last);
		}
	};

	template< typename hashtype, typename type >
	struct HashSwitch<hashtype, type, 2> {
		static constexpr const hashtype next(const type _int, const hashtype _last = 1) {
			return HashFunc((const uint8_t)_int, HashFunc((const uint8_t)(_int >> 8), _last));
		}
	};

	template< typename hashtype, typename type >
	struct HashSwitch<hashtype, type, 4> {
		static constexpr const hashtype next(const type _int, const hashtype _last = 1) {
			return HashFunc((const uint8_t)_int,
				HashFunc((const uint8_t)(_int >> 8),
					HashFunc((const uint8_t)(_int >> 16),
						HashFunc((const uint8_t)(_int >> 24),
							_last))));
		}
	};

	template< typename hashtype, typename type >
	struct HashSwitch<hashtype, type, 8> {
		static constexpr const hashtype next(const type _int, const hashtype _last = 1) {
			return HashFunc((const uint8_t)_int,
				HashFunc((const uint8_t)(_int >> 8),
					HashFunc((const uint8_t)(_int >> 16),
						HashFunc((const uint8_t)(_int >> 24),
							HashFunc((const uint8_t)(_int >> 32),
								HashFunc((const uint8_t)(_int >> 40),
									HashFunc((const uint8_t)(_int >> 48),
										HashFunc((const uint8_t)(_int >> 56),
											_last))))))));
		}
	};


}

template< typename hashtype, size_t len >
constexpr const hashtype Hash(const char(&_cstr)[len], const hashtype _last = 1) {
	return (len != 0) ? HashPipe< hashtype, len - 1 >::next(_cstr, _last) : _last;
}

template< typename hashtype, typename type >
constexpr const hashtype Hash(const type _pod, const hashtype _last = 1) {
	static_assert(std::is_integral< type >::value);
	return HashSwitch< hashtype, type, sizeof(type)>::next(_pod, _last);
}

/// Runtime hash for dynamic c strings
template< typename hashtype >
const hashtype RuntimeHash(const char *_cstr) {
	hashtype last = 1;
	auto len = strlen(_cstr);
	while (len != 0) {
		--len;
		last = HashFunc< hashtype >((const uint8_t)_cstr[len], last);
	}
	return last;
}

/// Runtime hash for c++ strings
template< typename hashtype >
const hashtype RuntimeHash(const std::string &_str) {

	hashtype last = 1;
	for (int i = _str.size() - 1; i >= 0; --i) {
		last = HashFunc< hashtype >((const uint8_t)_str[i], last);
	}
	return last;
}

/// Runtime for arbitary byte data of a fixed length
template< typename hashtype >
const hashtype RuntimeHash(const uint8_t *_data, const size_t _len, const hashtype _last = 1) {
	hashtype last = _last;
	size_t len = _len;
	while (len != 0) {
		--len;
		last = HashFunc< hashtype >(_data[len], last);
	}
	return last;
}

/// Runtime for POD types (can be converted to data of a fixed length
template< typename hashtype, typename type >
const hashtype RuntimeHash(const type &_data, const hashtype _last = 1) {
	static_assert(std::is_pod< type >::value);

	const uint8_t *data = (const uint8_t *)&_data;
	const size_t len = sizeof(type);

	return RuntimeHash(data, len, _last);

}