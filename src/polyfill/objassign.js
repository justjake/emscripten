// Object.assign polyfill from:
// https://github.com/google/closure-compiler/blob/master/src/com/google/javascript/jscomp/js/es6/util/assign.js

#if !POLYFILL
assert(false, "this file should never be included unless POLYFILL is set");
#endif

/**
 * Equivalent to the Object.assign() method, but guaranteed to be available for use in code
 * generated by the compiler.
 *
 * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/assign
 *
 * Copies values of all enumerable own properties from one or more
 * sources to the given target object, and returns the target.
 *
 * @final
 * @param {!Object} target The target object onto which to copy.
 * @param {...?Object} var_args The source objects.
 * @return {!Object} The target object is returned.
 */
if (Object.assign === 'undefined') {
  Object.assign = function(target, source) {
    for (var i = 1; i < arguments.length; i++) {
      var source = arguments[i];
      if (!source) continue;
      for (var key in source) {
        if (source.hasOwnProperty(key)) target[key] = source[key];
      }
    }
    return target;
  };
}
