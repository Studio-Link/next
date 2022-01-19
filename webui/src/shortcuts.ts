import { tracks } from './states/tracks'
import api from './api'

document.onkeydown = (event) => {
	//ignore global events if input field is focused
	const inputs = document.getElementsByTagName('input');
	for (let i = 0, len = inputs.length; i < len; i++) {
		const input = inputs[i];
		if (input === document.activeElement)
			return
	}

	if (event.code == 'Digit1') {
		tracks.select(1)
		document.getElementById('track1')?.focus()
	}
	if (event.code == 'Digit2') {
		tracks.select(2)
		document.getElementById('track2')?.focus()
	}
	if (event.code == 'Digit3') {
		tracks.select(3)
		document.getElementById('track3')?.focus()
	}
	if (event.code == 'Digit4') {
		tracks.select(4)
		document.getElementById('track4')?.focus()
	}
	if (event.code == 'Digit5') {
		tracks.select(5)
		document.getElementById('track5')?.focus()
	}
	if (event.code == 'Digit6') {
		tracks.select(6)
		document.getElementById('track6')?.focus()
	}
	if (event.code == 'Digit7') {
		tracks.select(7)
		document.getElementById('track7')?.focus()
	}
	if (event.code == 'Digit8') {
		tracks.select(8)
		document.getElementById('track8')?.focus()
	}
	if (event.code == 'Digit9') {
		tracks.select(9)
		document.getElementById('track9')?.focus()
	}

	if (event.code == 'KeyN') {
		api.track_add('remote')
	}

	if (event.code == 'KeyX') {
		const id = tracks.selected()
		if (id == -1 || id == 1)
			return

		api.track_del(id)
	}
}
