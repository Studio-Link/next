export const config = {
    ws_host(): string {
        if (process.env.NODE_ENV == 'production') { 
            return location.host;
        }
        return "127.0.0.1:3333";
    }
}