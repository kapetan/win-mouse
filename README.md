# win-mouse

Mouse tracking for Windows. Receive the screen position of various mouse events. The events are also emitted while another application is in the foreground.

Tested in Windows Vista with node version `0.10`.

	npm install win-mouse

# Usage

The module returns an event emitter instance.

```javascript
var mouse = require('win-mouse')();

mouse.on('move', fuction(x, y) {
	console.log(x, y);
});
```

The program will not terminate as long as a mouse listener is active. To allow the program to exit, either call `mouse.unref` (works as `unref`/`ref` on a TCP server) or `mouse.destroy()`.

The events emitted are: `move`, `left-down`, `left-up`, `left-drag`, `right-up`, `right-down` and `right-drag`. For each event the screen coordinates are passed to the handler function.