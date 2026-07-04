extends Control

# ------------------------------------------------------------------
# Godot Video Editor - Main Controller
# Uses VideoStreamPlayer (Godot native) + FFmpeg GDExtension for export
# ------------------------------------------------------------------

@onready var video_player: VideoStreamPlayer = $VideoContainer/SubViewportContainer/SubViewport/VideoStreamPlayer
@onready var sub_viewport: SubViewport = $VideoContainer/SubViewportContainer/SubViewport
@onready var sub_viewport_container: SubViewportContainer = $VideoContainer/SubViewportContainer
@onready var effects_overlay: ColorRect = $VideoContainer/EffectsOverlay
@onready var file_dialog: FileDialog = $FileDialog
@onready var export_dialog: FileDialog = $ExportDialog

var encoder: VideoEncoder
var is_exporting: bool = false
var export_frame_timer: float = 0.0
var export_fps: int = 30

# Video editing state
var trim_start: float = 0.0
var trim_end: float = -1.0
var video_duration: float = 0.0
var original_stream: VideoStream = null

func _ready() -> void:
	# Connect UI buttons
	$UI/BottomBar/Controls/ImportBtn.pressed.connect(_on_import_pressed)
	$UI/BottomBar/Controls/PlayBtn.pressed.connect(_on_play_pressed)
	$UI/BottomBar/Controls/TrimBtn.pressed.connect(_on_trim_pressed)
	$UI/BottomBar/Controls/CameraBtn.pressed.connect(_on_camera_pressed)
	$UI/BottomBar/Controls/ExportBtn.pressed.connect(_on_export_pressed)

	# Connect dialogs
	file_dialog.file_selected.connect(_on_video_selected)
	export_dialog.file_selected.connect(_on_export_path_selected)

	# Handle window resize for aspect-fit
	get_viewport().size_changed.connect(_on_window_resized)
	_on_window_resized()

	# Init CameraServer if available
	if CameraServer.get_feed_count() > 0:
		CameraServer.feed_added.connect(_on_camera_feed_added)

func _on_window_resized() -> void:
	"""Maintain TikTok-style 9:16 aspect fit (no stretch)."""
	var screen_size = get_viewport().get_visible_rect().size
	var target_ratio = 9.0 / 16.0
	var screen_ratio = screen_size.x / screen_size.y

	var container_size: Vector2
	if screen_ratio > target_ratio:
		# Screen is wider than 9:16 -> fit to height
		container_size = Vector2(screen_size.y * target_ratio, screen_size.y)
	else:
		# Screen is taller -> fit to width
		container_size = Vector2(screen_size.x, screen_size.x / target_ratio)

	sub_viewport_container.custom_minimum_size = container_size
	sub_viewport_container.size = container_size
	effects_overlay.custom_minimum_size = container_size
	effects_overlay.size = container_size

func _on_import_pressed() -> void:
	file_dialog.popup_centered()

func _on_video_selected(path: String) -> void:
	var stream = load(path)
	if stream is VideoStream:
		original_stream = stream
		video_player.stream = stream
		video_player.play()
		video_player.pause()
		video_duration = video_player.get_stream_length()
		trim_end = video_duration
		print("Loaded video: ", path, " Duration: ", video_duration)
	else:
		push_error("Failed to load video: " + path)

func _on_play_pressed() -> void:
	if video_player.is_playing():
		video_player.pause()
		$UI/BottomBar/Controls/PlayBtn.text = "Play"
	else:
		video_player.play()
		$UI/BottomBar/Controls/PlayBtn.text = "Pause"

func _on_trim_pressed() -> void:
	# Placeholder: In a full implementation, show a trim UI with sliders
	# For now, just print the current playback position
	print("Current position: ", video_player.get_playback_position())
	print("Set trim markers in a real UI. This is a starter scaffold.")

func _on_camera_pressed() -> void:
	# Switch to camera feed if available
	if CameraServer.get_feed_count() == 0:
		push_warning("No camera feed available")
		return

	var feed = CameraServer.get_feed(0)
	# Note: CameraFeed texture can be displayed in a TextureRect
	# For this starter, we just print the available feed
	print("Camera feed: ", feed.get_name())
	push_warning("Camera integration scaffold. Implement TextureRect display.")

func _on_export_pressed() -> void:
	if not original_stream:
		push_warning("No video loaded to export")
		return
	export_dialog.popup_centered()

func _on_export_path_selected(path: String) -> void:
	_start_export(path)

func _start_export(output_path: String) -> void:
	encoder = VideoEncoder.new()
	if not encoder:
		push_error("VideoEncoder GDExtension not loaded")
		return

	# Use viewport resolution for export
	var export_width = sub_viewport.size.x
	var export_height = sub_viewport.size.y
	var bitrate = 4_000_000  # 4 Mbps

	var success = encoder.open(output_path, export_width, export_height, export_fps, bitrate)
	if not success:
		push_error("Failed to open encoder")
		return

	is_exporting = true
	export_frame_timer = 0.0
	video_player.seek(trim_start)
	video_player.play()
	print("Export started to: ", output_path)

func _process(delta: float) -> void:
	if not is_exporting or not encoder or not encoder.is_open():
		return

	export_frame_timer += delta
	var frame_interval = 1.0 / export_fps

	# Capture frames at target FPS
	while export_frame_timer >= frame_interval:
		export_frame_timer -= frame_interval

		# Grab frame from SubViewport
		var tex = sub_viewport.get_texture()
		if tex:
			var img = tex.get_image()
			if img:
				# Ensure RGBA8 format for FFmpeg encoder
				if img.get_format() != Image.FORMAT_RGBA8:
					img.convert(Image.FORMAT_RGBA8)
				encoder.write_frame(img)

	# Check if we've reached trim end
	if video_player.get_playback_position() >= trim_end:
		_finish_export()

func _finish_export() -> void:
	is_exporting = false
	video_player.pause()
	if encoder and encoder.is_open():
		encoder.close()
		print("Export complete!")
	encoder = null

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		if encoder and encoder.is_open():
			encoder.close()
