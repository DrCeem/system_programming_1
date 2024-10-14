#!/bin/bash

killall ./progDelay
./jobCommander issueJob ./jobCommander poll running
./jobCommander issueJob ./jobCommander poll queued
./jobCommander exit