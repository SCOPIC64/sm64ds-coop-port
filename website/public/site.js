"use strict";
try { if (localStorage.getItem("sm64ds-site-theme") === "light") document.documentElement.dataset.theme = "light"; } catch (_) {}
for (const button of document.querySelectorAll(".theme-toggle")) {
  button.addEventListener("click", () => {
    const theme = document.documentElement.dataset.theme === "light" ? "dark" : "light";
    document.documentElement.dataset.theme = theme;
    try { localStorage.setItem("sm64ds-site-theme", theme); } catch (_) {}
  });
}
const catalog = document.querySelector("#catalog");
const count = document.querySelector("#catalog-count");
const empty = document.querySelector("#catalog-empty");
const search = document.querySelector("#mod-search");
let mods = [], kind = "all";
function node(tag, className, text) {
  const element = document.createElement(tag); element.className = className;
  if (text !== undefined) element.textContent = text; return element;
}
function link(text, url) {
  const target = new URL(url);
  if (target.protocol !== "https:" || target.hostname !== "github.com") throw new Error("Invalid catalogue link");
  const element = node("a", "", text); element.href = target.href; return element;
}
function render() {
  const query = search.value.trim().toLocaleLowerCase();
  const visible = mods.filter(mod => (kind === "all" || mod.kind === kind) && [mod.name, mod.description, mod.author, ...mod.tags].join(" ").toLocaleLowerCase().includes(query));
  catalog.replaceChildren();
  for (const mod of visible) {
    const row = node("article", "mod-row");
    const icon = node("div", "mod-icon", mod.kind === "texture" ? "TX" : "64"); icon.setAttribute("aria-hidden", "true");
    const content = node("div", "mod-content"), heading = node("div", "mod-heading");
    heading.append(node("h2", "", mod.name), node("span", "mod-version", mod.version));
    content.append(heading, node("p", "mod-author", "By " + mod.author + " · " + (mod.kind === "texture" ? "Texture packs" : "Gameplay")), node("p", "mod-description", mod.description), node("p", "mod-tags", mod.tags.join(" · ")));
    const links = node("div", "mod-links"), download = link(mod.bundled ? "Included in the preview" : "Download", mod.download); download.className = "button small";
    links.append(download, link("Source", mod.source));
    const details = node("details", ""), install = node("p", "mod-install", "Extract into ");
    install.append(node("code", "", mod.install)); details.append(node("summary", "", "Installation"), install);
    content.append(links, details); row.append(icon, content); catalog.append(row);
  }
  count.textContent = visible.length + (visible.length === 1 ? " approved mod" : " approved mods");
  empty.hidden = visible.length !== 0; catalog.hidden = visible.length === 0;
}
if (catalog) {
  for (const button of document.querySelectorAll(".filter")) {
    button.addEventListener("click", () => {
      kind = button.dataset.kind;
      for (const other of document.querySelectorAll(".filter")) {
        const selected = other === button;
        other.classList.toggle("active", selected); other.setAttribute("aria-pressed", String(selected));
      }
      render();
    });
  }
  search.addEventListener("input", render);
  fetch("catalog.json").then(response => { if (!response.ok) throw new Error("Catalogue unavailable"); return response.json(); }).then(data => {
    mods = data.mods;
    for (const counter of document.querySelectorAll("[data-count]")) counter.textContent = String(mods.filter(mod => counter.dataset.count === "all" || counter.dataset.count === mod.kind).length);
    render();
  }).catch(() => { count.textContent = "Mods could not load. Please refresh or browse the repository on GitHub."; catalog.replaceChildren(); empty.hidden = true; });
}
