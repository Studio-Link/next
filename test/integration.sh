#!/usr/bin/env bash

# Copyright 2022 - Studio.Link Sebastian Reimers

set -o errexit  # Exit on most errors
set -o nounset  # Disallow expansion of unset variables
set -o errtrace # Make sure any error trap is inherited
set -o pipefail # Use last non-zero exit code in a pipeline
set -o xtrace   # Trace the execution of the script

IFS=$'\n\t'

test_url="127.0.0.1:9999"

NC='\033[0m' # Text Reset
RED='\033[0;31m'
GREEN='\033[0;32m'

# DESC: Handler for unexpected errors
# ARGS: $1 (optional): Exit code (defaults to 1)
# OUTS: None
script_trap_err() {
	local exit_code=1

	# Disable the error trap handler to prevent potential recursion
	trap - ERR

	# Consider any further errors non-fatal to ensure we run to completion
	set +o errexit
	set +o pipefail

	# Validate any provided exit code
	if [[ ${1-} =~ ^[0-9]+$ ]]; then
		exit_code="$1"
	fi

	echo -e "${RED}${FUNCNAME[2]}$NC"
	echo -e "${RED}${FUNCNAME[1]} - TEST FAILED!$NC"
	#exec 1>&3 2>&4
	#cat $script_output

	# Exit with failure status
	exit "$exit_code"
}

# DESC: Handler for exiting the script
# ARGS: None
# OUTS: None
script_trap_exit() {
	exit_code=$?
	if [[ -f ${script_output-} ]]; then
		rm "$script_output"
		# exec 1>&3 2>&4
	fi
	if [[ $exit_code -eq 0 ]]; then
		echo -e "${GREEN}All integration tests were sucessfully!$NC"
	fi

	kill -INT "$test_pid"
}

script_init() {
	readonly script_output="$(mktemp "/tmp/test".XXXXX)"
	../app/linux/studiolink --headless 1>>"$script_output" 2>&1 &
	test_pid="$!"

	# exec 3>&1 4>&2 1>"$script_output" 2>&1
}

curl_head() {
	curl -I "${test_url}$1" 2>/dev/null
}

curl_post() {
	curl --fail -X POST "${test_url}$1"
}

ws_test() {
	websocat -1 -E -t "ws://${test_url}$1" -
}

# --- TESTS ---
a_user_gets_404_if_page_not_exists() {
	curl_head /does_not_exists | head -1 | grep 404 >/dev/null
}

a_user_can_connect_with_websocket() {
	ws_test /ws/v1/tracks
}

a_user_can_call_cli_help() {
	../app/linux/studiolink -h || [[ $? == 254 ]]
}

a_user_can_not_call_unknown_cli_options() {
	../app/linux/studiolink -sjdfksdf || [[ $? == 22 ]]
	../app/linux/studiolink -Hasldkfj || [[ $? == 22 ]]
	../app/linux/studiolink -x || [[ $? == 22 ]]
	../app/linux/studiolink --headlesssadlkfjasd || [[ $? == 22 ]]
	../app/linux/studiolink --headless="asdfjdsf" || [[ $? == 22 ]]
}

a_user_can_add_tracks() {
	track_count=$(ws_test /ws/v1/tracks | jq ".[].type" | grep -c local)
	[ "$track_count" == "1" ]
	
	curl_post /api/v1/tracks/remote
	curl_post /api/v1/tracks/remote
	track_count=$(ws_test /ws/v1/tracks | jq ".[].type" | grep -c remote)
	[ "$track_count" == "2" ]
}

# --- TESTS ---

# DESC: Main control flow
# ARGS: $@ (optional): Arguments provided to the script
# OUTS: None
main() {
	trap script_trap_err ERR
	trap script_trap_exit EXIT
	script_init "$@"

	a_user_can_call_cli_help
	a_user_can_not_call_unknown_cli_options
	a_user_gets_404_if_page_not_exists
	a_user_can_connect_with_websocket
	a_user_can_add_tracks
}

# ready to backup?
main "$@"
