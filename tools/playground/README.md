# Playground assets

Regenerate the committed CodeMirror bundle with:

```bash
cd tools/playground
npm ci
npm run bundle
```

Node.js and npm are developer-only dependencies. ZealRN loads the generated bundle from Qt resources at runtime.
