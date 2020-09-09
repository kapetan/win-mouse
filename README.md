# win-mouse

[![Build status](https://ci.appveyor.com/api/projects/status/wj9wy01q1ufhmfns?svg=true)](https://ci.appveyor.com/project/kapetan/win-mouse)

Mouse tracking for Windows. Receive the screen position of various mouse events. The events are also emitted while another application is in the foreground.

Versions of this library prior to version 2.0.0 also run with Node.js version 9 and below. Version 2.0.0 and above are context-aware.

	npm install win-mouse

# Usage

The module returns an event emitter instance.

```javascript
var mouse = require('win-mouse')()

mouse.on('move', function(x, y) {
	console.log(x, y)
})
```

The program will not terminate as long as a mouse listener is active. To allow the program to exit, either call `mouse.unref` (works as `unref`/`ref` on a TCP server) or `mouse.destroy()`.

The events emitted are: `move`, `left-down`, `left-up`, `left-drag`, `right-up`, `right-down` and `right-drag`. For each event the screen coordinates are passed to the handler function.
