// matrix-entry.js — entry point para el bundle del chat Matrix.
// Importa matrix-js-sdk y lo expone al window. Olm se carga por separado
// (olm.js con WASM inline) porque no se bundlea bien con esbuild.
//
// Se compila con esbuild a un solo archivo IIFE:
//   npm run build   →  public/matrix-bundle.js
import * as matrixcs from "matrix-js-sdk";

window.matrixcs = matrixcs;