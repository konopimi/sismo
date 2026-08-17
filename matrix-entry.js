// matrix-entry.js — entry point para el bundle del chat Matrix.
// Importa matrix-js-sdk y Olm, y los expone al window para que el
// frontend vanilla (index.js) los use sin bundler.
//
// Se compila con esbuild a un solo archivo IIFE:
//   npm run build   →  public/matrix-bundle.js
import * as matrixcs from "matrix-js-sdk";
import Olm from "@matrix-org/olm";

window.matrixcs = matrixcs;
window.Olm = Olm;