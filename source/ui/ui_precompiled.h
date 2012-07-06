#include <Rocket/Core.h>

#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"
#include "../gameshared/gs_ref.h"

// few fixes regarding Quake and std compatibility
#ifdef min
	#undef min
#endif
#ifdef max
	#undef max
#endif

// STD
#include <string>
#include <new>
#include <algorithm>
#include <stdexcept>

#include "kernel/ui_syscalls.h"
