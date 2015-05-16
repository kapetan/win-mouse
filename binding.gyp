{
	"targets": [
		{
			"target_name": "addon",
			"sources": ["source/mouse_hook.cc", "source/mouse.cc", "source/addon.cc"],
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			]
		}
	]
}
