#pragma once

#if !defined(NDEBUG) // TODO: Change: Everything but Release.
#	define FT_CHECK(condition) if(!(condition)) { __debugbreak(); }
#	define FT_CHECK_MSG(condition, fmt, ...) \
		if(!(condition)) \
		{ \
			fprintf(stderr, fmt, __VA_ARGS__); \
			__debugbreak(); \
		}

#define FT_FAIL(message) FT_CHECK_MSG(0, message)
#else
#	define FT_CHECK(condition)
#	define FT_CHECK_MSG(condition, message)
#	define FT_FAIL(message)
#endif

#define FT_VK_CALL(call) \
	do { \
		VkResult result = call; \
		FT_CHECK(result == VK_SUCCESS) \
	} while (0)
