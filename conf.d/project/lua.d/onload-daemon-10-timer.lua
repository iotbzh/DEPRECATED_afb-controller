--[[
  Copyright (C) 2016 "IoT.bzh"
  Author Fulup Ar Foll <fulup@iot.bzh>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.


  Provide Sample Timer Handing to push event from LUA
--]]

-- Create event on Lua script load
_MyContext={}

-- WARNING: call back are global and should start with '_'
function _Timer_Test_CB (source, context)

   local evtinfo = AFB:timerget(_MyContext["timer"])
   printf ("[-- _Timer_Test_CB --] evtinfo=%s context=%s", Dump_Table(evtinfo), Dump_Table(context))

   --send an event an event with count as value
   AFB:evtpush (source, _MyContext["event"], {["label"]= evtinfo["label"], ["count"]=evtinfo["count"], ["info"]=context["info"]})

   -- note when timerCB return!=0 timer is kill
   return 0

end

-- sendback event depending on count and delay
function _Simple_Timer_Start (source, args, query)
    local error
    local context = {
        ["info"]="My 1st private Event",
    }

    -- if event does not exit create it now.
    if (_MyContext["event"] == nil) then
      _MyContext["event"]= AFB:evtmake(source, "MyTimerEvent")
    end

    -- if delay not defined default is 5s
    if (query["delay"]==nil) then query["delay"]=5000 end

    -- if count is not defined default is 10
    if (query["count"]==nil) then query["count"]=10 end

    -- we could use directly query but it is a sample
    local myTimer = {
       ["label"]=query["label"],
       ["delay"]=query["delay"],
       ["count"]=query["count"],
    }
    AFB:notice (source, "Test_Timer myTimer=%s", myTimer)

    -- subscribe to event
    AFB:subscribe (source, _MyContext["event"])

    -- settimer take a table with delay+count as input (count==0 means infinite)
    error, _MyContext["timer"] = AFB:timerset (source, myTimer, "_Timer_Test_CB", context)

    -- return status
    if (error == nil) then
        AFB:success (source, myTimer)
    else
        AFB:fail (source, "Fail to Create Timer")
    end    

end
