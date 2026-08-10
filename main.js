import { render } from "https://esm.sh/uhtml@4.0";
import { subscribe } from "https://esm.sh/valtio@2.1.3/vanilla";
import { createIcons, icons } from "https://esm.sh/lucide@0.460.0";
import App from "./components/App.js";
import { store } from "./vStore/assets.js";

document.documentElement.setAttribute("data-theme", store.ui.theme);

const root = document.getElementById("app");

function paint() {
  render(root, App());
}

paint();
createIcons({ icons });

// Re-render when UI state changes (view selector, theme, etc.)
subscribe(store.ui, () => {
  paint();
});