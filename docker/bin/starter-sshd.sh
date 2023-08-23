#!/bin/bash

HOST_KEY_DIR=/etc/ssh/ssh_host_rsa_key

if [ ! -f "${HOST_KEY_DIR}" ]; then
    ssh-keygen -A
fi
/usr/sbin/sshd

echo sshd started!
