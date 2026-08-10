import { html } from "https://esm.sh/uhtml@4.0";
import Desaparecidos from "./Desaparecidos.js";

// View registry. App.js reads views[0] as the default.
export const views = [
  {
    name: "desaparecidos",
    label: "Desaparecidos",
    component: () => Desaparecidos(),
  },
];