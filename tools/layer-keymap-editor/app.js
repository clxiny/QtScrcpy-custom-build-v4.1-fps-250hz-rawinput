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
  addDoubleClickButton: $("#add-double-click-button"),
  addDragButton: $("#add-drag-button"),
  addSteerButton: $("#add-steer-button"),
  placeFpsButton: $("#place-fps-button"),
  nodeList: $("#node-list"),
  nodeCount: $("#node-count"),
  fitButton: $("#fit-button"),
  backgroundImageInput: $("#background-image-input"),
  clearBackgroundButton: $("#clear-background-button"),
  board: $("#board"),
  nodeLayer: $("#node-layer"),
  fileLabel: $("#file-label"),
  backgroundLabel: $("#background-label"),
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
  switchMapHelp: $("#switch-map-help"),
  switchMap: $("#switch-map"),
  deleteNodeButton: $("#delete-node-button"),
  steerSettings: $("#steer-settings"),
  steerUpKey: $("#steer-up-key"),
  steerUpOffset: $("#steer-up-offset"),
  steerLeftKey: $("#steer-left-key"),
  steerLeftOffset: $("#steer-left-offset"),
  steerRightKey: $("#steer-right-key"),
  steerRightOffset: $("#steer-right-offset"),
  steerDownKey: $("#steer-down-key"),
  steerDownOffset: $("#steer-down-offset")
};

const STEER_DIRECTIONS = [
  { name: "up", label: "上", keyField: "upKey", offsetField: "upOffset", keyElement: "steerUpKey", offsetElement: "steerUpOffset" },
  { name: "left", label: "左", keyField: "leftKey", offsetField: "leftOffset", keyElement: "steerLeftKey", offsetElement: "steerLeftOffset" },
  { name: "right", label: "右", keyField: "rightKey", offsetField: "rightOffset", keyElement: "steerRightKey", offsetElement: "steerRightOffset" },
  { name: "down", label: "下", keyField: "downKey", offsetField: "downOffset", keyElement: "steerDownKey", offsetElement: "steerDownOffset" }
];

const NODE_TYPE_LABELS = {
  KMT_CLICK: "点击",
  KMT_CLICK_TWICE: "双击",
  KMT_DRAG: "拖拽",
  KMT_STEER_WHEEL: "方向轮盘"
};

const KEY_CODE_MAP = {
  Backquote: "Key_QuoteLeft",
  Space: "Key_Space",
  Tab: "Key_Tab",
  Escape: "Key_Escape",
  Enter: "Key_Enter",
  Backspace: "Key_Backspace",
  Delete: "Key_Delete",
  Insert: "Key_Insert",
  Home: "Key_Home",
  End: "Key_End",
  PageUp: "Key_PageUp",
  PageDown: "Key_PageDown",
  ArrowUp: "Key_Up",
  ArrowDown: "Key_Down",
  ArrowLeft: "Key_Left",
  ArrowRight: "Key_Right",
  ShiftLeft: "Key_Shift",
  ShiftRight: "Key_Shift",
  ControlLeft: "Key_Control",
  ControlRight: "Key_Control",
  AltLeft: "Key_Alt",
  AltRight: "Key_Alt",
  Equal: "Key_Equal",
  Minus: "Key_Minus",
  BracketLeft: "Key_BracketLeft",
  BracketRight: "Key_BracketRight",
  Backslash: "Key_Backslash",
  Semicolon: "Key_Semicolon",
  Quote: "Key_Quote",
  Comma: "Key_Comma",
  Period: "Key_Period",
  Slash: "Key_Slash"
};

const MOUSE_BUTTON_MAP = {
  0: "LeftButton",
  1: "MiddleButton",
  2: "RightButton",
  3: "BackButton",
  4: "ForwardButton"
};

const state = {
  data: createSample(),
  activeLayer: "base",
  selectedIndex: 0,
  fileName: "分层按键映射.json",
  dragging: null,
  backgroundImageUrl: null,
  bindingCapture: null,
  suppressContextMenu: false,
  selectedFpsOrigin: false,
  placingFpsOrigin: false
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

function keyMapValue(event) {
  if (/^Key[A-Z]$/.test(event.code)) {
    return `Key_${event.code.slice(3)}`;
  }
  if (/^Digit[0-9]$/.test(event.code)) {
    return `Key_${event.code.slice(5)}`;
  }
  if (/^F([1-9]|1[0-2])$/.test(event.code)) {
    return `Key_${event.code}`;
  }
  return KEY_CODE_MAP[event.code] || "";
}

function cancelBindingCapture() {
  const capture = state.bindingCapture;
  if (!capture) {
    return;
  }
  capture.input.classList.remove("capturing");
  capture.input.value = capture.originalValue;
  state.bindingCapture = null;
}

function beginBindingCapture(input) {
  if (input.disabled || !input.dataset.bindingField) {
    return;
  }
  cancelBindingCapture();
  state.bindingCapture = {
    input,
    field: input.dataset.bindingField,
    originalValue: input.value,
    armed: false
  };
  input.classList.add("capturing");
  input.value = "按下按键或鼠标键...";
  input.focus({ preventScroll: true });
  window.setTimeout(() => {
    if (state.bindingCapture && state.bindingCapture.input === input) {
      state.bindingCapture.armed = true;
    }
  }, 0);
}

function applyCapturedBinding(value) {
  const capture = state.bindingCapture;
  const node = selectedNode();
  if (!capture || !node) {
    cancelBindingCapture();
    return;
  }
  node[capture.field] = value;
  capture.input.classList.remove("capturing");
  state.bindingCapture = null;
  render();
}

function captureKeyboardBinding(event) {
  const capture = state.bindingCapture;
  if (!capture || !capture.armed) {
    return;
  }
  event.preventDefault();
  event.stopImmediatePropagation();
  if (event.code === "Escape") {
    cancelBindingCapture();
    setStatus("按键读取已取消");
    return;
  }
  const value = keyMapValue(event);
  if (value) {
    applyCapturedBinding(value);
  }
}

function captureMouseBinding(event) {
  const capture = state.bindingCapture;
  if (!capture || !capture.armed) {
    return;
  }
  const value = MOUSE_BUTTON_MAP[event.button];
  if (!value) {
    return;
  }
  event.preventDefault();
  event.stopImmediatePropagation();
  state.suppressContextMenu = event.button === 2;
  window.setTimeout(() => { state.suppressContextMenu = false; }, 0);
  applyCapturedBinding(value);
}

function steerDirectionDefinition(name) {
  return STEER_DIRECTIONS.find((direction) => direction.name === name);
}

function steerDirectionLimit(node, name) {
  const center = nodePosition(node);
  if (name === "up") {
    return center.y;
  }
  if (name === "left") {
    return center.x;
  }
  if (name === "right") {
    return 1 - center.x;
  }
  return 1 - center.y;
}

function steerDirectionOffset(node, name) {
  const definition = steerDirectionDefinition(name);
  return Math.max(0, validNumber(node[definition.offsetField], 0.075));
}

function steerDirectionPosition(node, name) {
  const center = nodePosition(node);
  const offset = clamp(steerDirectionOffset(node, name), 0, steerDirectionLimit(node, name));
  if (name === "up") {
    return { x: center.x, y: center.y - offset };
  }
  if (name === "left") {
    return { x: center.x - offset, y: center.y };
  }
  if (name === "right") {
    return { x: center.x + offset, y: center.y };
  }
  return { x: center.x, y: center.y + offset };
}

function setSteerDirectionOffset(node, name, value) {
  const definition = steerDirectionDefinition(name);
  const fallback = steerDirectionOffset(node, name);
  node[definition.offsetField] = clamp(validNumber(value, fallback), 0, steerDirectionLimit(node, name));
}

function clearBackgroundImage() {
  if (state.backgroundImageUrl) {
    URL.revokeObjectURL(state.backgroundImageUrl);
  }
  state.backgroundImageUrl = null;
  elements.board.style.removeProperty("background-image");
  elements.backgroundLabel.hidden = true;
  elements.backgroundLabel.textContent = "";
  elements.clearBackgroundButton.disabled = true;
}

function isImageFile(file) {
  return Boolean(file) && (file.type.startsWith("image/") || /\.(png|jpe?g|webp|bmp|gif)$/i.test(file.name));
}

function loadBackgroundImage(file) {
  if (!isImageFile(file)) {
    throw new Error("请选择图片文件");
  }
  clearBackgroundImage();
  state.backgroundImageUrl = URL.createObjectURL(file);
  elements.board.style.backgroundImage = `url("${state.backgroundImageUrl}")`;
  elements.backgroundLabel.textContent = `背景图：${file.name}`;
  elements.backgroundLabel.hidden = false;
  elements.clearBackgroundButton.disabled = false;
  setStatus("背景图已加载，仅用于核对点位");
}

function bindingLabel(node) {
  if (node.type === "KMT_STEER_WHEEL") {
    return "WASD";
  }
  if (node.type === "KMT_DRAG") {
    return node.key || "拖拽";
  }
  return node.key || "?";
}

function pinLabel(node) {
  const binding = bindingLabel(node);
  const displayMap = {
    QuoteLeft: "`",
    Space: "SPACE",
    Control: "CTRL",
    Shift: "SHIFT",
    Alt: "ALT",
    Escape: "ESC",
    Enter: "ENTER",
    Backspace: "BKSP",
    Up: "UP",
    Down: "DOWN",
    Left: "LEFT",
    Right: "RIGHT",
    LeftButton: "LMB",
    RightButton: "RMB",
    MiddleButton: "MMB",
    BackButton: "MB4",
    ForwardButton: "MB5"
  };
  const raw = binding.startsWith("Key_") ? binding.slice(4) : binding;
  if (displayMap[raw]) {
    return displayMap[raw];
  }
  return raw.length > 10 ? `${raw.slice(0, 8)}...` : raw;
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
    setStatus("图层名称只能使用字母、数字、下划线或连字符", true);
    return;
  }
  if (state.data.layers[name]) {
    setStatus("图层名称已存在", true);
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
    setStatus("映射至少需要保留一个图层", true);
    return;
  }
  if (!window.confirm(`确定删除图层“${state.activeLayer}”吗？`)) {
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
      state.selectedFpsOrigin = false;
      state.placingFpsOrigin = false;
      render();
    });
    row.append(button);
    const tag = document.createElement("span");
    tag.className = "layer-tag";
    tag.textContent = layered && state.data.layers[name].parent ? `父级：${state.data.layers[name].parent}` : "";
    row.append(tag);
    elements.layerList.append(row);
  });

  elements.convertButton.disabled = layered;
  elements.addLayerButton.disabled = false;
  elements.newLayerName.disabled = false;
  elements.duplicateLayerButton.disabled = !layered;
  elements.deleteLayerButton.disabled = !layered || names.length <= 1;
  elements.clearBackgroundButton.disabled = !state.backgroundImageUrl;
}

function populateSelect(select, entries, selected, emptyLabel = "无") {
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
    populateSelect(elements.parentLayer, [], "", "旧版映射不可用");
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
  elements.board.style.removeProperty("aspect-ratio");
  elements.nodeLayer.innerHTML = "";
  nodesForActiveLayer().forEach((node, index) => {
    const position = nodePosition(node);
    if (node.type === "KMT_STEER_WHEEL") {
      renderSteerDirections(node, index, position);
    }
    const button = document.createElement("button");
    button.type = "button";
    const typeClass = (node.type || "unknown").toLowerCase().replaceAll("_", "-");
    button.className = `map-node type-${typeClass}${node.type === "KMT_STEER_WHEEL" ? " steer" : ""}${node.type === "KMT_DRAG" ? " drag" : ""}${index === state.selectedIndex ? " selected" : ""}`;
    button.style.left = `${clamp(validNumber(position.x, 0.5)) * 100}%`;
    button.style.top = `${clamp(validNumber(position.y, 0.5)) * 100}%`;
    button.dataset.index = String(index);
    button.textContent = node.type === "KMT_STEER_WHEEL" ? "" : pinLabel(node);
    button.title = `${bindingLabel(node)} (${position.x}, ${position.y})`;
    elements.nodeLayer.append(button);
  });
  renderFpsOrigin();
}

function renderFpsOrigin() {
  const map = getViewMap();
  const position = isObject(map.startPos) ? map.startPos : { x: 0.5, y: 0.5 };
  const button = document.createElement("button");
  button.type = "button";
  button.className = `fps-origin-node${state.selectedFpsOrigin ? " selected" : ""}`;
  button.style.left = `${clamp(validNumber(position.x, 0.5)) * 100}%`;
  button.style.top = `${clamp(validNumber(position.y, 0.5)) * 100}%`;
  button.dataset.fpsOrigin = "true";
  button.textContent = "FPS";
  button.title = `FPS 视角起点 (${position.x}, ${position.y})`;
  elements.nodeLayer.append(button);
}

function setFpsOrigin(x, y) {
  const map = ensureViewMap();
  map.startPos.x = clamp(validNumber(x, map.startPos.x));
  map.startPos.y = clamp(validNumber(y, map.startPos.y));
}

function renderNodeList() {
  const nodes = nodesForActiveLayer();
  elements.nodeList.innerHTML = "";
  elements.nodeCount.textContent = String(nodes.length);
  nodes.forEach((node, index) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `node-list-button${index === state.selectedIndex ? " active" : ""}`;
    button.title = `${NODE_TYPE_LABELS[node.type] || node.type || "未指定类型"}：${bindingLabel(node)}`;

    const nodeIndex = document.createElement("span");
    nodeIndex.className = "node-list-index";
    nodeIndex.textContent = String(index + 1);
    const label = document.createElement("span");
    label.className = "node-list-label";
    label.textContent = bindingLabel(node);
    const type = document.createElement("span");
    type.className = "node-list-type";
    type.textContent = NODE_TYPE_LABELS[node.type] || node.type || "未知";
    button.append(nodeIndex, label, type);
    button.addEventListener("click", () => {
      state.selectedIndex = index;
      state.selectedFpsOrigin = false;
      state.placingFpsOrigin = false;
      render();
    });
    elements.nodeList.append(button);
  });
}

function renderSteerDirections(node, index, center) {
  STEER_DIRECTIONS.forEach((direction) => {
    const position = steerDirectionPosition(node, direction.name);
    const line = document.createElement("div");
    line.className = "steer-guide-line";
    line.dataset.index = String(index);
    line.dataset.steerDirection = direction.name;
    positionSteerGuideLine(line, center, position);
    elements.nodeLayer.append(line);

    const handle = document.createElement("button");
    handle.type = "button";
    handle.className = "map-node steer-direction";
    handle.style.left = `${position.x * 100}%`;
    handle.style.top = `${position.y * 100}%`;
    handle.dataset.index = String(index);
    handle.dataset.steerDirection = direction.name;
    handle.textContent = pinLabel({ key: node[direction.keyField] || "?" });
    handle.title = `${direction.label}方向：${node[direction.keyField] || "未设置"}，偏移 ${steerDirectionOffset(node, direction.name).toFixed(4)}`;
    elements.nodeLayer.append(handle);
  });
}

function positionSteerGuideLine(line, center, position) {
  const horizontal = (position.x - center.x) * 100;
  const vertical = (position.y - center.y) * 100;
  line.style.left = `${center.x * 100}%`;
  line.style.top = `${center.y * 100}%`;
  line.style.width = `${Math.hypot(horizontal, vertical)}%`;
  line.style.transform = `rotate(${Math.atan2(vertical, horizontal) * (180 / Math.PI)}deg)`;
}

function updateSteerDirectionsOnBoard(node, index) {
  const center = nodePosition(node);
  STEER_DIRECTIONS.forEach((direction) => {
    const position = steerDirectionPosition(node, direction.name);
    const selector = `[data-index="${index}"][data-steer-direction="${direction.name}"]`;
    const line = elements.nodeLayer.querySelector(`.steer-guide-line${selector}`);
    const handle = elements.nodeLayer.querySelector(`.steer-direction${selector}`);
    if (line) {
      positionSteerGuideLine(line, center, position);
    }
    if (handle) {
      handle.style.left = `${position.x * 100}%`;
      handle.style.top = `${position.y * 100}%`;
      handle.title = `${direction.label}方向：${node[direction.keyField] || "未设置"}，偏移 ${steerDirectionOffset(node, direction.name).toFixed(4)}`;
    }
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
  populateSelect(elements.switchLayer, isLayered() ? layerNames() : [], node.switchLayer || "", "不切换图层");
  elements.switchLayer.disabled = !isLayered();
  elements.switchMapRow.hidden = node.type !== "KMT_CLICK";
  elements.switchMapHelp.hidden = node.type !== "KMT_CLICK";
  elements.switchMap.checked = Boolean(node.switchMap);
  elements.steerSettings.hidden = !isSteer;
  if (isSteer) {
    STEER_DIRECTIONS.forEach((direction) => {
      elements[direction.keyElement].value = node[direction.keyField] || "";
      elements[direction.offsetElement].value = steerDirectionOffset(node, direction.name);
    });
  }
}

function validateMap() {
  const messages = [];
  if (typeof state.data.switchKey !== "string" || !state.data.switchKey) {
    messages.push({ error: true, text: "字段 switchKey 为必填项" });
  }
  if (isLayered()) {
    const names = layerNames();
    if (!names.includes(state.data.defaultLayer)) {
      messages.push({ error: true, text: "字段 defaultLayer 必须引用已有图层" });
    }
    for (const name of names) {
      const layer = state.data.layers[name];
      if (!Array.isArray(layer.keyMapNodes)) {
        messages.push({ error: true, text: `${name}：字段 keyMapNodes 必须是数组` });
      }
      if (layer.parent && !state.data.layers[layer.parent]) {
        messages.push({ error: true, text: `${name}：父图层不存在` });
      }
      const chain = new Set([name]);
      let parent = layer.parent;
      while (parent) {
        if (chain.has(parent)) {
          messages.push({ error: true, text: `${name}：父图层存在循环引用` });
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
        messages.push({ error: true, text: `${name} #${index + 1}：类型为必填项` });
      }
      if (node.type !== "KMT_STEER_WHEEL" && !node.key) {
        messages.push({ error: true, text: `${name} #${index + 1}：绑定按键为必填项` });
      }
      if (node.type === "KMT_STEER_WHEEL") {
        STEER_DIRECTIONS.forEach((direction) => {
          if (typeof node[direction.keyField] !== "string" || !node[direction.keyField]) {
            messages.push({ error: true, text: `${name} #${index + 1}：${direction.label}方向按键为必填项` });
          }
          const offset = Number(node[direction.offsetField]);
          if (!Number.isFinite(offset) || offset < 0 || offset > 1) {
            messages.push({ error: true, text: `${name} #${index + 1}：${direction.label}方向偏移量无效` });
          }
        });
      }
      if (node.switchLayer && (!isLayered() || !state.data.layers[node.switchLayer])) {
        messages.push({ error: true, text: `${name} #${index + 1}：目标 switchLayer 不存在` });
      }
      const position = nodePosition(node);
      if (!Number.isFinite(Number(position.x)) || !Number.isFinite(Number(position.y))) {
        messages.push({ error: true, text: `${name} #${index + 1}：坐标无效` });
      }
    });
  });
  return messages.length ? messages : [{ error: false, text: "配置有效" }];
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
  elements.validationLabel.textContent = errors.length ? `发现 ${errors.length} 个问题` : "校验通过";
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
  renderNodeList();
  renderBindingForm();
  renderValidation();
  const activeName = isLayered() ? state.activeLayer : "旧版映射";
  elements.activeLayerLabel.textContent = `当前图层：${activeName}`;
  elements.fileLabel.textContent = state.fileName;
  elements.placeFpsButton.classList.toggle("active", state.placingFpsOrigin);
  elements.placeFpsButton.setAttribute("aria-pressed", String(state.placingFpsOrigin));
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

function addNode(type) {
  const nodes = nodesForActiveLayer();
  let node;
  if (type === "KMT_STEER_WHEEL") {
    node = createSteerWheel();
    node.centerPos = { x: 0.5, y: 0.5 };
  } else if (type === "KMT_DRAG") {
    node = {
      type,
      key: "Key_Unknown",
      startPos: { x: 0.5, y: 0.5 },
      endPos: { x: 0.6, y: 0.5 }
    };
  } else {
    node = createClick("Key_Unknown", 0.5, 0.5);
    node.type = type;
  }
  nodes.push(node);
  state.selectedIndex = nodes.length - 1;
  state.selectedFpsOrigin = false;
  state.placingFpsOrigin = false;
  render();
}

function addClick() {
  addNode("KMT_CLICK");
}

function deleteSelectedNode() {
  if (!Number.isInteger(state.selectedIndex)) {
    return;
  }
  if (!window.confirm("确定删除选中的节点吗？")) {
    return;
  }
  nodesForActiveLayer().splice(state.selectedIndex, 1);
  state.selectedIndex = null;
  render();
}

function downloadJson() {
  const body = `${JSON.stringify(state.data, null, 2)}\n`;
  const blob = new Blob([body], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = state.fileName.replace(/\.json$/i, "") + "-已编辑.json";
  link.click();
  URL.revokeObjectURL(url);
}

function loadJson(text, fileName) {
  const parsed = JSON.parse(text);
  if (!isObject(parsed)) {
    throw new Error("JSON 根节点必须是对象");
  }
  state.data = parsed;
  state.fileName = fileName || "按键映射.json";
  state.activeLayer = isObject(parsed.layers) ? (parsed.defaultLayer || Object.keys(parsed.layers)[0]) : "legacy";
  state.selectedIndex = null;
  state.selectedFpsOrigin = false;
  state.placingFpsOrigin = false;
  render();
}

async function loadFile(file) {
  if (!file) {
    return;
  }
  if (isImageFile(file)) {
    loadBackgroundImage(file);
    return;
  }
  loadJson(await file.text(), file.name);
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
    await loadFile(file);
  } catch (error) {
    setStatus(error.message, true);
  } finally {
    elements.fileInput.value = "";
  }
});

elements.backgroundImageInput.addEventListener("change", () => {
  const [file] = elements.backgroundImageInput.files;
  try {
    loadBackgroundImage(file);
  } catch (error) {
    setStatus(error.message, true);
  } finally {
    elements.backgroundImageInput.value = "";
  }
});
elements.clearBackgroundButton.addEventListener("click", () => {
  clearBackgroundImage();
  setStatus("背景图已清除");
});

elements.sampleButton.addEventListener("click", () => {
  state.data = createSample();
  state.activeLayer = "base";
  state.selectedIndex = 0;
  state.selectedFpsOrigin = false;
  state.placingFpsOrigin = false;
  state.fileName = "分层按键映射示例.json";
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
elements.addDoubleClickButton.addEventListener("click", () => addNode("KMT_CLICK_TWICE"));
elements.addDragButton.addEventListener("click", () => addNode("KMT_DRAG"));
elements.addSteerButton.addEventListener("click", () => addNode("KMT_STEER_WHEEL"));
elements.placeFpsButton.addEventListener("click", () => {
  state.placingFpsOrigin = !state.placingFpsOrigin;
  state.selectedFpsOrigin = state.placingFpsOrigin;
  state.selectedIndex = null;
  render();
  if (state.placingFpsOrigin) {
    setStatus("在画布上点击以放置 FPS 起点");
  }
});
elements.fitButton.addEventListener("click", () => elements.board.scrollIntoView({ block: "center", inline: "center", behavior: "smooth" }));

[elements.bindingKey, ...STEER_DIRECTIONS.map((direction) => elements[direction.keyElement])].forEach((input) => {
  input.addEventListener("pointerdown", (event) => {
    event.preventDefault();
    beginBindingCapture(input);
  });
});
document.addEventListener("keydown", captureKeyboardBinding, true);
document.addEventListener("mousedown", (event) => {
  if (event.target.closest && event.target.closest(".key-capture-input")) {
    return;
  }
  captureMouseBinding(event);
}, true);
document.addEventListener("contextmenu", (event) => {
  if (state.bindingCapture || state.suppressContextMenu) {
    event.preventDefault();
    state.suppressContextMenu = false;
  }
}, true);
window.addEventListener("blur", cancelBindingCapture);

elements.nodeLayer.addEventListener("pointerdown", (event) => {
  const target = event.target.closest(".map-node, .fps-origin-node");
  if (!target) {
    return;
  }
  if (target.dataset.fpsOrigin) {
    state.selectedIndex = null;
    state.selectedFpsOrigin = true;
    state.placingFpsOrigin = false;
    state.dragging = { pointerId: event.pointerId, target, fpsOrigin: true };
    target.setPointerCapture(event.pointerId);
    elements.nodeLayer.querySelectorAll(".map-node.selected, .fps-origin-node.selected").forEach((node) => node.classList.remove("selected"));
    target.classList.add("selected");
    renderBindingForm();
    return;
  }
  state.selectedIndex = Number(target.dataset.index);
  state.selectedFpsOrigin = false;
  state.placingFpsOrigin = false;
  state.dragging = { pointerId: event.pointerId, index: state.selectedIndex, target, steerDirection: target.dataset.steerDirection || null };
  target.setPointerCapture(event.pointerId);
  elements.nodeLayer.querySelectorAll(".map-node.selected, .fps-origin-node.selected").forEach((node) => node.classList.remove("selected"));
  target.classList.add("selected");
  renderBindingForm();
});
elements.nodeLayer.addEventListener("pointermove", (event) => {
  if (!state.dragging || state.dragging.pointerId !== event.pointerId) {
    return;
  }
  const rect = elements.board.getBoundingClientRect();
  const x = clamp((event.clientX - rect.left) / rect.width);
  const y = clamp((event.clientY - rect.top) / rect.height);
  if (state.dragging.fpsOrigin) {
    setFpsOrigin(x, y);
    state.dragging.target.style.left = `${x * 100}%`;
    state.dragging.target.style.top = `${y * 100}%`;
    renderViewSettings();
    return;
  }
  const node = nodesForActiveLayer()[state.dragging.index];
  if (state.dragging.steerDirection) {
    const direction = state.dragging.steerDirection;
    const center = nodePosition(node);
    const offset = direction === "up" || direction === "down" ? Math.abs(y - center.y) : Math.abs(x - center.x);
    setSteerDirectionOffset(node, direction, offset);
    updateSteerDirectionsOnBoard(node, state.dragging.index);
  } else {
    setNodePosition(node, x, y);
    const position = nodePosition(node);
    state.dragging.target.style.left = `${position.x * 100}%`;
    state.dragging.target.style.top = `${position.y * 100}%`;
    if (node.type === "KMT_STEER_WHEEL") {
      updateSteerDirectionsOnBoard(node, state.dragging.index);
    }
  }
  renderBindingForm();
});
elements.board.addEventListener("pointerdown", (event) => {
  if (!state.placingFpsOrigin) {
    return;
  }
  const rect = elements.board.getBoundingClientRect();
  setFpsOrigin((event.clientX - rect.left) / rect.width, (event.clientY - rect.top) / rect.height);
  state.placingFpsOrigin = false;
  state.selectedFpsOrigin = true;
  state.selectedIndex = null;
  render();
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
elements.deleteNodeButton.addEventListener("click", deleteSelectedNode);
STEER_DIRECTIONS.forEach((direction) => {
  [elements[direction.keyElement], elements[direction.offsetElement]].forEach((input) => input.addEventListener("change", () => {
    const node = selectedNode();
    if (!node || node.type !== "KMT_STEER_WHEEL") {
      return;
    }
    node[direction.keyField] = elements[direction.keyElement].value.trim();
    setSteerDirectionOffset(node, direction.name, elements[direction.offsetElement].value);
    render();
  }));
});

document.addEventListener("dragover", (event) => event.preventDefault());
document.addEventListener("drop", async (event) => {
  event.preventDefault();
  const [file] = event.dataTransfer.files;
  if (!file) {
    return;
  }
  try {
    await loadFile(file);
  } catch (error) {
    setStatus(error.message, true);
  }
});

render();
