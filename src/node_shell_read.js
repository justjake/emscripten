/**
 * @license
 * Copyright 2019 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

requireNodeFS = () => {
  // Use nodePath as the indicator for these not being initialized,
  // since in some environments a global fs may have already been
  // created.
  if (!nodePath) {
    fs = require('fs');
    nodePath = require('path');
    nodeURL = require('url')
  }
};

read_ = function shell_read(filename, binary) {
#if SUPPORT_BASE64_EMBEDDING
  var ret = tryParseAsDataURI(filename);
  if (ret) {
    return binary ? ret : ret.toString();
  }
#endif
  requireNodeFS();
  if (isFileURI(filename)) {
    // For ES6 modules.
    // This will normalize the path, too.
    filename = nodeURL['fileURLToPath'](filename);
  } else {
    filename = nodePath['normalize'](filename);
  }
  return fs.readFileSync(filename, binary ? undefined : 'utf8');
};

readBinary = (filename) => {
  var ret = read_(filename, true);
  if (!ret.buffer) {
    ret = new Uint8Array(ret);
  }
#if ASSERTIONS
  assert(ret.buffer);
#endif
  return ret;
};

readAsync = (filename, onload, onerror) => {
#if SUPPORT_BASE64_EMBEDDING
  var ret = tryParseAsDataURI(filename);
  if (ret) {
    onload(ret);
  }
#endif
  requireNodeFS();
  if (isFileURI(filename)) {
    filename = nodeURL['fileURLToPath'](filename);
  } else {
    filename = nodePath['normalize'](filename);
  }
  fs.readFile(filename, function(err, data) {
    if (err) onerror(err);
    else onload(data.buffer);
  });
};
