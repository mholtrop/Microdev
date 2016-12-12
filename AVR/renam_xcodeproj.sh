#!/bin/bash 
if [ -z $2 ]; then
    old_name="XXX_PROJ_NAME"
else
    old_name=$2
fi

if [ -z $1 ]; then
    echo "Please call with a name to change to."
else    
    new_name=$1
    echo "Renaming project files from ${old_name} to ${new_name} "
    mv Template_proj.xcodeproj ${new_name}.xcodeproj
    for f in ${new_name}.xcodeproj/project.pbxproj ${new_name}.xcodeproj/project.xcworkspace/contents.xcworkspacedata ${new_name}.xcodeproj/xcuserdata/maurik.xcuserdatad/xcschemes/*; do
#	echo "Fixing file: $f - sed -e 's/${old_name}/${new_name}/g'  $f "
	sed -e "s/${old_name}/${new_name}/g"  $f > tmpss && mv tmpss $f;
    done
    rm -rf ${new_name}.xcodeproj/project.xcworkspace/xcuserdata/maurik.xcuserdatad
fi
