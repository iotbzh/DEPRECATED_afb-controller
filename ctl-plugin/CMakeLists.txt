###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
#
# author: Fulup Ar Foll <fulup@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################



PROJECT_TARGET_ADD(plug-capi-sample)

    ADD_LIBRARY(${TARGET_NAME} MODULE plug-capi-sample.c)

    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	PREFIX ${CTL_PLUGIN_PRE}
        SUFFIX ${CTL_PLUGIN_EXT}
        OUTPUT_NAME ${TARGET_NAME}
    )

    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        afb-utilities
        ctl-utilities
        ${link_libraries}
    )

#add_compile_options(-E)
PROJECT_TARGET_ADD(plug-lua2c-sample)

    ADD_LIBRARY(${TARGET_NAME} MODULE plug-lua2c-sample.c)

    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	PREFIX ${CTL_PLUGIN_PRE}
        SUFFIX ${CTL_PLUGIN_EXT}
        OUTPUT_NAME ${TARGET_NAME}
    )

    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        afb-utilities
        ctl-utilities
        ${link_libraries}
    )



