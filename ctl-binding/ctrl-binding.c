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

// Include Binding Stub generated from Json OpenAPI
#include "ctl-apidef.h"


STATIC CtlConfigT *ctrlConfig=NULL;

typedef enum {
  CTL_SECTION_PLUGIN,  
  CTL_SECTION_ONLOAD,  
  CTL_SECTION_CONTROL,  
  CTL_SECTION_EVENT,
          
  CTL_SECTION_ENDTAG, 
} SectionEnumT;

// Config Section definition (note: controls section index should match handle retrieval in HalConfigExec)
static CtlSectionT ctrlSections[]= {
    [CTL_SECTION_PLUGIN] ={.key="plugins" , .loadCB= PluginConfig},
    [CTL_SECTION_ONLOAD] ={.key="onload"  , .loadCB= OnloadConfig},
    [CTL_SECTION_CONTROL]={.key="control" , .loadCB= ApiMapConfig},
    [CTL_SECTION_EVENT]  ={.key="event"   , .loadCB= ActionConfig},
    
    [CTL_SECTION_ENDTAG] ={.key=NULL}
};

// Every HAL export the same API & Interface Mapping from SndCard to AudioLogic is done through alsaHalSndCardT
STATIC afb_verb_v2 CtrlApiVerbs[] = {
    /* VERB'S NAME         FUNCTION TO CALL         SHORT DESCRIPTION */
    { .verb = "ping",     .callback = ctrlapi_ping     , .info = "ping test for API"},
    { .verb = "subscribe",.callback = ctrlapi_subscribe, .info = "Subscribe to controller event"},
    { .verb = "request"  ,.callback = ctrlapi_request  , .info = "Request a control in V2 mode"},
    { .verb = NULL} /* marker for end of the array */
};

STATIC int CtrlLoadStaticVerbs (afb_dynapi *dynapi, afb_verb_v2 *verbs) {
    int errcount=0;
    
    for (int idx=0; verbs[idx].verb; idx++) {
        errcount+= afb_dynapi_add_verb(dynapi, verbs[idx].verb, NULL, verbs[idx].callback, (void*)&verbs[idx], verbs[idx].auth, 0);
    }
    
    return errcount;
};

STATIC void ctrlapi_ping (AFB_ReqT request) {
    static int count=0;

    count++;
    AFB_NOTICE ("Controller:ping count=%d", count);
    AFB_ReqSucess(request,json_object_new_int(count), NULL);

    return;
}

STATIC void ctrlapi_subscribe (AFB_ReqT request) {

    // subscribe Client to event
    int err = afb_req_subscribe(request, TimerEvtGet());
    if (err != 0) {
        AFB_ReqFailF(request, "register-event", "Fail to subscribe binder event");
        goto OnErrorExit;
    }

    AFB_ReqSucess(request, NULL, NULL);

 OnErrorExit:
    return;
}

STATIC void ctrlapi_request (AFB_ReqT request) {
    static int count=0;

    count++;
    AFB_NOTICE ("Controller:request count=%d", count);
    AFB_ReqSucess(request,json_object_new_int(count), NULL);

    return;
}

// call action attached to even name if any
PUBLIC void CtrlDispatchEvent(const char *evtLabel, json_object *eventJ) {
    CtlSectionT* actions = ctlConfig[CTL_SECTION_EVENT];
            
    int index= ActionLabelToIndex(CtlActionT* actions, evtLabel);
    if (index < 0) {
        AFB_WARNING ("CtlDispatchEvent: fail to find label=%s in action event section", evtLabel);
        return;
    }

    // Best effort ignoring error to exec corresponding action 
    (void) ActionExecOne (actions[index], eventJ);
}


STATIC int CtrlBindingInit () {
    // process config sessions
    int err = CtlConfigExec (ctlConfig);
    
    return err;   
}

// In APIV2 we may load only one API per binding 
STATIC int CtrlPreInit() {
    char *apiname;
    const char *dirList= getenv("CONTROL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;
    const char *configPath = CtlConfigSearch (dirList, "control-");
    
    if (!configPath) {
        AFB_ERROR("CtlPreInit: No control-* config found invalid JSON %s ", dirList);
        goto OnErrorExit;
    }
    
    // create one API per file
    CtlConfigT *ctlHandle = CtlLoadMetaData (configPath);
    if (!ctlHandle) {
        AFB_ERROR("CtrlPreInit No valid control config file in:\n-- %s", configPath);
        goto OnErrorExit;
    }

    if (ctlHandle->api) {
        int err = afb_daemon_rename_api(ctlConfig->api);
        if (err) {
            AFB_ERROR("Fail to rename api to:%s", ctlHandle->api);
            goto OnErrorExit;
        }
    }
    
    ctrlConfig= CtlLoadSections(ctlHandle, ctrlSections, NULL);
    return err;
    
OnErrorExit:
    return 1;
}


#ifdef AFB_DYNAPI_INFO
// next generation dynamic API-V3 mode

STATIC int CtrlLoadOneApi (void *cbdata, afb_dynapi *dynapi) {
    CtlConfigT *ctlHandle = (json_object*) cbdata;

    // add static controls verbs
    int err = CtrlLoadStaticVerbs (dynapi, CtrlApiVerbs);
    if (err) {
        AFB_ERROR("CtrlLoadSection fail to register static V2 verbs");
        goto OnErrorExit;
    }
    
    ctrlConfig= CtlLoadSections(ctlHandle, ctrlSections, dynapi);
    
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
    for (index = 0; index < json_object_array_length(configJ); index++) {
        json_object *entryJ = json_object_array_get_idx(configJ, index);
        json_object *configJ;
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
        CtlConfigT *ctlHandle = CtlLoadMetaData (filepath);
        if (!ctlHandle) {
            AFB_ERROR("CtrlBindingDyn No valid control config file in:\n-- %s", filepath);
            goto OnErrorExit;
        }
        
        if (!ctlHandle->api) {
            AFB_ERROR("CtrlBindingDyn API Missing from metadata in:\n-- %s", filepath);
            goto OnErrorExit;
        }
        // create one API per config file
        status += afb_dynapi_new_api(dynapi, ctlHandle->api, ctlHandle, CtrlLoadOneApi, configJ);
    }
 
    return status;
    
OnErrorExit:
    return 1;
}

#else

// compatibility mode with APIV2
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "ctl",
    .preinit = CtrlPreInit,
    .init    = ConfigExec,
    .verbs   = ApiVerbs,
    .onevent = CtrlOneEvent,
};
  
#endif
