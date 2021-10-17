#pragma once

#if !defined(NDEBUG) // TODO: Change: Everything but Release.
#	include "optick.h"

#	define FT_PROFILE_BLOCK(name)				ProfileBlock profileBlock(name)
#	define FT_PROFILE_PUSH(name)				do { OPTICK_PUSH(name) } while (0)
#	define FT_PROFILE_POP()						do { OPTICK_POP() } while (0)
#	define FT_PROFILE_EVENT()					do { OPTICK_EVENT() } while (0)

#	define FT_PROFILE_START_CAPTURE()			do { OPTICK_START_CAPTURE() } while (0)
#	define FT_PROFILE_STOP_CAPTURE(fileName)	do { OPTICK_STOP_CAPTURE(); OPTICK_SAVE_CAPTURE(fileName); } while (0)

#	define FT_PROFILE_FRAME(name)				do { OPTICK_FRAME(name) } while (0)
#	define FT_PROFILE_THREAD(name)				do { OPTICK_THREAD(name) } while (0)

namespace FT
{
	struct ProfileBlock
	{
		ProfileBlock(const char* name)	{ FT_PROFILE_PUSH(name); }
		~ProfileBlock()					{ FT_PROFILE_POP(); }
	};
}
#else
#	define FT_PROFILE_BLOCK(name)
#	define FT_PROFILE_PUSH(name)
#	define FT_PROFILE_POP()
#	define FT_PROFILE_EVENT()

#	define FT_PROFILE_START_CAPTURE()
#	define FT_PROFILE_STOP_CAPTURE(fileName)

#	define FT_PROFILE_FRAME(name)
#	define FT_PROFILE_THREAD(name)
#endif
