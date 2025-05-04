##
# \file clean.sh
# \author Isaiah Lateer
# 
# Used to clean the workspace.
# 

#!/bin/bash

directory=$(dirname "$0")

pushd $directory

rm -rf obj
rm -rf bin

popd
