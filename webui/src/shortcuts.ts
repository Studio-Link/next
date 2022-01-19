import { tracks } from './states/tracks'
import api from './api'

function track_select(id: number) {
	tracks.select(id)
	document.getElementById('track' + id)?.focus()
}

document.onkeydown = (event) => {
	//ignore global events if input field is focused
	const inputs = document.getElementsByTagName('input');
	for (let i = 0, len = inputs.length; i < len; i++) {
		const input = inputs[i];
		if (input === document.activeElement)
			return
	}

	// --- Track shortcuts ---
	if (event.code == 'Digit1') {
		track_select(1)
	}
	if (event.code == 'Digit2') {
		track_select(2)
	}
	if (event.code == 'Digit3') {
		track_select(3)
	}
	if (event.code == 'Digit4') {
		track_select(4)
	}
	if (event.code == 'Digit5') {
		track_select(5)
	}
	if (event.code == 'Digit6') {
		track_select(6)
	}
	if (event.code == 'Digit7') {
		track_select(7)
	}
	if (event.code == 'Digit8') {
		track_select(8)
	}
	if (event.code == 'Digit9') {
		track_select(9)
	}

	// Previous Track
	if (event.code == 'ArrowUp' || event.code == 'ArrowLeft' ||
		event.code == 'KeyH' || event.code == 'KeyK') {
		track_select(tracks.selected() - 1)
	}

	// Next Track
	if (event.code == 'ArrowDown' || event.code == 'ArrowRight' ||
		event.code == 'KeyL' || event.code == 'KeyJ') {
		track_select(tracks.selected() + 1)
	}

	// New Track
	if (event.code == 'KeyN') {
		api.track_add('remote')
	}

	// Remote Track
	if (event.code == 'KeyX') {
		const id = tracks.selected()
		if (id == -1 || id == 1)
			return

		api.track_del(id)
	}
}
