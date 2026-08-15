/**
 * GIT:konopimi IG:neokpm
 * virte.js — minimal windowed virtualization for long HTML lists.
 *
 * Keeps only the DOM nodes near the viewport rendered. Two spacer
 * elements (top/bottom) simulate the height of everything that's
 * currently scrolled out of view. Item heights are measured per node
 * (so variable-height cards, e.g. with/without a photo, work fine) and
 * refined as they render or as their images load.
 *
 * Usage:
 *   import { VirtualList } from "./virtual-list.js";
 *
 *   const vlist = new VirtualList(containerEl, {
 *     overscan: 6,          // extra items rendered beyond the viewport edge
 *     estimatedHeight: 160, // initial guess before anything is measured
 *   });
 *
 *   vlist.setItems(items, (item) => `<div class="card">...</div>`);
 *
 *   // later, e.g. after a search filter changes the array:
 *   vlist.setItems(filteredItems, cardHtmlFn);
 *
 *   // if the container becomes visible again after being display:none
 *   // (e.g. a tab switch), force a recalculation:
 *   vlist.refresh();
 *
 *   // cleanup if the container is ever removed from the DOM:
 *   vlist.destroy();
 */
export class VirtualList {
  /**
   * @param {HTMLElement} containerEl - element that will hold the list.
   *   Its content is fully managed by VirtualList once constructed.
   * @param {Object} [options]
   * @param {number} [options.overscan=6] - extra items rendered past each viewport edge.
   * @param {number} [options.estimatedHeight=160] - initial per-item height guess (px).
   */
  constructor(containerEl, options = {}) {
    this.containerEl = containerEl;
    this.overscan = options.overscan ?? 6;
    this.avgHeight = options.estimatedHeight ?? 160;
    this.items = [];
    this.cardHtmlFn = null;
    this.heights = [];
    this.start = 0;
    this.end = 0;
    this.nodes = new Map(); // index -> element
    this.scrollParent = null;
    this._onScroll = null;
    this._rafScheduled = false;
    this._destroyed = false;
    this.containerEl.style.position = "relative";
    this.containerEl.innerHTML = "";
    this.topSpacer = document.createElement("div");
    this.bottomSpacer = document.createElement("div");
    this.containerEl.appendChild(this.topSpacer);
    this.containerEl.appendChild(this.bottomSpacer);
  }
  /**
   * Replace the items and/or the card renderer. Safe to call repeatedly
   * (e.g. every time a search filter changes the array) — if the array
   * reference differs from the last call, previously rendered nodes are
   * discarded and heights are reset to the last known estimates.
   *
   * @param {Array} items
   * @param {(item: any, index: number) => string} cardHtmlFn - returns
   *   HTML for a single item; must produce exactly one root element.
   */
  setItems(items, cardHtmlFn) {
    if (this._destroyed) return;
    if (this.items !== items) {
      this.nodes.forEach((node) => node.remove());
      this.nodes.clear();
      const oldHeights = this.heights;
      this.heights = items.map((_, i) => oldHeights[i] || this.avgHeight);
      this.start = 0;
      this.end = 0;
    }
    this.items = items;
    this.cardHtmlFn = cardHtmlFn;
    if (!this.scrollParent) {
      this.scrollParent = VirtualList._getScrollParent(this.containerEl);
      this._onScroll = () => this._scheduleUpdate();
      this.scrollParent.addEventListener("scroll", this._onScroll, { passive: true });
      window.addEventListener("resize", this._onScroll, { passive: true });
    }
    this._update(true);
  }
  /** Force a full recalculation — call after the container becomes
   * visible again (e.g. a tab switch away from display:none). */
  refresh() {
    if (this._destroyed) return;
    this._update(true);
  }
  /** Detach listeners and clear rendered nodes. */
  destroy() {
    if (this._destroyed) return;
    this._destroyed = true;
    if (this.scrollParent && this._onScroll) {
      this.scrollParent.removeEventListener("scroll", this._onScroll);
      window.removeEventListener("resize", this._onScroll);
    }
    this.nodes.forEach((node) => node.remove());
    this.nodes.clear();
  }
  // ---- internals ----
  static _getScrollParent(el) {
    let node = el.parentElement;
    while (node) {
      const style = getComputedStyle(node);
      if (/(auto|scroll)/.test(style.overflowY) && node.scrollHeight > node.clientHeight) {
        return node;
      }
      node = node.parentElement;
    }
    return document.scrollingElement || document.documentElement;
  }
  _getContainerOffsetTop(isWindowScroll) {
    if (isWindowScroll) {
      return this.containerEl.getBoundingClientRect().top + window.scrollY;
    }
    let top = 0;
    let node = this.containerEl;
    while (node && node !== this.scrollParent) {
      top += node.offsetTop;
      node = node.offsetParent;
    }
    return top;
  }
  _scheduleUpdate() {
    if (this._rafScheduled) return;
    this._rafScheduled = true;
    requestAnimationFrame(() => {
      this._rafScheduled = false;
      this._update(false);
    });
  }
  _update(force) {
    if (this._destroyed) return;
    const items = this.items;
    if (!items.length) {
      this.topSpacer.style.height = "0px";
      this.bottomSpacer.style.height = "0px";
      this.nodes.forEach((node) => node.remove());
      this.nodes.clear();
      this.start = 0;
      this.end = 0;
      return;
    }
    const scrollParent = this.scrollParent;
    const isWindowScroll = scrollParent === document.scrollingElement || scrollParent === document.documentElement;
    const viewportTop = isWindowScroll ? window.scrollY : scrollParent.scrollTop;
    const viewportHeight = isWindowScroll ? window.innerHeight : scrollParent.clientHeight;
    const containerTop = this._getContainerOffsetTop(isWindowScroll);
    const relativeScroll = Math.max(0, viewportTop - containerTop);
    const relativeBottom = relativeScroll + viewportHeight;
    const heights = this.heights;
    let acc = 0;
    let start = 0;
    for (; start < heights.length; start++) {
      if (acc + heights[start] > relativeScroll) break;
      acc += heights[start];
    }
    let end = start;
    let visAcc = acc;
    for (; end < heights.length; end++) {
      if (visAcc > relativeBottom) break;
      visAcc += heights[end];
    }
    start = Math.max(0, start - this.overscan);
    end = Math.min(heights.length, end + this.overscan);
    if (!force && start === this.start && end === this.end) return;
    let topSpacerHeight = 0;
    for (let i = 0; i < start; i++) topSpacerHeight += heights[i];
    let bottomSpacerHeight = 0;
    for (let i = end; i < heights.length; i++) bottomSpacerHeight += heights[i];
    this.topSpacer.style.height = topSpacerHeight + "px";
    this.bottomSpacer.style.height = bottomSpacerHeight + "px";
    this.nodes.forEach((node, idx) => {
      if (idx < start || idx >= end) {
        node.remove();
        this.nodes.delete(idx);
      }
    });
    for (let i = start; i < end; i++) {
      if (!this.nodes.has(i) && items[i] !== undefined) {
        const wrapper = document.createElement("div");
        wrapper.innerHTML = this.cardHtmlFn(items[i], i).trim();
        const node = wrapper.firstElementChild;
        if (node) {
          node.dataset.vIndex = i;
          this.nodes.set(i, node);
          node.querySelectorAll("img").forEach((img) => {
            if (!img.complete) {
              const bump = () => this._scheduleUpdate();
              img.addEventListener("load", bump, { once: true });
              img.addEventListener("error", bump, { once: true });
            }
          });
        }
      }
    }
    const orderedIndices = Array.from(this.nodes.keys()).sort((a, b) => a - b);
    orderedIndices.forEach((idx) => {
      this.containerEl.insertBefore(this.nodes.get(idx), this.bottomSpacer);
    });
    this.start = start;
    this.end = end;
    requestAnimationFrame(() => {
      if (this._destroyed) return;
      let changed = false;
      let total = 0;
      let count = 0;
      this.nodes.forEach((node, idx) => {
        const h = node.offsetHeight;
        if (h > 0) {
          if (Math.abs((this.heights[idx] || 0) - h) > 1) {
            this.heights[idx] = h;
            changed = true;
          }
          total += h;
          count++;
        }
      });
      if (count) this.avgHeight = total / count;
      if (changed) this._update(true);
    });
  }
}
