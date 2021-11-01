#pragma once

// TOOD: Add FT_LOG_ERR instead direct fprintf.
#if !defined(NDEBUG) // TODO: Change: Everything but Release.
#	define FT_CHECK(condition) do { if(!(condition)) { __debugbreak(); } } while(0)
#	define FT_CHECK_MSG(condition, fmt, ...) \
		do \
		{ \
			if(!(condition)) \
			{ \
				fprintf(stderr, fmt, __VA_ARGS__); \
				__debugbreak(); \
			} \
		} \
		while (0)

#define FT_FAIL(fmt, ...) FT_CHECK_MSG(0, fmt, __VA_ARGS__)
#else
#	define FT_CHECK(condition)
#	define FT_CHECK_MSG(condition, fmt, ...)
#	define FT_FAIL(fmt, ...)
#endif

#define FT_VK_CALL(call) \
	do { \
		VkResult result = call; \
		FT_CHECK(result == VK_SUCCESS); \
	} while (0)
