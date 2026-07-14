import { FitAddon } from '@xterm/addon-fit';
import { SearchAddon } from '@xterm/addon-search';
import { Terminal } from '@xterm/xterm';
import '@xterm/xterm/css/xterm.css';
import './terminal.css';

const lightTheme = {
  background: '#fbf7ec',
  foreground: '#1f1b14',
  cursor: '#b4322b',
  cursorAccent: '#fbf7ec',
  selectionBackground: '#b4322b55',
  black: '#1f1b14',
  red: '#b4322b',
  green: '#4a7a4a',
  yellow: '#9b6511',
  blue: '#3e5f7a',
  magenta: '#86527a',
  cyan: '#397575',
  white: '#f5ecd1',
  brightBlack: '#6c6555',
  brightRed: '#d94a42',
  brightGreen: '#639764',
  brightYellow: '#b9862f',
  brightBlue: '#5486ac',
  brightMagenta: '#a36b96',
  brightCyan: '#559393',
  brightWhite: '#fffdf7',
};

const darkTheme = {
  background: '#15100a',
  foreground: '#ddd2b6',
  cursor: '#e8534a',
  cursorAccent: '#15100a',
  selectionBackground: '#e8534a55',
  black: '#15100a',
  red: '#e8534a',
  green: '#75a878',
  yellow: '#c89a4b',
  blue: '#5486ac',
  magenta: '#b17aa4',
  cyan: '#67a9a9',
  white: '#ddd2b6',
  brightBlack: '#796f60',
  brightRed: '#f0756e',
  brightGreen: '#91c094',
  brightYellow: '#dfb96f',
  brightBlue: '#75a9ce',
  brightMagenta: '#cc9abf',
  brightCyan: '#88c5c5',
  brightWhite: '#fff8e8',
};

const terminal = new Terminal({
  allowProposedApi: false,
  convertEol: false,
  cursorBlink: true,
  fontFamily: 'ui-monospace, "SFMono-Regular", Consolas, "Liberation Mono", monospace',
  fontSize: 14,
  scrollback: 10_000,
  theme: lightTheme,
});
const fitAddon = new FitAddon();
const searchAddon = new SearchAddon();
terminal.loadAddon(fitAddon);
terminal.loadAddon(searchAddon);
terminal.open(document.getElementById('terminal'));

let bridge;
let pendingInput = [];

function bytesToBase64(bytes) {
  let binary = '';
  for (let offset = 0; offset < bytes.length; offset += 0x8000) {
    binary += String.fromCharCode(...bytes.subarray(offset, offset + 0x8000));
  }
  return btoa(binary);
}

function base64ToBytes(encoded) {
  const binary = atob(encoded);
  return Uint8Array.from(binary, (character) => character.charCodeAt(0));
}

function sendBytes(bytes) {
  const encoded = bytesToBase64(bytes);
  if (bridge) bridge.sendInput(encoded);
  else pendingInput.push(encoded);
}

terminal.onData((data) => sendBytes(new TextEncoder().encode(data)));
terminal.onBinary((data) => sendBytes(Uint8Array.from(data, (character) => character.charCodeAt(0))));
terminal.attachCustomKeyEventHandler((event) => {
  if (event.type !== 'keydown' || !event.ctrlKey || !event.shiftKey) return true;
  if (event.key.toLowerCase() === 'c') {
    bridge?.copySelection(terminal.getSelection());
    return false;
  }
  if (event.key.toLowerCase() === 'v') {
    bridge?.requestPaste();
    return false;
  }
  return true;
});

function fit() {
  fitAddon.fit();
  bridge?.resizeTerminal(terminal.cols, terminal.rows);
}

const resizeObserver = new ResizeObserver(() => requestAnimationFrame(fit));
resizeObserver.observe(document.getElementById('terminal'));

window.zealrnTerminal = {
  clear: () => terminal.clear(),
  focus: () => terminal.focus(),
  getSelection: () => terminal.getSelection(),
  paste: (encoded) => terminal.paste(new TextDecoder().decode(base64ToBytes(encoded))),
  search: (text, backward = false) => backward ? searchAddon.findPrevious(text) : searchAddon.findNext(text),
  setFontSize: (size) => {
    terminal.options.fontSize = Math.max(10, Math.min(28, Number(size) || 14));
    fit();
  },
  setTheme: (theme) => { terminal.options.theme = theme === 'dark' ? darkTheme : lightTheme; },
  write: (encoded) => terminal.write(base64ToBytes(encoded)),
};

new QWebChannel(qt.webChannelTransport, (channel) => {
  bridge = channel.objects.terminalBridge;
  bridge.outputReceived.connect(window.zealrnTerminal.write);
  bridge.themeChanged.connect(window.zealrnTerminal.setTheme);
  bridge.fontSizeChanged.connect(window.zealrnTerminal.setFontSize);
  bridge.clearRequested.connect(window.zealrnTerminal.clear);
  bridge.focusRequested.connect(window.zealrnTerminal.focus);
  bridge.copyRequested.connect(() => bridge.copySelection(terminal.getSelection()));
  bridge.pasteReceived.connect(window.zealrnTerminal.paste);
  bridge.searchRequested.connect(window.zealrnTerminal.search);
  for (const encoded of pendingInput) bridge.sendInput(encoded);
  pendingInput = [];
  fit();
  bridge.frontendReady();
});
