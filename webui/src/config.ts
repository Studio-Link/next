export const config = {
	ws_host(): string {
		if (process.env.NODE_ENV == 'production') {
			return location.host;
		}
        /* Development */
		return location.hostname + ":9999";
	}
}
