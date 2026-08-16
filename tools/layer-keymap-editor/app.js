"use strict";

const $ = (selector) => document.querySelector(selector);

const elements = {
  fileInput: $("#file-input"),
  sampleButton: $("#sample-button"),
  exportButton: $("#export-button"),
  convertButton: $("#convert-button"),
  layerList: $("#layer-list"),
  newLayerName: $("#new-layer-name"),
  addLayerButton: $("#add-layer-button"),
  defaultLayer: $("#default-layer"),
  parentLayer: $("#parent-layer"),
  duplicateLayerButton: $("#duplicate-layer-button"),
  deleteLayerButton: $("#delete-layer-button"),
  viewStartX: $("#view-start-x"),
  viewStartY: $("#view-start-y"),
  viewSpeedX: $("#view-speed-x"),
  viewSpeedY: $("#view-speed-y"),
  activeLayerLabel: $("#active-layer-label"),
  addClickButton: $("#add-click-button"),
  fitButton: $("#fit-button"),
  board: $("#board"),
  nodeLayer: $("#node-layer"),
  fileLabel: $("#file-label"),
  validationLabel: $("#validation-label"),
  validationList: $("#validation-list"),
  emptySelection: $("#empty-selection"),
  bindingForm: $("#binding-form"),
  nodeType: $("#node-type"),
  bindingKey: $("#binding-key"),
  positionX: $("#position-x"),
  positionY: $("#position-y"),
  switchLayer: $("#switch-layer"),
  switchMapRow: $("#switch-map-row"),
  switchMap: $("#switch-map")
};

const state = {
  data: createSample(),
  activeLayer: "base",
  selectedIndex: 0,
  fileName: "layer-keymap.json",
  dragging: null
};

function createSample() {
  return {
    switchKey: "Key_QuoteLeft",
    width: 3200,
    height: 2136,
    defaultLayer: "base",
    layers: {
      base: {
        mouseMoveMap: {
          speedRatioX: 1,
          speedRatioY: 1,
          startPos: { x: 0.798, y: 0.694 }
        },
        keyMapNodes: [
          createSteerWheel(),
          createClick("Key_E", 0.855, 0.554, "secondary"),
          createClick("LeftButton", 0.18, 0.287)
        ]
      },
      secondary: {
        mouseMoveMap: {
          speedRatioX: 1,
          speedRatioY: 1,
          startPos: { x: 0.798, y: 0.694 }
        },
        keyMapNodes: [
          createSteerWheel(),
          createClick("Key_E", 0.855, 0.554, "base"),
          createClick("LeftButton", 0.5783, 0.5958),
          createClick("RightButton", 0.5766, 0.7123)
        ]
      }
    }
  };
}

function createSteerWheel() {
  return {
    type: "KMT_STEER_WHEEL",
    centerPos: { x: 0.132, y: 0.804 },
    leftOffset: 0.06314,
    rightOffset: 0.06049,
    upOffset: 0.07891,
    downOffset: 0.07522,
    leftKey: "Key_A",
    rightKey: "Key_D",
    upKey: "Key_W",
    downKey: "Key_S"
  };
}

function createClick(key, x, y, switchLayer = "") {
  const node = {
    type: "KMT_CLICK",
    key,
    pos: { x, y },
    switchMap: false
  };
  if (switchLayer) {
    node.switchLayer = switchLayer;
  }
  return node;
}

function isObject(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

function isLayered() {
  return isObject(state.data.layers);
}

function layerNames() {
  return isLayered() ? Object.keys(state.data.layers) : ["legacy"];
}

function activeContainer() {
  return isLayered() ? state.data.layers[state.activeLayer] : state.data;
}

function nodesForActiveLayer() {
  const container = activeContainer();
  if (!Array.isArray(container.keyMapNodes)) {
    container.keyMapNodes = [];
  }
  return container.keyMapNodes;
}

function selectedNode() {
  const nodes = nodesForActiveLayer();
  return Number.isInteger(state.selectedIndex) && state.selectedIndex >= 0 && state.selectedIndex < nodes.length
    ? nodes[state.selectedIndex]
    : null;
}

function clamp(value, minimum = 0, maximum = 1) {
  return Math.min(maximum, Math.max(minimum, value));
}

function validNumber(value, fallback) {
  const number = Number(value);
  return Number.isFinite(number) ? number : fallback;
}

function positionField(node) {
  if (!node) {
    return null;
  }
  if (node.type === "KMT_STEER_WHEEL") {
    return "centerPos";
  }
  if (node.type === "KMT_DRAG") {
    return "startPos";
  }
  return "pos";
}

function nodePosition(node) {
  const field = positionField(node);
  const position = field ? node[field] : null;
  return isObject(position) ? position : { x: 0.5, y: 0.5 };
}

function setNodePosition(node, x, y) {
  const field = positionField(node);
  if (!isObject(node[field])) {
    node[field] = {};
  }
  node[field].x = clamp(validNumber(x, 0.5));
  node[field].y = clamp(validNumber(y, 0.5));
}

function bindingLabel(node) {
  if (node.type === "KMT_STEER_WHEEL") {
    return "WASD";
  }
  if (node.type === "KMT_DRAG") {
    return node.key || "Drag";
  }
  return node.key || "?";
}

function pinLabel(node) {
  const binding = bindingLabel(node);
  const shortLabels = {
    LeftButton: "L",
    RightButton: "R",
    MiddleButton: "M",
    BackButton: "B",
    ForwardButton: "F"
  };
  if (shortLabels[binding]) {
    return shortLabels[binding];
  }
  if (binding.startsWith("Key_") && binding.length > 4) {
    return binding.slice(4, 8);
  }
  return binding.length > 4 ? binding.slice(0, 4) : binding;
}

function ensureLayered() {
  if (isLayered()) {
    return;
  }
  const { mouseMoveMap, keyMapNodes, defaultLayer, layers, ...root } = state.data;
  state.data = {
    ...root,
    defaultLayer: defaultLayer || "base",
    layers: {
      base: {
        mouseMoveMap: isObject(mouseMoveMap) ? mouseMoveMap : undefined,
        keyMapNodes: Array.isArray(keyMapNodes) ? keyMapNodes : []
      }
    }
  };
  if (!state.data.layers.base.mouseMoveMap) {
    delete state.data.layers.base.mouseMoveMap;
  }
  state.activeLayer = "base";
}

function addLayer() {
  ensureLayered();
  const name = elements.newLayerName.value.trim();
  if (!name || !/^[A-Za-z0-9_-]+$/.test(name)) {
    setStatus("Layer names use letters, numbers, _ or -", true);
    return;
  }
  if (state.data.layers[name]) {
    setStatus("Layer already exists", true);
    return;
  }
  state.data.layers[name] = { keyMapNodes: [] };
  state.activeLayer = name;
  state.selectedIndex = null;
  elements.newLayerName.value = "";
  render();
}

function duplicateActiveLayer() {
  ensureLayered();
  const source = activeContainer();
  let index = 2;
  let name = `${state.activeLayer}-copy`;
  while (state.data.layers[name]) {
    name = `${state.activeLayer}-copy-${index++}`;
  }
  state.data.layers[name] = structuredClone(source);
  delete state.data.layers[name].parent;
  state.activeLayer = name;
  state.selectedIndex = null;
  render();
}

function deleteActiveLayer() {
  if (!isLayered()) {
    return;
  }
  const names = layerNames();
  if (names.length <= 1) {
    setStatus("A map needs at least one layer", true);
    return;
  }
  if (!window.confirm(`Delete layer "${state.activeLayer}"?`)) {
    return;
  }
  const removed = state.activeLayer;
  delete state.data.layers[removed];
  for (const layer of Object.values(state.data.layers)) {
    if (layer.parent === removed) {
      delete layer.parent;
    }
    for (const node of layer.keyMapNodes || []) {
      if (node.switchLayer === removed) {
        delete node.switchLayer;
      }
    }
  }
  state.activeLayer = state.data.defaultLayer === removed ? layerNames()[0] : state.data.defaultLayer;
  state.data.defaultLayer = state.activeLayer;
  state.selectedIndex = null;
  render();
}

function defaultViewMap() {
  return {
    startPos: { x: 0.5, y: 0.5 },
    speedRatioX: 1,
    speedRatioY: 1
  };
}

function getViewMap() {
  let layerName = state.activeLayer;
  const seen = new Set();
  while (true) {
    const container = isLayered() ? state.data.layers[layerName] : state.data;
    if (isObject(container && container.mouseMoveMap)) {
      return container.mouseMoveMap;
    }
    if (!isLayered() || !container || !container.parent || seen.has(layerName)) {
      return defaultViewMap();
    }
    seen.add(layerName);
    layerName = container.parent;
  }
}

function ensureViewMap() {
  const container = activeContainer();
  if (!isObject(container.mouseMoveMap)) {
    container.mouseMoveMap = structuredClone(getViewMap());
  }
  if (!isObject(container.mouseMoveMap.startPos)) {
    container.mouseMoveMap.startPos = { x: 0.5, y: 0.5 };
  }
  return container.mouseMoveMap;
}

function renderLayers() {
  const layered = isLayered();
  const names = layerNames();
  elements.layerList.innerHTML = "";
  names.forEach((name) => {
    const row = document.createElement("div");
    row.className = "layer-row";
    const button = document.createElement("button");
    button.className = `layer-button${name === state.activeLayer || (!layered && name === "legacy") ? " active" : ""}`;
    button.type = "button";
    button.textContent = name;
    button.title = name;
    button.addEventListener("click", () => {
      state.activeLayer = layered ? name : "legacy";
      state.selectedIndex = null;
      render();
    });
    row.append(button);
    const tag = document.createElement("span");
    tag.className = "layer-tag";
    tag.textContent = layered && state.data.layers[name].parent ? `parent: ${state.data.layers[name].parent}` : "";
    row.append(tag);
    elements.layerList.append(row);
  });

  elements.convertButton.disabled = layered;
  elements.addLayerButton.disabled = false;
  elements.newLayerName.disabled = false;
  elements.duplicateLayerButton.disabled = !layered;
  elements.deleteLayerButton.disabled = !layered || names.length <= 1;
}

function populateSelect(select, entries, selected, emptyLabel = "None") {
  select.innerHTML = "";
  if (emptyLabel !== null) {
    const empty = document.createElement("option");
    empty.value = "";
    empty.textContent = emptyLabel;
    select.append(empty);
  }
  entries.forEach((entry) => {
    const option = document.createElement("option");
    option.value = entry;
    option.textContent = entry;
    option.selected = entry === selected;
    select.append(option);
  });
}

function renderLayerSettings() {
  const layered = isLayered();
  const names = layerNames();
  elements.defaultLayer.disabled = !layered;
  elements.parentLayer.disabled = !layered;
  if (!layered) {
    populateSelect(elements.defaultLayer, ["legacy"], "legacy", null);
    populateSelect(elements.parentLayer, [], "", "Not available for legacy maps");
    return;
  }
  populateSelect(elements.defaultLayer, names, state.data.defaultLayer, null);
  populateSelect(elements.parentLayer, names.filter((name) => name !== state.activeLayer), activeContainer().parent || "");
}

function renderViewSettings() {
  const map = getViewMap();
  elements.viewStartX.value = validNumber(map.startPos.x, 0.5);
  elements.viewStartY.value = validNumber(map.startPos.y, 0.5);
  elements.viewSpeedX.value = validNumber(map.speedRatioX, validNumber(map.speedRatio, 1));
  elements.viewSpeedY.value = validNumber(map.speedRatioY, validNumber(map.speedRatio, 1));
}

function renderBoard() {
  const width = validNumber(state.data.width, 3200);
  const height = validNumber(state.data.height, 2136);
  elements.board.style.aspectRatio = `${width} / ${height}`;
  elements.nodeLayer.innerHTML = "";
  nodesForActiveLayer().forEach((node, index) => {
    const position = nodePosition(node);
    const button = document.createElement("button");
    button.type = "button";
    button.className = `map-node${node.type === "KMT_STEER_WHEEL" ? " steer" : ""}${node.type === "KMT_DRAG" ? " drag" : ""}${index === state.selectedIndex ? " selected" : ""}`;
    button.style.left = `${clamp(validNumber(position.x, 0.5)) * 100}%`;
    button.style.top = `${clamp(validNumber(position.y, 0.5)) * 100}%`;
    button.dataset.index = String(index);
    button.textContent = pinLabel(node);
    button.title = `${bindingLabel(node)} (${position.x}, ${position.y})`;
    elements.nodeLayer.append(button);
  });
}

function renderBindingForm() {
  const node = selectedNode();
  const isSteer = node && node.type === "KMT_STEER_WHEEL";
  elements.emptySelection.hidden = Boolean(node);
  elements.bindingForm.hidden = !node;
  if (!node) {
    return;
  }
  const position = nodePosition(node);
  elements.nodeType.value = node.type || "KMT_CLICK";
  elements.bindingKey.value = isSteer ? "WASD" : (node.key || "");
  elements.bindingKey.disabled = isSteer;
  elements.positionX.value = validNumber(position.x, 0.5);
  elements.positionY.value = validNumber(position.y, 0.5);
  populateSelect(elements.switchLayer, isLayered() ? layerNames() : [], node.switchLayer || "", "No layer switch");
  elements.switchLayer.disabled = !isLayered();
  elements.switchMapRow.hidden = node.type !== "KMT_CLICK";
  elements.switchMap.checked = Boolean(node.switchMap);
}

function validateMap() {
  const messages = [];
  if (typeof state.data.switchKey !== "string" || !state.data.switchKey) {
    messages.push({ error: true, text: "switchKey is required" });
  }
  if (isLayered()) {
    const names = layerNames();
    if (!names.includes(state.data.defaultLayer)) {
      messages.push({ error: true, text: "defaultLayer must name an existing layer" });
    }
    for (const name of names) {
      const layer = state.data.layers[name];
      if (!Array.isArray(layer.keyMapNodes)) {
        messages.push({ error: true, text: `${name}: keyMapNodes must be an array` });
      }
      if (layer.parent && !state.data.layers[layer.parent]) {
        messages.push({ error: true, text: `${name}: parent does not exist` });
      }
      const chain = new Set([name]);
      let parent = layer.parent;
      while (parent) {
        if (chain.has(parent)) {
          messages.push({ error: true, text: `${name}: parent cycle` });
          break;
        }
        chain.add(parent);
        parent = state.data.layers[parent] && state.data.layers[parent].parent;
      }
    }
  }
  const contexts = isLayered() ? Object.entries(state.data.layers) : [["legacy", state.data]];
  contexts.forEach(([name, layer]) => {
    (layer.keyMapNodes || []).forEach((node, index) => {
      if (!node.type) {
        messages.push({ error: true, text: `${name} #${index + 1}: type is required` });
      }
      if (node.type !== "KMT_STEER_WHEEL" && !node.key) {
        messages.push({ error: true, text: `${name} #${index + 1}: binding is required` });
      }
      if (node.switchLayer && (!isLayered() || !state.data.layers[node.switchLayer])) {
        messages.push({ error: true, text: `${name} #${index + 1}: switchLayer does not exist` });
      }
      const position = nodePosition(node);
      if (!Number.isFinite(Number(position.x)) || !Number.isFinite(Number(position.y))) {
        messages.push({ error: true, text: `${name} #${index + 1}: position is invalid` });
      }
    });
  });
  return messages.length ? messages : [{ error: false, text: "Configuration is valid" }];
}

function renderValidation() {
  const messages = validateMap();
  const errors = messages.filter((message) => message.error);
  elements.validationList.innerHTML = "";
  messages.forEach((message) => {
    const item = document.createElement("li");
    item.className = message.error ? "error" : "ok";
    item.textContent = message.text;
    elements.validationList.append(item);
  });
  elements.validationLabel.textContent = errors.length ? `${errors.length} issue${errors.length === 1 ? "" : "s"}` : "Valid";
  elements.validationLabel.className = errors.length ? "error" : "ok";
}

function render() {
  if (isLayered() && !state.data.layers[state.activeLayer]) {
    state.activeLayer = state.data.defaultLayer || layerNames()[0];
  }
  renderLayers();
  renderLayerSettings();
  renderViewSettings();
  renderBoard();
  renderBindingForm();
  renderValidation();
  const activeName = isLayered() ? state.activeLayer : "legacy";
  elements.activeLayerLabel.textContent = `Layer: ${activeName}`;
  elements.fileLabel.textContent = state.fileName;
}

function setStatus(message, error = false) {
  elements.validationLabel.textContent = message;
  elements.validationLabel.className = error ? "error" : "ok";
}

function replaceSelectedNode(type) {
  const oldNode = selectedNode();
  if (!oldNode) {
    return;
  }
  const position = nodePosition(oldNode);
  let replacement;
  if (type === "KMT_STEER_WHEEL") {
    replacement = createSteerWheel();
    replacement.centerPos = { ...position };
  } else if (type === "KMT_DRAG") {
    replacement = {
      type: "KMT_DRAG",
      key: oldNode.key || "Key_Unknown",
      startPos: { ...position },
      endPos: { x: clamp(position.x + 0.1), y: position.y }
    };
  } else {
    replacement = createClick(oldNode.key || "Key_Unknown", position.x, position.y, oldNode.switchLayer || "");
    replacement.type = type;
  }
  nodesForActiveLayer()[state.selectedIndex] = replacement;
  render();
}

function addClick() {
  const nodes = nodesForActiveLayer();
  nodes.push(createClick("Key_Unknown", 0.5, 0.5));
  state.selectedIndex = nodes.length - 1;
  render();
}

function downloadJson() {
  const body = `${JSON.stringify(state.data, null, 2)}\n`;
  const blob = new Blob([body], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = state.fileName.replace(/\.json$/i, "") + "-edited.json";
  link.click();
  URL.revokeObjectURL(url);
}

function loadJson(text, fileName) {
  const parsed = JSON.parse(text);
  if (!isObject(parsed)) {
    throw new Error("The JSON root must be an object");
  }
  state.data = parsed;
  state.fileName = fileName || "keymap.json";
  state.activeLayer = isObject(parsed.layers) ? (parsed.defaultLayer || Object.keys(parsed.layers)[0]) : "legacy";
  state.selectedIndex = null;
  render();
}

function updateViewMap() {
  const map = ensureViewMap();
  map.startPos.x = clamp(validNumber(elements.viewStartX.value, map.startPos.x));
  map.startPos.y = clamp(validNumber(elements.viewStartY.value, map.startPos.y));
  map.speedRatioX = Math.max(0.001, validNumber(elements.viewSpeedX.value, map.speedRatioX));
  map.speedRatioY = Math.max(0.001, validNumber(elements.viewSpeedY.value, map.speedRatioY));
  renderBoard();
  renderValidation();
}

elements.fileInput.addEventListener("change", async () => {
  const [file] = elements.fileInput.files;
  if (!file) {
    return;
  }
  try {
    loadJson(await file.text(), file.name);
  } catch (error) {
    setStatus(error.message, true);
  } finally {
    elements.fileInput.value = "";
  }
});

elements.sampleButton.addEventListener("click", () => {
  state.data = createSample();
  state.activeLayer = "base";
  state.selectedIndex = 0;
  state.fileName = "layer-keymap-sample.json";
  render();
});
elements.exportButton.addEventListener("click", downloadJson);
elements.convertButton.addEventListener("click", () => { ensureLayered(); render(); });
elements.addLayerButton.addEventListener("click", addLayer);
elements.newLayerName.addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    event.preventDefault();
    addLayer();
  }
});
elements.duplicateLayerButton.addEventListener("click", duplicateActiveLayer);
elements.deleteLayerButton.addEventListener("click", deleteActiveLayer);
elements.defaultLayer.addEventListener("change", () => {
  if (isLayered()) {
    state.data.defaultLayer = elements.defaultLayer.value;
    renderValidation();
  }
});
elements.parentLayer.addEventListener("change", () => {
  if (!isLayered()) {
    return;
  }
  const layer = activeContainer();
  if (elements.parentLayer.value) {
    layer.parent = elements.parentLayer.value;
  } else {
    delete layer.parent;
  }
  render();
});
[elements.viewStartX, elements.viewStartY, elements.viewSpeedX, elements.viewSpeedY].forEach((input) => {
  input.addEventListener("change", updateViewMap);
});
elements.addClickButton.addEventListener("click", addClick);
elements.fitButton.addEventListener("click", () => elements.board.scrollIntoView({ block: "center", inline: "center", behavior: "smooth" }));

elements.nodeLayer.addEventListener("pointerdown", (event) => {
  const target = event.target.closest(".map-node");
  if (!target) {
    return;
  }
  state.selectedIndex = Number(target.dataset.index);
  state.dragging = { pointerId: event.pointerId, index: state.selectedIndex, target };
  target.setPointerCapture(event.pointerId);
  elements.nodeLayer.querySelectorAll(".map-node.selected").forEach((node) => node.classList.remove("selected"));
  target.classList.add("selected");
  renderBindingForm();
});
elements.nodeLayer.addEventListener("pointermove", (event) => {
  if (!state.dragging || state.dragging.pointerId !== event.pointerId) {
    return;
  }
  const rect = elements.board.getBoundingClientRect();
  const node = nodesForActiveLayer()[state.dragging.index];
  setNodePosition(node, (event.clientX - rect.left) / rect.width, (event.clientY - rect.top) / rect.height);
  const position = nodePosition(node);
  state.dragging.target.style.left = `${position.x * 100}%`;
  state.dragging.target.style.top = `${position.y * 100}%`;
  renderBindingForm();
});
elements.nodeLayer.addEventListener("pointerup", (event) => {
  if (state.dragging && state.dragging.pointerId === event.pointerId) {
    state.dragging = null;
    renderBoard();
    renderValidation();
  }
});
elements.nodeLayer.addEventListener("pointercancel", () => {
  state.dragging = null;
  renderBoard();
  renderValidation();
});

elements.nodeType.addEventListener("change", () => replaceSelectedNode(elements.nodeType.value));
elements.bindingKey.addEventListener("change", () => {
  const node = selectedNode();
  if (node && node.type !== "KMT_STEER_WHEEL") {
    node.key = elements.bindingKey.value.trim();
    render();
  }
});
[elements.positionX, elements.positionY].forEach((input) => input.addEventListener("change", () => {
  const node = selectedNode();
  if (node) {
    setNodePosition(node, elements.positionX.value, elements.positionY.value);
    render();
  }
}));
elements.switchLayer.addEventListener("change", () => {
  const node = selectedNode();
  if (!node) {
    return;
  }
  if (elements.switchLayer.value) {
    node.switchLayer = elements.switchLayer.value;
  } else {
    delete node.switchLayer;
  }
  render();
});
elements.switchMap.addEventListener("change", () => {
  const node = selectedNode();
  if (node && node.type === "KMT_CLICK") {
    node.switchMap = elements.switchMap.checked;
    renderValidation();
  }
});

document.addEventListener("dragover", (event) => event.preventDefault());
document.addEventListener("drop", async (event) => {
  event.preventDefault();
  const [file] = event.dataTransfer.files;
  if (!file) {
    return;
  }
  try {
    loadJson(await file.text(), file.name);
  } catch (error) {
    setStatus(error.message, true);
  }
});

render();
