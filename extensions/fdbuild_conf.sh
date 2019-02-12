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
  echo "Scans the current directory and adds, as required, the "
  echo "extensions to the specified config file intially disabled."
  echo "Usage: $0 <config file>"
}

if [ "$#" -ne 1 ]; then
  usage
  exit 0
fi

if [ -e $1 ]; then
  echo "Updating $1"
  for D in *; do
    if [ -d "${D}" ]; then
      INDIR=`grep -c "${D}.fdx" $1`
      if [ "$INDIR" -eq 0 ]; then
	echo "Adding ${D} to file"
	sed -i '$ i#LoadExtension = \"/usr/local/lib/freeDiameter/'"${D}"'.fdx\";' $1
      fi
    fi
  done
else
  echo "File $1 does not exist. Please provide valid config file\n"
fi
