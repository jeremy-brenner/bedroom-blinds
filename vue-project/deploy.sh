#!/bin/bash

for oldstuff in $(curl -s http://bedroomblinds/api/ls | jq -r '.root.entries[] | select(.name == "webapp").entries[] | select(.name == "assets").entries[] | "webapp/assets/\(.name)"'
); do
  curl -X DELETE http://bedroomblinds/${oldstuff}
done

for fn in $(find ./dist/ -type f); do
  trimmed=$(cut -d '/' -f '3-' <<< "${fn}")
  pathname=$(dirname "${trimmed}")
  if [ "${pathname}" == "." ]; then
    pathname=""
  fi
  echo "${fn}"
  curl -F "file=@${fn}" http://bedroomblinds/${pathname}  
  echo
done