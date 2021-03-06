#pragma once

#ifdef NDEBUG
#	define FT_CHECK(condition, fmt, ...) \
		do \
		{ \
			if (!(condition)) \
			{ \
				FT_LOG(fmt, __VA_ARGS__); \
			} \
		} \
		while (0)
#else
#	define FT_CHECK(condition, fmt, ...) \
		do \
		{ \
			if(!(condition)) \
			{ \
				FT_LOG(fmt, __VA_ARGS__); \
				__debugbreak(); \
			} \
		} \
		while (0)
#endif // NDEBUG

#define FT_FAIL(msg) \
	do \
	{ \
		FT_LOG(msg); \
		throw std::runtime_error(msg); \
	} \
	while(0)
