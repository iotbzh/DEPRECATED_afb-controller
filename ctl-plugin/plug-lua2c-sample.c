/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Sample plugin for Controller
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "ctl-plugin.h"

// Declare this sharelib as a Controller Plugin
CTLP_LUA_REGISTER("Lua2CSamplePlugin");

// Ultra basic context control
#define MY_PLUGIN_MAGIC 123456789

typedef struct {
  int magic;
  int count;
} MyPluginCtxT;

STATIC const char* jsonToString (json_object *valueJ) {
    const char *value;
    if (valueJ)
        value=json_object_get_string(valueJ);
    else
        value="NULL";
    return value;
}


// Call at initialisation time
PUBLIC CTLP_ONLOAD(plugin, handle) {
    
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)calloc (1, sizeof(MyPluginCtxT));
    pluginCtx->magic = MY_PLUGIN_MAGIC;
    pluginCtx->count = -1;

    AFB_ApiNotice (plugin->api, "CONTROLLER-PLUGIN-SAMPLE:Onload label=%s info=%s", plugin->label, plugin->info);
    return (void*)pluginCtx;
}


// This function is a LUA function. Lua2CHelloWorld label should be declare in the "onload" section of JSON config file
PUBLIC CTLP_LUA2C (Lua2cHelloWorld1, source, argsJ, responseJ) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)source->context;

    if (!pluginCtx || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ApiError(source->api, "CONTROLLER-PLUGIN-SAMPLE:Lua2cHelloWorld1 (Hoops) Invalid Sample Plugin Context");
        return 1;
    };
    pluginCtx->count++;
    AFB_ApiNotice (source->api, "CONTROLLER-PLUGIN-SAMPLE:Lua2cHelloWorld1 SamplePolicyCount action=%s args=%s count=%d", source->label, jsonToString(argsJ), pluginCtx->count);
    
    // build response as a json object
    *responseJ = json_object_new_string("Response from Lua2C");
    
    return 0;
}
