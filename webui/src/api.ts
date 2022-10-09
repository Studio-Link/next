import { config } from './config';

function api_request(met: string, url: string, data: string | null) {
    const req = new XMLHttpRequest()
    req.open(met, 'http://' + config.ws_host() + '/api/v1' + url)
    req.onerror = (e) => { console.log(e) }
    req.send(data)
}

export default {
    track_add(type: string) {
        api_request('POST', '/tracks/' + type, null);
    },

    track_del(id: number) {
        api_request('DELETE', '/tracks', String(id));
    },

    audio_src_dev(track: number, device: number) {
        api_request('PUT', '/audio/src/' + String(track), String(device));
    },

    audio_play_dev(track: number, device: number) {
        api_request('PUT', '/audio/play/' + String(track), String(device));
    }
}
