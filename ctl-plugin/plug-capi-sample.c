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



#define MY_PLUGIN_MAGIC 987654321

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

// Declare this sharelib as a Controller Plugin
CTLP_REGISTER("MyCtlSamplePlugin");


// Call at initialisation time
PUBLIC CTLP_ONLOAD(plugin, api) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)calloc (1, sizeof(MyPluginCtxT));
    pluginCtx->magic = MY_PLUGIN_MAGIC;
    pluginCtx->count = -1;

    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:Onload label=%s version=%s info=%s", plugin->label, plugin->info, plugin->version);
    return (void*)pluginCtx;
}


PUBLIC CTLP_CAPI (SamplePolicyInit, source, argsJ, queryJ) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)pluginCtx;
    if (!pluginCtx || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:SamplePolicyInit (Hoops) Invalid Sample Plugin Context");
        return -1;
    };

    pluginCtx->count = 0;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:Init label=%s args=%s\n", source->label, jsonToString(argsJ));
    return 0;
}

PUBLIC CTLP_CAPI (sampleControlMultimedia, source, argsJ,queryJ) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)source->context;

    if (!pluginCtx || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:sampleControlMultimedia (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:sampleControlMultimedia SamplePolicyCount action=%s args=%s query=%s count=%d"
               , source->label, jsonToString(argsJ), jsonToString(queryJ), pluginCtx->count);
    return 0;
}

PUBLIC  CTLP_CAPI (sampleControlNavigation, source, argsJ, queryJ) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)source->context;

    if (!pluginCtx || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:sampleControlNavigation (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:sampleControlNavigation SamplePolicyCount action=%s args=%s query=%s count=%d"
               ,source->label, jsonToString(argsJ), jsonToString(queryJ), pluginCtx->count);
    return 0;
}

PUBLIC  CTLP_CAPI (SampleControlEvent, source, argsJ, queryJ) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)source->context;

    if (!pluginCtx || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:cousampleControlMultimediant (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:sampleControlMultimedia SamplePolicyCount action=%s args=%s query=%s count=%d"
               ,source->label, jsonToString(argsJ), jsonToString(queryJ), pluginCtx->count);
    return 0;
}
