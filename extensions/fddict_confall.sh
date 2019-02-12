#Copyright (c) 2017 Sprint
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/sh

usage () 
{
  echo "Usage: $0 <dictionary name> <ON|OFF> <config file>"
}

if [ "$#" -ne 3 ]; then
  usage
  exit 0
fi

if [ "$2" != "ON" ] && [ "$2" != "OFF" ]; then
  usage
  exit
fi 

DCTDIR=$1
STATE=$2
CFGFILE=$3

if [ -e $3 ]; then
  if [ -d "dict_${DCTDIR}" ]; then
    if [ -e "dict_${DCTDIR}/${DCTDIR}.did" ]; then
      while read p; do
        echo "Turning $p $2"
        ./fddict_conf.sh $p $2 $3 
      done <dict_${DCTDIR}/${DCTDIR}.did  
    else
      echo "dict_${DCTDIR}.did was not present - skipping"
    fi
  else
    echo "Directory dict_${DCTDIR} doesn't exist\n"
  fi
else
  echo "File $3 doesn't exist. Please provide valid config file\n"
fi
