var events = require('events');
var bindings = require('bindings');
var Mouse = bindings('addon').Mouse;

module.exports = function() {
	var that = new events.EventEmitter();
	var mouse = null;
	var left = false;
	var right = false;

	that.once('newListener', function() {
		mouse = new Mouse(function(type, x, y) {
			if(type === 'left-down') left = true;
			else if(type === 'left-up') left = false;
			else if(type === 'right-down') right = true;
			else if(type === 'right-up') right = false;

			if(type === 'move' && left) type = 'left-drag';
			else if(type === 'move' && right) type = 'right-drag';

			that.emit(type, x, y);
		});
	});

	that.ref = function() {
		if(mouse) mouse.ref();
	};

	that.unref = function() {
		if(mouse) mouse.unref();
	};

	that.destroy = function() {
		if(mouse) mouse.destroy();
		mouse = null;
	};

	return that;
};
