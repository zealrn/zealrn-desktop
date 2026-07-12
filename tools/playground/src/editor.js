import {defaultKeymap, history, historyKeymap, indentWithTab} from "@codemirror/commands";
import {bracketMatching, defaultHighlightStyle, indentOnInput, syntaxHighlighting} from "@codemirror/language";
import {css} from "@codemirror/lang-css";
import {html} from "@codemirror/lang-html";
import {javascript} from "@codemirror/lang-javascript";
import {highlightSelectionMatches, searchKeymap} from "@codemirror/search";
import {Compartment, EditorState} from "@codemirror/state";
import {oneDark} from "@codemirror/theme-one-dark";
import {
  drawSelection,
  EditorView,
  highlightActiveLine,
  highlightActiveLineGutter,
  highlightSpecialChars,
  keymap,
  lineNumbers
} from "@codemirror/view";

const starter = {
  html: `<div class="card">
  <h1>Hello ZealRN</h1>
  <p>Edit the code and click Run.</p>
  <button id="hello">Click me</button>
</div>`,
  css: `body {
  font-family: system-ui, sans-serif;
  padding: 2rem;
}

.card {
  max-width: 32rem;
  padding: 2rem;
  border: 1px solid #8884;
  border-radius: 0.75rem;
}`,
  javascript: `document.querySelector("#hello").addEventListener("click", () => {
  console.log("Hello from ZealRN");
});`
};

const lightTheme = EditorView.theme({
  "&": {height: "100%", backgroundColor: "#fff", color: "#202020"},
  ".cm-content": {fontFamily: "monospace", fontSize: "13px"},
  ".cm-gutters": {backgroundColor: "#f5f5f5", color: "#666", borderRight: "1px solid #ddd"},
  ".cm-activeLine, .cm-activeLineGutter": {backgroundColor: "#eef5ff"}
});

const languages = {html: html(), css: css(), javascript: javascript()};
const names = ["html", "css", "javascript"];
const themes = {};
const views = {};
let bridge = null;
let active = "html";

function extensions(name) {
  themes[name] = new Compartment();
  return [
    lineNumbers(),
    highlightActiveLineGutter(),
    highlightSpecialChars(),
    history(),
    drawSelection(),
    indentOnInput(),
    syntaxHighlighting(defaultHighlightStyle, {fallback: true}),
    bracketMatching(),
    highlightActiveLine(),
    highlightSelectionMatches(),
    keymap.of([...defaultKeymap, ...searchKeymap, ...historyKeymap, indentWithTab]),
    languages[name],
    themes[name].of(lightTheme),
    EditorView.updateListener.of(update => {
      if (update.docChanged && bridge) bridge.contentChanged();
    })
  ];
}

for (const name of names) {
  views[name] = new EditorView({
    state: EditorState.create({doc: starter[name], extensions: extensions(name)}),
    parent: document.getElementById(`editor-${name}`)
  });
}

function setActive(name) {
  if (!names.includes(name)) return;
  active = name;
  for (const editorName of names) {
    document.getElementById(`editor-${editorName}`).hidden = editorName !== name;
  }
  views[name].focus();
}

function documents() {
  return Object.fromEntries(names.map(name => [name, views[name].state.doc.toString()]));
}

function replaceDocuments(content) {
  for (const name of names) {
    const value = typeof content[name] === "string" ? content[name] : "";
    views[name].setState(EditorState.create({doc: value, extensions: extensions(name)}));
  }
  setActive(active);
}

function setDark(dark) {
  for (const name of names) {
    views[name].dispatch({effects: themes[name].reconfigure(dark ? oneDark : lightTheme)});
  }
}

window.zealrnEditor = {documents, replaceDocuments, setActive, setDark, starter: () => ({...starter})};
setActive(active);

new QWebChannel(qt.webChannelTransport, channel => {
  bridge = channel.objects.playgroundBridge;
  bridge.editorReady();
});
