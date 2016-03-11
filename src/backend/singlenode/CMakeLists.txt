# Copyright (C) Eta Scale AB. Licensed under the ArgoDSM Open Source License. See the LICENSE file for details.

add_library(argobackend-singlenode singlenode.cpp)

install(TARGETS argobackend-singlenode
	COMPONENT "Runtime"
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
