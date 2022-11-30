#!/bin/bash
ls -lld $PWD/*starter-* | awk '{print $9;}' | xargs -L 1 bash -c
echo 'starter scripts run successfully!'
tail -f /dev/null
