# Copyright (C) Eta Scale AB. Licensed under the Eta Scale Open Source License. See the LICENSE file for details.

# which backends to enable?
option(ARGO_BACKEND_SINGLENODE
	"Provide the singlenode (test-only) backend" ON)
option(ARGO_BACKEND_MPI
	"Provide the default MPI backend" ON)

#create list of enabled backends
set(backends "")
if(ARGO_BACKEND_SINGLENODE)
	list(APPEND backends singlenode)
endif(ARGO_BACKEND_SINGLENODE)
if(ARGO_BACKEND_MPI)
	list(APPEND backends mpi)
endif(ARGO_BACKEND_MPI)

# make backends visible to other parts of the build system (used in tests)
#set (backends ${backends} PARENT_SCOPE)
set(backends "${backends}" CACHE INTERNAL "backends")

foreach(BACKEND IN LISTS backends)
	include_directories("${PROJECT_SOURCE_DIR}/${BACKEND}")
	add_subdirectory(${BACKEND})
endforeach(BACKEND)

#install(FILES backend.hpp DESTINATION include)
