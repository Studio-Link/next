module.exports = function () {
  return {
    name: "vitePluginPostCSSComponent",
    
    handleHotUpdate({ server, file }) {
      // Reload index.html if posthtml include changes
      if (file.match(/components\/.*\.html$/)) {
        server.ws.send({
          type: "full-reload",
          event: "index.html",
          data: {},
        });
      }
      return [];
    },
  };
};
