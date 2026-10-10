"use strict";
const catalog = document.querySelector("#catalog");
const count = document.querySelector("#catalog-count");
const empty = document.querySelector("#catalog-empty");
const search = document.querySelector("#mod-search");
let mods = [], kind = "all";

function node(tag, className, text) {
  const element = document.createElement(tag);
  element.className = className;
  if (text !== undefined) element.textContent = text;
  return element;
}
function link(text, url) {
  const element = node("a", "", text);
  const target = new URL(url);
  if (target.protocol !== "https:" || target.hostname !== "github.com") throw new Error("Invalid catalogue link");
  element.href = target.href;
  return element;
}
function render() {
  const query = search.value.trim().toLocaleLowerCase();
  const visible = mods.filter(mod => (kind === "all" || mod.kind === kind) &&
    [mod.name, mod.description, mod.author, ...mod.tags].join(" ").toLocaleLowerCase().includes(query));
  catalog.replaceChildren();
  for (const mod of visible) {
    const card = node("article", "mod-card");
    const art = node("div", "mod-art"); art.setAttribute("aria-hidden", "true");
    art.append(node("span", "", mod.kind === "texture" ? "✦" : "64"));
    const content = node("div", "mod-content");
    content.append(node("div", "mod-meta", mod.kind === "texture" ? "Texture pack" : "Gameplay · DS Lua"),
      node("h3", "", mod.name), node("p", "mod-author", "By " + mod.author), node("p", "", mod.description));
    const tags = node("div", "mod-tags");
    for (const tag of mod.tags) tags.append(node("span", "", tag));
    const links = node("div", "mod-links");
    links.append(link(mod.bundled ? "Included in the preview ↗" : "Get this mod ↗", mod.download), link("Source ↗", mod.source));
    content.append(tags, links); card.append(art, content); catalog.append(card);
  }
  count.textContent = visible.length + (visible.length === 1 ? " approved entry" : " approved entries");
  empty.hidden = visible.length !== 0;
}
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
fetch("catalog.json").then(response => {
  if (!response.ok) throw new Error("Catalogue unavailable");
  return response.json();
}).then(data => { mods = data.mods; render(); }).catch(() => {
  count.textContent = "The catalogue could not load. Please try again or browse the repository on GitHub.";
  catalog.replaceChildren(); empty.hidden = true;
});
