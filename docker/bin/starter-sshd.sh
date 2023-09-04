#!/bin/bash

HOST_KEY_DIR=/etc/ssh/ssh_host_rsa_key

if [ -f /etc/.firstrun ]; then
  echo "root:root" | chpasswd
  rm -f /etc/.firstrun
fi

if [ ! -f "${HOST_KEY_DIR}" ]; then
    ssh-keygen -A
fi
/usr/sbin/sshd

echo sshd started!
