/* eslint-disable no-undef */
module.exports = {
  purge: ["./index.html", './src/**/*.{vue,js,ts,jsx,tsx}'],
  darkMode: false, // or 'media' or 'class'
  theme: {
    container: {
      screens: {
        sm: "100%",
        md: "100%",
        lg: "1024px",
        xl: "1280px",
      },
    },
    fontFamily: {
      sans: ['"Roboto Mono"'],
    },
    extend: {
      colors: {
        sl: {
          primary: "#de7b00",
          primary_h: "#ff8d00",
          surface: "#121212",
          disabled: "#6c6c6c",
          on_surface_1: "#e0e0e0",
          on_surface_2: "#a0a0a0",
          "01dpa": "rgba(255, 255, 255, 0.05)",
          "02dpa": "rgba(255, 255, 255, 0.07)",
          "03dpa": "rgba(255, 255, 255, 0.08)",
          "04dpa": "rgba(255, 255, 255, 0.09)",
          "06dpa": "rgba(255, 255, 255, 0.11)",
          "08dpa": "rgba(255, 255, 255, 0.12)",
          "12dpa": "rgba(255, 255, 255, 0.14)",
          "16dpa": "rgba(255, 255, 255, 0.15)",
          "24dpa": "rgba(255, 255, 255, 0.16)",
          "01dp": "#1e1e1e",
          "02dp": "#232323",
          "03dp": "#252525",
          "04dp": "#272727",
          "06dp": "#2c2c2c",
          "08dp": "#2f2f2f",
          "12dp": "#333",
          "16dp": "#353535",
          "24dp": "#383838",
        },
      },
    },
  },
  plugins: [
    require("@tailwindcss/forms"),
    require("@tailwindcss/typography"),
    require("@tailwindcss/aspect-ratio"),
  ],
};
