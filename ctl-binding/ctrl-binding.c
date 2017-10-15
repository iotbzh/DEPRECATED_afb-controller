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
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ctrl-binding.h"


// STATIC CtlConfigT *ctrlConfig=NULL;

STATIC void ctrlapi_ping (AFB_ReqT request) {
    static int count=0;

    count++;
    AFB_NOTICE ("Controller:ping count=%d", count);
    AFB_ReqSucess(request,json_object_new_int(count), NULL);

    return;
}

STATIC void ctrlapi_request (AFB_ReqT request) {
    static int count=0;

    count++;
    AFB_NOTICE ("Controller:request count=%d", count);
    AFB_ReqSucess(request,json_object_new_int(count), NULL);

    return;
}

// Config Section definition (note: controls section index should match handle retrieval in HalConfigExec)
static CtlSectionT ctrlSections[]= {
    [CTL_SECTION_PLUGIN] ={.key="plugins" , .loadCB= PluginConfig},
    [CTL_SECTION_ONLOAD] ={.key="onload"  , .loadCB= OnloadConfig},
    [CTL_SECTION_CONTROL]={.key="control" , .loadCB= ControlConfig},
    [CTL_SECTION_EVENT]  ={.key="event"   , .loadCB= EventConfig},
    
    [CTL_SECTION_ENDTAG] ={.key=NULL}
};

// Every HAL export the same API & Interface Mapping from SndCard to AudioLogic is done through alsaHalSndCardT
STATIC AFB_ApiVerbs CtrlApiVerbs[] = {
    /* VERB'S NAME         FUNCTION TO CALL         SHORT DESCRIPTION */
    { .verb = "ping",     .callback = ctrlapi_ping     , .info = "ping test for API"},
    { .verb = "request"  ,.callback = ctrlapi_request  , .info = "Request a control in V2 mode"},
    { .verb = NULL} /* marker for end of the array */
};

#ifdef AFB_BINDING_PREV3

STATIC int CtrlLoadStaticVerbs (afb_dynapi *dynapi, AFB_ApiVerbs *verbs) {
    int errcount=0;
    
    for (int idx=0; verbs[idx].verb; idx++) {
        errcount+= afb_dynapi_add_verb(dynapi, CtrlApiVerbs[idx].verb, NULL, CtrlApiVerbs[idx].callback, (void*)&CtrlApiVerbs[idx], CtrlApiVerbs[idx].auth, 0);
    }
    
    return errcount;
};


// next generation dynamic API-V3 mode

STATIC int CtrlLoadOneApi (void *cbdata, AFB_ApiT dynapi) {
    CtlConfigT *ctrlConfig = (CtlConfigT*) cbdata;

    // add static controls verbs
    int err = CtrlLoadStaticVerbs (dynapi, CtrlApiVerbs);
    if (err) {
        AFB_ERROR("CtrlLoadSection fail to register static V2 verbs");
        goto OnErrorExit;
    }
    
    // load section for corresponding API
    err= CtlLoadSections(ctrlConfig, ctrlSections, dynapi);
    
    // declare an event event manager for this API;
    afb_dynapi_on_event(dynapi, CtrlDispatchApiEvent);
    
    return err;
    
OnErrorExit:
    return 1;
}

PUBLIC int afbBindingVdyn(afb_dynapi *dynapi) {
    int status=0;
   
    const char *dirList= getenv("CONTROL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;
    
    json_object *configJ=CtlConfigScan (dirList, "control-");
    if (!configJ) {
        AFB_ERROR("CtlAfbBindingVdyn: No control-* config found invalid JSON %s ", dirList);
        goto OnErrorExit;
    }
    
    // We load 1st file others are just warnings
    for (int index = 0; index < json_object_array_length(configJ); index++) {
        json_object *entryJ = json_object_array_get_idx(configJ, index);
        char *filename;
        char*fullpath;
        
        int err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ERROR("CtrlBindingDyn HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
            goto OnErrorExit;
        }

        char filepath[CONTROL_MAXPATH_LEN];
        strncpy(filepath, fullpath, sizeof (filepath));
        strncat(filepath, "/", sizeof (filepath));
        strncat(filepath, filename, sizeof (filepath));

        // create one API per file
        CtlConfigT *ctrlConfig = CtlLoadMetaData (filepath);
        if (!ctrlConfig) {
            AFB_ERROR("CtrlBindingDyn No valid control config file in:\n-- %s", filepath);
            goto OnErrorExit;
        }
        
        if (!ctrlConfig->api) {
            AFB_ERROR("CtrlBindingDyn API Missing from metadata in:\n-- %s", filepath);
            goto OnErrorExit;
        }
        // create one API per config file
        status += afb_dynapi_new_api(dynapi, ctrlConfig->api, ctrlConfig->info, CtrlLoadOneApi, ctrlConfig);
    }
 
    return status;
    
OnErrorExit:
    return 1;
}

#endif



#ifndef AFB_BINDING_PREV3
PUBLIC CtlConfigT *ctrlConfig=NULL;

STATIC int CtrlV2Init () {
    // process config sessions
    int err = CtlConfigExec (ctrlConfig);
    
    return err;   
}

// In APIV2 we may load only one API per binding 
STATIC int CtrlPreInit() {

    const char *dirList= getenv("CONTROL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;
    const char *configPath = CtlConfigSearch (dirList, "control-");
    
    if (!configPath) {
        AFB_ERROR("CtlPreInit: No control-* config found invalid JSON %s ", dirList);
        goto OnErrorExit;
    }
    
    // create one API per file
    ctrlConfig = CtlLoadMetaData (configPath);
    if (!ctrlConfig) {
        AFB_ERROR("CtrlPreInit No valid control config file in:\n-- %s", configPath);
        goto OnErrorExit;
    }

    if (ctrlConfig->api) {
        int err = afb_daemon_rename_api(ctrlConfig->api);
        if (err) {
            AFB_ERROR("Fail to rename api to:%s", ctrlConfig->api);
            goto OnErrorExit;
        }
    }
    
    int err= CtlLoadSections(ctrlConfig, ctrlSections, NULL);
    return err;
    
OnErrorExit:
    return 1;
}

// compatibility mode with APIV2
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "ctl",
    .preinit = CtrlPreInit,
    .init    = CtrlV2Init,
    .verbs   = CtrlApiVerbs,
    .onevent = CtrlDispatchV2Event,
};
  
#endif
