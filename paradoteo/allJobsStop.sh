#!/bin/bash

output_queued=$(./jobCommander poll queued)
queued_ids=$(awk '{print $1}' <<<"$output_queued")

while read -r qid; do
    ./jobCommander stop $qid
done <<< "$queued_ids"

output_running=$(./jobCommander poll running)
running_ids=$(awk '{print $1}' <<<"$output_running")

while read -r lid; do
    ./jobCommander stop $lid
done <<< "$running_ids"


