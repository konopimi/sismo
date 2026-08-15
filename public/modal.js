// modal.js — reusable modal shell.
// Loaded before map.js and index.js. Defines window.Modal, a factory
// that wraps a .modal element and provides open/close with focus
// management, scroll-lock, backdrop-click, Escape, and a basic Tab trap.
// Slots (header/body/footer) are opt-in via data-modal-* attributes;
// they are null until the HTML is refactored in later stages.
(function () {
  "use strict";

  // Stack of all created instances; used for Escape dispatch and
  // scroll-lock refcounting.
  const instances = [];
  let escapeWired = false;

  function wireEscape() {
    if (escapeWired) return;
    escapeWired = true;
    document.addEventListener("keydown", function (e) {
      if (e.key !== "Escape") return;
      // Close the topmost open modal (most recently pushed).
      for (let i = instances.length - 1; i >= 0; i--) {
        if (instances[i].isOpen()) {
          instances[i].close();
          break;
        }
      }
    });
  }

  function lockScroll() {
    document.body.style.overflow = "hidden";
  }

  function unlockScroll() {
    // Only release the scroll lock when no modal remains open.
    if (!instances.some(function (m) { return m.isOpen(); })) {
      document.body.style.overflow = "";
    }
  }

  function focusableEls(root) {
    return root.querySelectorAll(
      'button:not([disabled]), [href], input, select, textarea, ' +
      '[tabindex]:not([tabindex="-1"])'
    );
  }

  /**
   * @param {Object} opts
   * @param {string} opts.id            — element id of the .modal wrapper
   * @param {Function} [opts.onOpen]   — called after opening
   * @param {Function} [opts.onClose]   — called after closing
   * @returns {{el, header, body, footer, open, close, isOpen}|null}
   */
  function Modal(opts) {
    var el = document.getElementById(opts.id);
    if (!el) {
      console.warn("Modal: #" + opts.id + " not found");
      return null;
    }

    var header = el.querySelector("[data-modal-header]");
    var body = el.querySelector("[data-modal-body]");
    var footer = el.querySelector("[data-modal-footer]");
    var closeBtn = el.querySelector("[data-modal-close]");
    var onOpen = opts.onOpen || null;
    var onClose = opts.onClose || null;
    var previousFocus = null;

    wireEscape();

    var api = {
      el: el,
      header: header,
      body: body,
      footer: footer,
      previousFocus: null,
      isOpen: isOpen,
      open: open,
      close: close
    };
    instances.push(api);

    function isOpen() {
      return el.classList.contains("open");
    }

    function open() {
      previousFocus = document.activeElement;
      el.classList.add("open");
      lockScroll();
      // Move focus into the modal for keyboard users.
      var f = focusableEls(el);
      if (f.length) f[0].focus();
      if (onOpen) onOpen();
    }

    function close() {
      if (!isOpen()) return;
      el.classList.remove("open");
      unlockScroll();
      if (previousFocus && typeof previousFocus.focus === "function") {
        previousFocus.focus();
        previousFocus = null;
      }
      if (onClose) onClose();
    }

    // Backdrop click: close only when the click lands on the wrapper
    // itself (not an inner element).
    el.addEventListener("click", function (e) {
      if (e.target === el) close();
    });

    // Explicit close button.
    if (closeBtn) closeBtn.addEventListener("click", close);

    // Basic focus trap: keep Tab cycling inside the modal while open.
    el.addEventListener("keydown", function (e) {
      if (e.key !== "Tab" || !isOpen()) return;
      var f = focusableEls(el);
      if (!f.length) return;
      var first = f[0];
      var last = f[f.length - 1];
      if (e.shiftKey && document.activeElement === first) {
        e.preventDefault();
        last.focus();
      } else if (!e.shiftKey && document.activeElement === last) {
        e.preventDefault();
        first.focus();
      }
    });

    return api;
  }

  window.Modal = Modal;
})();