import { config } from './config';

function api_request(met: string, url: string, data: string | null) {
    const req = new XMLHttpRequest()
    req.open(met, 'http://' + config.ws_host() + '/api/v1' + url)
    req.onerror = (e) => { console.log(e) }
    req.send(data)
}

export default {
    dial(track: number, peer: string) {
        api_request('POST', '/dial?track=' + String(track), peer);
    },

    track_add(type: string) {
        api_request('POST', '/tracks/' + type, null);
    },

    track_del(id: number) {
        api_request('DELETE', '/tracks', String(id));
    },

    audio_device(track: number, mic: number, speaker: number) {
        api_request('PUT', '/audio/device?track=' + String(track), String(mic) + ";" + String(speaker));
    },
}
