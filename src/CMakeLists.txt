# Copyright (C) Eta Scale AB. Licensed under the Eta Scale Open Source License. See the LICENSE file for details.

include_directories ("${PROJECT_SOURCE_DIR}/backend")
add_subdirectory (backend)

# put together all ArgoDSM source files
set(argo_sources "argo.cpp")

set(allocator_sources allocators.cpp)
foreach(src ${allocator_sources})
	list(APPEND argo_sources allocators/${src})
endforeach(src)

set(synchronization_sources
	synchronization.cpp
	cohort_lock.cpp
	intranode/mcs_lock.cpp
)
foreach(src ${synchronization_sources})
	list(APPEND argo_sources synchronization/${src})
endforeach(src)

# add the frontend library
add_library(argo ${argo_sources})


option(ARGO_USE_LIBNUMA
	"Use libnuma to determine NUMA structure within ArgoDSM" ON)
if(ARGO_USE_LIBNUMA)
	target_link_libraries(argo numa)
	add_definitions(-DARGO_USE_LIBNUMA)
endif(ARGO_USE_LIBNUMA)

#install (TARGETS argo DESTINATION bin)

install(TARGETS argo
	COMPONENT "Runtime"
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(DIRECTORY .
	DESTINATION include/argo
	COMPONENT "Development"
	FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hpp")
