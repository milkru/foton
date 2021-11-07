#pragma once

// TOOD: Add FT_LOG_ERR instead direct fprintf. And Log FT_FAIL too.
#if !defined(NDEBUG) // TODO: Change: Everything but Release.
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

#define FT_FAIL(msg) do { throw std::runtime_error(msg); } while(0)
#else
#	define FT_CHECK_MSG(condition, fmt, ...)
#	define FT_FAIL(fmt, ...)
#endif

// TODO: This shouldn't be here.
#define FT_VK_CALL(call) \
	do { \
		VkResult result = call; \
		FT_CHECK_MSG(result == VK_SUCCESS, "Vulkan API call failed."); \
	} \
	while (0)
