#!/bin/bash

# create group
groupadd -g 2000 -f otto

# create user
id -u otto &>/dev/null || useradd -g otto -m -c "OTTO User" otto

