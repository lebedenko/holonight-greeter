#!/usr/bin/env bash

set -euo pipefail

live_root=/tmp/holonight-greeter-live
getty_was_active=false

if systemctl is-active --quiet getty@tty2.service; then
  getty_was_active=true
fi

restore_getty() {
  if [[ "${getty_was_active}" == true ]]; then
    sudo systemctl start getty@tty2.service
  fi
}
trap restore_getty EXIT

sudo systemctl stop getty@tty2.service
sudo sh -c 'ulimit -c unlimited; exec greetd "$@"' sh \
  --config "${live_root}/greetd-live.toml" \
  --socket-path /run/greetd-holonight-live.sock \
  --vt 2 2>&1 | tee "${live_root}/greetd-live.log"
