'use strict';

const RP_INIT_CONNECTION = 1;
const RP_UPDATE_DISPLAY_MODE = 2;
const RP_CLOSE_CONNECTION = 3;
const RP_GET_SYSTEM_PALETTE = 4;
const RP_GET_SYSTEM_PALETTE_RESULT = 5;

const RP_CREATE_STATE = 20;
const RP_DELETE_STATE = 21;
const RP_ENABLE_SYNC_DRAWING = 22;
const RP_DISABLE_SYNC_DRAWING = 23;
const RP_INVALIDATE_RECT = 24;
const RP_INVALIDATE_REGION = 25;

const RP_SET_OFFSETS = 40;
const RP_SET_HIGH_COLOR = 41;
const RP_SET_LOW_COLOR = 42;
const RP_SET_PEN_SIZE = 43;
const RP_SET_STROKE_MODE = 44;
const RP_SET_BLENDING_MODE = 45;
const RP_SET_PATTERN = 46;
const RP_SET_DRAWING_MODE = 47;
const RP_SET_FONT = 48;
const RP_SET_TRANSFORM = 49;

const RP_CONSTRAIN_CLIPPING_REGION = 60;
const RP_COPY_RECT_NO_CLIPPING = 61;
const RP_INVERT_RECT = 62;
const RP_DRAW_BITMAP = 63;
const RP_DRAW_BITMAP_RECTS = 64;

const RP_STROKE_ARC = 80;
const RP_STROKE_BEZIER = 81;
const RP_STROKE_ELLIPSE = 82;
const RP_STROKE_POLYGON = 83;
const RP_STROKE_RECT = 84;
const RP_STROKE_ROUND_RECT = 85;
const RP_STROKE_SHAPE = 86;
const RP_STROKE_TRIANGLE = 87;
const RP_STROKE_LINE = 88;
const RP_STROKE_LINE_ARRAY = 89;

const RP_FILL_ARC = 100;
const RP_FILL_BEZIER = 101;
const RP_FILL_ELLIPSE = 102;
const RP_FILL_POLYGON = 103;
const RP_FILL_RECT = 104;
const RP_FILL_ROUND_RECT = 105;
const RP_FILL_SHAPE = 106;
const RP_FILL_TRIANGLE = 107;
const RP_FILL_REGION = 108;

const RP_FILL_ARC_GRADIENT = 120;
const RP_FILL_BEZIER_GRADIENT = 121;
const RP_FILL_ELLIPSE_GRADIENT = 122;
const RP_FILL_POLYGON_GRADIENT = 123;
const RP_FILL_RECT_GRADIENT = 124;
const RP_FILL_ROUND_RECT_GRADIENT = 125;
const RP_FILL_SHAPE_GRADIENT = 126;
const RP_FILL_TRIANGLE_GRADIENT = 127;
const RP_FILL_REGION_GRADIENT = 128;

const RP_STROKE_POINT_COLOR = 140;
const RP_STROKE_LINE_1PX_COLOR = 141;
const RP_STROKE_RECT_1PX_COLOR = 142;

const RP_FILL_RECT_COLOR = 160;
const RP_FILL_REGION_COLOR_NO_CLIPPING = 161;

const RP_DRAW_STRING = 180;
const RP_DRAW_STRING_WITH_OFFSETS = 181;
const RP_DRAW_STRING_RESULT = 182;
const RP_STRING_WIDTH = 183;
const RP_STRING_WIDTH_RESULT = 184;
const RP_READ_BITMAP = 185;
const RP_READ_BITMAP_RESULT = 186;

const RP_SET_CURSOR = 200;
const RP_SET_CURSOR_VISIBLE = 201;
const RP_MOVE_CURSOR_TO = 202;

const RP_MOUSE_MOVED = 220;
const RP_MOUSE_DOWN = 221;
const RP_MOUSE_UP = 222;
const RP_MOUSE_WHEEL_CHANGED = 223;

const RP_KEY_DOWN = 240;
const RP_KEY_UP = 241;
const RP_UNMAPPED_KEY_DOWN = 242;
const RP_UNMAPPED_KEY_UP = 243;
const RP_MODIFIERS_CHANGED = 244;


// drawing_mode
const B_OP_COPY = 0;
const B_OP_OVER = 1;
const B_OP_ERASE = 2;
const B_OP_INVERT = 3;
const B_OP_ADD = 4;
const B_OP_SUBTRACT = 5;
const B_OP_BLEND = 6;
const B_OP_MIN = 7;
const B_OP_MAX = 8;
const B_OP_SELECT = 9;
const B_OP_ALPHA = 10;


// color_space
const B_NO_COLOR_SPACE = 0x0000;
const B_RGB32 = 0x0008;			// BGR- 8:8:8:8
const B_RGBA32 = 0x2008;		// BGRA	8:8:8:8
const B_RGB24 = 0x0003;			// BGR	8:8:8
const B_RGB16 = 0x0005;			// BGR	5:6:5
const B_RGB15 = 0x0010;			// BGR- 5:5:5:1
const B_RGBA15 = 0x2010;		// BGRA 5:5:5:1
const B_CMAP8 = 0x0004;			// 256 color index table
const B_GRAY8 = 0x0002;			// 256 greyscale table
const B_GRAY1 = 0x0001;			// Each bit represents a single pixel
const B_RGB32_BIG = 0x1008;		// -RGB	8:8:8:8
const B_RGBA32_BIG = 0x3008;	// ARGB	8:8:8:8
const B_RGB24_BIG = 0x1003;		// RGB	8:8:8
const B_RGB16_BIG = 0x1005;		// RGB	5:6:5
const B_RGB15_BIG = 0x1010;		// -RGB	1:5:5:5
const B_RGBA15_BIG = 0x3010;	// ARGB	1:5:5:5

const B_TRANSPARENT_MAGIC_CMAP8 = 0xff;
const B_TRANSPARENT_MAGIC_RGBA15 = 0x39ce;
const B_TRANSPARENT_MAGIC_RGBA15_BIG = 0xce39;
const B_TRANSPARENT_MAGIC_RGBA32 = 0xff777477;
const B_TRANSPARENT_MAGIC_RGBA32_BIG = 0x777477ff;


// source_alpha
const B_PIXEL_ALPHA = 0;
const B_CONSTANT_ALPHA = 1;


// alpha_function
const B_ALPHA_OVERLAY = 0;
const B_ALPHA_COMPOSITE = 1;


// BGradient::Type
const B_GRADIENT_TYPE_LINEAR = 0;
const B_GRADIENT_TYPE_RADIAL = 1;
const B_GRADIENT_TYPE_RADIAL_FOCUS = 2;
const B_GRADIENT_TYPE_DIAMOND = 3;
const B_GRADIENT_TYPE_CONIC = 4;
const B_GRADIENT_TYPE_NONE = 5;


// BShape ops
const B_SHAPE_OP_MOVE_TO = 0x80000000;
const B_SHAPE_OP_CLOSE = 0x40000000;
const B_SHAPE_OP_BEZIER_TO = 0x20000000;
const B_SHAPE_OP_LINE_TO = 0x10000000;
const B_SHAPE_OP_SMALL_ARC_TO_CCW = 0x08000000;
const B_SHAPE_OP_SMALL_ARC_TO_CW = 0x04000000;
const B_SHAPE_OP_LARGE_ARC_TO_CCW = 0x02000000;
const B_SHAPE_OP_LARGE_ARC_TO_CW = 0x01000000;


// Line join_modes
const B_ROUND_JOIN = 0;
const B_MITER_JOIN = 1;
const B_BEVEL_JOIN = 2;
const B_BUTT_JOIN = 3;
const B_SQUARE_JOIN = 4;


// Line cap_modes
const B_ROUND_CAP = B_ROUND_JOIN;
const B_BUTT_CAP = B_BUTT_JOIN;
const B_SQUARE_CAP = B_SQUARE_JOIN;


const B_DEFAULT_MITER_LIMIT = 10;


// Font spacing and face
const B_FIXED_SPACING = 3;

const B_ITALIC_FACE = 0x0001;
const B_BOLD_FACE = 0x0020;


// modifiers
const B_SHIFT_KEY = 0x00000001;
const B_COMMAND_KEY = 0x00000002;
const B_CONTROL_KEY = 0x00000004;
const B_CAPS_LOCK = 0x00000008;
const B_SCROLL_LOCK = 0x00000010;
const B_NUM_LOCK = 0x00000020;
const B_OPTION_KEY = 0x00000040;
const B_MENU_KEY = 0x00000080;
const B_LEFT_SHIFT_KEY = 0x00000100;
const B_RIGHT_SHIFT_KEY = 0x00000200;
const B_LEFT_COMMAND_KEY = 0x00000400;
const B_RIGHT_COMMAND_KEY = 0x00000800;
const B_LEFT_CONTROL_KEY = 0x00001000;
const B_RIGHT_CONTROL_KEY = 0x00002000;
const B_LEFT_OPTION_KEY = 0x00004000;
const B_RIGHT_OPTION_KEY = 0x00008000;



var gSession;
var gSystemPalette;


function StreamingDataView(buffer, littleEndian, byteOffset, byteLength)
{
	this.buffer = buffer;
	this.dataView = new DataView(buffer.buffer, byteOffset, byteLength);
	this.position = 0;
	this.littleEndian = littleEndian;
	this.textDecoder = new TextDecoder('utf-8');
	this.textEncoder = new TextEncoder();
}


StreamingDataView.prototype.rewind = function()
{
	this.position = 0;
}


StreamingDataView.prototype.readInt8 = function()
{
	return this.dataView.getInt8(this.position++);
}


StreamingDataView.prototype.readUint8 = function()
{
	return this.dataView.getUint8(this.position++);
}


StreamingDataView.prototype.readInt16 = function()
{
	var result = this.dataView.getInt16(this.position, this.littleEndian);
	this.position += 2;
	return result;
}


StreamingDataView.prototype.readUint16 = function()
{
	var result = this.dataView.getUint16(this.position, this.littleEndian);
	this.position += 2;
	return result;
}


StreamingDataView.prototype.readInt32 = function()
{
	var result = this.dataView.getInt32(this.position, this.littleEndian);
	this.position += 4;
	return result;
}


StreamingDataView.prototype.readUint32 = function()
{
	var result = this.dataView.getUint32(this.position, this.littleEndian);
	this.position += 4;
	return result;
}


StreamingDataView.prototype.readFloat32 = function()
{
	var result = this.dataView.getFloat32(this.position, this.littleEndian);
	this.position += 4;
	return result;
}


StreamingDataView.prototype.readFloat64 = function()
{
	var result = this.dataView.getFloat64(this.position, this.littleEndian);
	this.position += 8;
	return result;
}


StreamingDataView.prototype.readString = function(length)
{
	var where = this.dataView.byteOffset + this.position;
	var part = this.buffer.slice(where, where + length);
	var result = this.textDecoder.decode(part);
	this.position += length;
	return result;
}


StreamingDataView.prototype.readInto = function(typedArray)
{
	var where = this.dataView.byteOffset + this.position;
	typedArray.set(this.buffer.slice(where, where + typedArray.byteLength));
	this.position += typedArray.byteLength;
}


StreamingDataView.prototype.writeInt8 = function(value)
{
	this.dataView.setInt8(this.position++, value);
}


StreamingDataView.prototype.writeUint8 = function(value)
{
	this.dataView.setUint8(this.position++, value);
}


StreamingDataView.prototype.writeInt16 = function(value)
{
	this.dataView.setInt16(this.position, value, this.littleEndian);
	this.position += 2;
}


StreamingDataView.prototype.writeUint16 = function(value)
{
	this.dataView.setUint16(this.position, value, this.littleEndian);
	this.position += 2;
}


StreamingDataView.prototype.writeInt32 = function(value)
{
	this.dataView.setInt32(this.position, value, this.littleEndian);
	this.position += 4;
}


StreamingDataView.prototype.writeUint32 = function(value)
{
	this.dataView.setUint32(this.position, value, this.littleEndian);
	this.position += 4;
}


StreamingDataView.prototype.writeFloat32 = function(value)
{
	this.dataView.setFloat32(this.position, value, this.littleEndian);
	this.position += 4;
}


StreamingDataView.prototype.writeFloat64 = function(value)
{
	this.dataView.setFloat64(this.position, value, this.littleEndian);
	this.position += 8;
}


StreamingDataView.prototype.writeString = function(string)
{
	var encoded = this.textEncoder.encode(string);
	this.writeUint32(encoded.length);
	this.buffer.set(encoded, this.position);
	this.position += encoded.length;
}


StreamingDataView.prototype.setUint32 = function(byteOffset, value)
{
	this.dataView.setUint32(byteOffset, value, this.littleEndian);
}


StreamingDataView.prototype.pad = function(length)
{
	this.buffer.fill(0, this.position, this.position + length);
	this.position += length;
}


function RemotePoint(remoteMessage)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage);
		return;
	}

	this.x = 0;
	this.y = 0;
}


RemotePoint.prototype.readFrom = function(remoteMessage)
{
	this.x = remoteMessage.dataView.readFloat32();
	this.y = remoteMessage.dataView.readFloat32();
	return this;
}


RemotePoint.prototype.writeTo = function(remoteMessage)
{
	remoteMessage.dataView.writeFloat32(this.x);
	remoteMessage.dataView.writeFloat32(this.y);
	return this;
}


function RemoteRect(remoteMessage)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage);
		return;
	}

	this.left = 0;
	this.top = 0;
	this.right = -1;
	this.bottom = -1;
}


RemoteRect.prototype.readFrom = function(remoteMessage)
{
	this.left = remoteMessage.dataView.readFloat32();
	this.top = remoteMessage.dataView.readFloat32();
	this.right = remoteMessage.dataView.readFloat32();
	this.bottom = remoteMessage.dataView.readFloat32();
	return this;
}


RemoteRect.prototype.width = function()
{
	return this.right - this.left + 1;
}


RemoteRect.prototype.height = function()
{
	return this.bottom - this.top + 1;
}


RemoteRect.prototype.integerWidth = function()
{
	return Math.ceil(this.right - this.left);
}


RemoteRect.prototype.integerHeight = function()
{
	return Math.ceil(this.bottom - this.top);
}


RemoteRect.prototype.centerX = function()
{
	return this.left + this.width() / 2;
}


RemoteRect.prototype.centerY = function()
{
	return this.top + this.height() / 2;
}


RemoteRect.prototype.apply = function(apply)
{
	var left = Math.floor(this.left);
	var top = Math.floor(this.top);
	var right = Math.ceil(this.right);
	var bottom = Math.ceil(this.bottom);
	apply(left, top, right - left + 1, bottom - top + 1);
}


RemoteRect.prototype.applyAsEllipse = function(context, which)
{
	context.beginPath();
	context.ellipse(this.centerX(), this.centerY(), this.width() / 2,
		this.height() / 2, 0, Math.PI * 2, false);
	which.call(context);
}


function RemoteColor(remoteMessage)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage);
		return;
	}

	this.red = 0;
	this.green = 0;
	this.blue = 0;
	this.alpha = 0;
}


RemoteColor.prototype.readFrom = function(remoteMessage)
{
	this.red = remoteMessage.dataView.readUint8();
	this.green = remoteMessage.dataView.readUint8();
	this.blue = remoteMessage.dataView.readUint8();
	this.alpha = remoteMessage.dataView.readUint8();
	return this;
}


RemoteColor.prototype.fromUint32 = function(value)
{
	this.red = value & 0xff;
	this.green = value >> 8 & 0xff;
	this.blue = value >> 16 & 0xff;
	this.alpha = value >> 24 & 0xff;
	return this;
}


RemoteColor.prototype.toColor = function(unsetAlpha)
{
	return 'rgba(' + this.red + ', ' + this.green + ', ' + this.blue + ', '
		+ (unsetAlpha ? 1 : this.alpha / 255) + ')';
}


RemoteColor.prototype.toUint32 = function(unsetAlpha)
{
	return this.red | this.green << 8 | this.blue << 16
		| (unsetAlpha ? 255 : this.alpha) << 24;
}


function RemoteFont(remoteMessage)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage);
		return;
	}

	this.direction = 0;
	this.encoding = 0;
	this.flags = 0;
	this.spacing = 0;
	this.shear = 0;
	this.rotation = 0;
	this.falseBoldWidth = 0;
	this.size = 12;
	this.face = 0;
	this.family = 0;
	this.style = 0;
}


RemoteFont.prototype.readFrom = function(remoteMessage)
{
	this.direction = remoteMessage.dataView.readUint8();
	this.encoding = remoteMessage.dataView.readUint8();
	this.flags = remoteMessage.dataView.readUint32();
	this.spacing = remoteMessage.dataView.readUint8();
	this.shear = remoteMessage.dataView.readFloat32();
	this.rotation = remoteMessage.dataView.readFloat32();
	this.falseBoldWidth = remoteMessage.dataView.readFloat32();
	this.size = remoteMessage.dataView.readFloat32();
	this.face = remoteMessage.dataView.readUint16();
	this.family = remoteMessage.dataView.readUint16();
	this.style = remoteMessage.dataView.readUint16();
	return this;
}


function RemoteTransform(remoteMessage)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage);
		return;
	}

	this.setIdentity();
}


RemoteTransform.prototype.readFrom = function(remoteMessage)
{
	var isIdentity = remoteMessage.dataView.readUint8();
	if (isIdentity) {
		this.setIdentity();
		return;
	}

	this.sx = remoteMessage.dataView.readFloat64();
	this.shy = remoteMessage.dataView.readFloat64();
	this.shx = remoteMessage.dataView.readFloat64();
	this.sy = remoteMessage.dataView.readFloat64();
	this.tx = remoteMessage.dataView.readFloat64();
	this.ty = remoteMessage.dataView.readFloat64();
	return this;
}


RemoteTransform.prototype.setIdentity = function()
{
	this.sx = 1;
	this.shy = 0;
	this.shx = 0;
	this.sy = 1;
	this.tx = 0;
	this.ty = 0;
	return this;
}


RemoteTransform.prototype.isIdentity = function()
{
	return this.sx == 1 && this.shy == 0 && this.shx == 0 && this.sy == 1
		&& this.tx == 0 && this.ty == 0;
}


RemoteTransform.prototype.apply = function(context)
{
	context.transform(this.sx, this.shy, this.shx, this.sy, this.tx,
		this.ty);
}


function RemoteBitmap(remoteMessage, unsetAlpha, colorSpace, flags)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage, unsetAlpha, colorSpace, flags);
		return;
	}
}


RemoteBitmap.prototype.readFrom = function(remoteMessage, unsetAlpha,
	colorSpace, flags)
{
	this.width = remoteMessage.dataView.readUint32();
	this.height = remoteMessage.dataView.readUint32();
	this.bytesPerRow = remoteMessage.dataView.readUint32();

	if (colorSpace != undefined) {
		this.colorSpace = colorSpace;
		this.flags = flags;
	} else {
		this.colorSpace = remoteMessage.dataView.readUint32();
		this.flags = remoteMessage.dataView.readUint32();
	}

	this.bitsLength = remoteMessage.dataView.readUint32();

	this.canvas = document.createElement('canvas');
	this.canvas.width = this.width;
	this.canvas.height = this.height;

	if (this.width == 0 || this.height == 0)
		return;

	var context = this.canvas.getContext('2d');
	var imageData = context.createImageData(this.width, this.height);
	switch (this.colorSpace) {
		case B_RGBA32:
			remoteMessage.dataView.readInto(imageData.data);
			var output = new Uint32Array(imageData.data.buffer);

			for (var i = 0; i < imageData.data.length / 4; i++) {
				output[i] = (output[i] & 0xff) << 16 | (output[i] >> 16 & 0xff)
					| (output[i] & 0xff00ff00);
			}

			if (unsetAlpha) {
				for (var i = 0; i < imageData.data.length / 4; i++)
					output[i] |= 0xff000000;
			}

			break;

		case B_RGB32:
			remoteMessage.dataView.readInto(imageData.data);
			var output = new Uint32Array(imageData.data.buffer);

			for (var i = 0; i < imageData.data.length / 4; i++) {
				output[i] = (output[i] & 0xff) << 16 | (output[i] >> 16 & 0xff)
					| (output[i] & 0xff00) | 0xff000000;

				if (!unsetAlpha && output[i] == B_TRANSPARENT_MAGIC_RGBA32)
					output[i] &= 0x00ffffff;
			}
			break;

		case B_RGB24:
			var line = new Uint8Array(this.bytesPerRow);
			var position = 0;

			for (var y = 0; y < this.height; y++) {
				remoteMessage.dataView.readInto(line);

				for (var x = 0; x < this.width; x++) {
					imageData.data[position++] = line[x * 3 + 2];
					imageData.data[position++] = line[x * 3 + 1];
					imageData.data[position++] = line[x * 3 + 0];
					imageData.data[position++] = 255;
				}
			}

			break;

		case B_RGB16:
			var lineBuffer = new Uint8Array(this.bytesPerRow);
			var line = new Uint16Array(lineBuffer.buffer);
			var position = 0;

			for (var y = 0; y < this.height; y++) {
				remoteMessage.dataView.readInto(lineBuffer);

				for (var x = 0; x < this.width; x++) {
					imageData.data[position++] = (line[x] & 0xf800) >> 8;
					imageData.data[position++] = (line[x] & 0x07e0) >> 3;
					imageData.data[position++] = (line[x] & 0x001f) << 3;
					imageData.data[position++] = 255;
				}
			}

			break;

		case B_CMAP8:
			var line = new Uint8Array(this.bytesPerRow);
			var output = new Uint32Array(imageData.data.buffer);
			var position = 0;

			for (var y = 0; y < this.height; y++) {
				remoteMessage.dataView.readInto(line);

				for (var x = 0; x < this.width; x++)
					output[position++] = gSystemPalette[line[x]];
			}

			break;

		case B_GRAY8:
			var source = new Uint8Array(this.bitsLength);
			remoteMessage.dataView.readInto(source);
			for (var i = 0; i < imageData.data.length / 4; i++) {
				imageData.data[i * 4 + 0] = source[i];
				imageData.data[i * 4 + 1] = source[i];
				imageData.data[i * 4 + 2] = source[i];
				imageData.data[i * 4 + 3] = 255;
			}
			break;

		case B_GRAY1:
			var source = new Uint8Array(this.bitsLength);
			remoteMessage.dataView.readInto(source);
			for (var i = 0; i < imageData.data.length / 4; i++) {
				var value = (source[Math.floor(i / 8)] >> i % 8) & 1 ? 255 : 0;
				imageData.data[i * 4 + 0] = value;
				imageData.data[i * 4 + 1] = value;
				imageData.data[i * 4 + 2] = value;
				imageData.data[i * 4 + 3] = 255;
			}
			break;

		default:
			console.warn('color space not implemented: ' + this.colorSpace);
			break;
	}

	context.putImageData(imageData, 0, 0);
	return this;
}


function RemotePattern(remoteMessage)
{
	this.data = new Uint8Array(8);

	if (remoteMessage)
		this.readFrom(remoteMessage);
	else
		this.data.fill(255);
}


RemotePattern.staticCanvas = document.createElement('canvas');
RemotePattern.staticCanvas.width = RemotePattern.staticCanvas.height = 8;
RemotePattern.staticContext = RemotePattern.staticCanvas.getContext('2d');
RemotePattern.staticImageData
	= RemotePattern.staticContext.createImageData(8, 8);
RemotePattern.staticPixels
	= new Uint32Array(RemotePattern.staticImageData.data.buffer);


RemotePattern.prototype.readFrom = function(remoteMessage)
{
	remoteMessage.dataView.readInto(this.data);
	return this;
}


RemotePattern.prototype.isSolid = function()
{
	var common = this.data[0];
	return this.data.every(function(value) { return value == common; });
}


RemotePattern.prototype.toPattern = function(context, lowColor, highColor)
{
	for (var i = 0; i < this.data.length * 8; i++) {
		RemotePattern.staticPixels[i]
			= (this.data[i / 8 | 0] & 1 << 7 - i % 8) == 0
				? lowColor : highColor;
	}

	// Apparently supplying ImageData to createPattern fails in Chrome.
	RemotePattern.staticContext.putImageData(RemotePattern.staticImageData, 0,
		0);
	return context.createPattern(RemotePattern.staticCanvas, 'repeat');
}


function RemoteGradient(remoteMessage, context, unsetAlpha)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage, context, unsetAlpha);
		return;
	}

	this.gradient = '#00000000';
}


RemoteGradient.prototype.readFrom = function(remoteMessage, context, unsetAlpha)
{
	this.type = remoteMessage.dataView.readUint32();
	switch (this.type) {
		case B_GRADIENT_TYPE_LINEAR:
			var start = new RemotePoint(remoteMessage);
			var end = new RemotePoint(remoteMessage);

			this.gradient = context.createLinearGradient(start.x, start.y,
				end.x, end.y);
			break;

		case B_GRADIENT_TYPE_RADIAL:
			var center = new RemotePoint(remoteMessage);
			var radius = remoteMessage.dataView.readFloat32();

			this.gradient = context.createRadialGradient(center.x, center.y, 0,
				center.x, center.y, radius);
			break;

		default:
			console.warn('gradient type not implemented: ' + this.type);
			this.gradient = 'black';
			return this;
	}

	var stopCount = remoteMessage.dataView.readUint32();
	for (var i = 0; i < stopCount; i++) {
		var color = remoteMessage.readColor(unsetAlpha);
		var offset = remoteMessage.dataView.readFloat32() / 255;
		this.gradient.addColorStop(offset, color);
	}

	return this;
}


function RemoteShape(remoteMessage)
{
	if (remoteMessage) {
		this.readFrom(remoteMessage);
		return;
	}

	this.opCount = 0;
	this.ops = [];
	this.pointCount = 0;
	this.points = [];
}


RemoteShape.prototype.readFrom = function(remoteMessage)
{
	this.bounds = new RemoteRect(remoteMessage);

	this.opCount = remoteMessage.dataView.readUint32();
	this.ops = new Array(this.opCount);
	for (var i = 0; i < this.opCount; i++)
		this.ops[i] = remoteMessage.dataView.readUint32();

	this.pointCount = remoteMessage.dataView.readUint32();
	this.points = new Array(this.pointCount);
	for (var i = 0; i < this.pointCount; i++)
		this.points[i] = new RemotePoint(remoteMessage);

	return this;
}


RemoteShape.prototype.play = function(context)
{
	var pointIndex = 0;
	for (var i = 0; i < this.opCount; i++) {
		var op = this.ops[i] & 0xff000000;
		var count = this.ops[i] & 0x00ffffff;

		if (op & B_SHAPE_OP_MOVE_TO) {
			var point = this.points[pointIndex++];
			context.moveTo(point.x, point.y);
		}

		if (op & B_SHAPE_OP_LINE_TO) {
			for (var j = 0; j < count; j++) {
				var point = this.points[pointIndex++];
				context.lineTo(point.x, point.y);
			}
		}

		if (op & B_SHAPE_OP_BEZIER_TO) {
			for (var j = 0; j < count / 3; j++) {
				var control1 = this.points[pointIndex++];
				var control2 = this.points[pointIndex++];
				var to = this.points[pointIndex++];
				context.bezierCurveTo(control1.x, control1.y, control2.x,
					control2.y, to.x, to.y);
			}
		}

		if (op & (B_SHAPE_OP_LARGE_ARC_TO_CW | B_SHAPE_OP_LARGE_ARC_TO_CCW
				| B_SHAPE_OP_SMALL_ARC_TO_CW | B_SHAPE_OP_SMALL_ARC_TO_CCW)) {

			console.warn('shape op arc to not implemented');
			for (var j = 0; j < count / 3; j++)
				pointIndex++;
		}

		if (op & B_SHAPE_OP_CLOSE)
			context.closePath();
	}
}


function RemoteMessage(socket)
{
	this.socket = socket;
}


RemoteMessage.staticRemoteColor = new RemoteColor();


RemoteMessage.prototype.allocate = function(bufferSize)
{
	this.buffer = new Uint8Array(bufferSize);
	this.dataView = new StreamingDataView(this.buffer, true);
}


RemoteMessage.prototype.ensureBufferSize = function(bufferSize)
{
	if (this.buffer.byteLength < bufferSize)
		this.allocate(bufferSize);
}


RemoteMessage.prototype.attach = function(buffer, byteOffset)
{
	var bytesLeft = buffer.byteLength - byteOffset;
	if (bytesLeft < 6)
		return false;

	this.buffer = buffer;
	this.dataView = new StreamingDataView(this.buffer, true, byteOffset);
	this.messageCode = this.dataView.readUint16();
	this.messageSize = this.dataView.readUint32();
	if (this.messageSize < 6)
		throw false;

	return this.messageSize <= bytesLeft;
}


RemoteMessage.prototype.code = function()
{
	return this.messageCode;
}


RemoteMessage.prototype.size = function()
{
	return this.messageSize;
}


RemoteMessage.prototype.start = function(code)
{
	this.dataView.rewind();
	this.dataView.writeUint16(code);
	this.dataView.writeUint32(0);
		// Placeholder for size field.
}


RemoteMessage.prototype.flush = function()
{
	this.dataView.setUint32(2, this.dataView.position);
	this.socket.send(this.buffer.slice(0, this.dataView.position));
}


RemoteMessage.prototype.readColor = function(unsetAlpha)
{
	return RemoteMessage.staticRemoteColor.readFrom(this).toColor(unsetAlpha);
}


function RemoteState(session, token)
{
	this.session = session;
	this.token = token;

	this.lowColor = new RemoteColor().fromUint32(0xffffffff);
	this.highColor = new RemoteColor().fromUint32(0xff000000);

	this.penSize = 1.0;
	this.lineCap = 'butt';
	this.lineJoin = 'miter';
	this.miterLimit = B_DEFAULT_MITER_LIMIT;
	this.drawingMode = 'source-over';

	this.pattern = new RemotePattern();
	this.font = new RemoteFont();
	this.transform = new RemoteTransform();
}


RemoteState.prototype.applyContext = function()
{
	var context = this.session.context;
	if (!this.invalidated && context.currentToken == this.token)
		return;

	this.session.removeClipping();

	if (this.blendModesEnabled && this.constantAlpha)
		context.globalAlpha = this.highColor.alpha / 255;
	else
		context.globalAlpha = 1;

	var style;
	if (this.pattern.isSolid()) {
		style = this.pattern.data[0] == 0 ? this.lowColor : this.highColor;
		if (this.invert)
			style = style == this.lowColor ? 'transparent' : 'white';
		else
			style = style.toColor(this.unsetAlpha);
	} else {
		style = this.pattern.toPattern(context,
			this.invert ? 0x00000000 : this.lowColor.toUint32(this.unsetAlpha),
			this.invert
				? 0xffffffff : this.highColor.toUint32(this.unsetAlpha));
	}

	context.fillStyle = context.strokeStyle = style;

	context.font = (this.font.face & B_ITALIC_FACE ? "italic " : "")
		+ (this.font.face & B_BOLD_FACE ? "bold " : "")
		+ this.font.size + 'px '
		+ (this.font.spacing==B_FIXED_SPACING ? 'monospace' : 'Helvetica');
	context.globalCompositeOperation = this.drawingMode;
	context.lineWidth = this.penSize;
	context.lineCap = this.lineCap;
	context.lineJoin = this.lineJoin;
	context.miterLimit = this.miterLimit;

	context.resetTransform();

	this.session.applyClipping(this.clipRects);

	if (!this.transform.isIdentity()) {
		context.translate(this.xOffset, this.yOffset);
		this.transform.apply(context);
		context.translate(-this.xOffset, -this.yOffset);
	}

	context.currentToken = this.token;
	this.invalidated = false;
}


RemoteState.prototype.prepareForRect = function()
{
	this.session.context.lineJoin = 'miter';
	this.session.context.miterLimit = 10;
}


RemoteState.prototype.messageReceived = function(remoteMessage, reply)
{
	var context = this.session.context;

	switch (remoteMessage.code()) {
		case RP_ENABLE_SYNC_DRAWING:
		case RP_DISABLE_SYNC_DRAWING:
			console.warn('sync drawing en-/disable not implemented');
			break;

		case RP_SET_LOW_COLOR:
			this.lowColor.readFrom(remoteMessage);
			this.invalidated = true;
			break;

		case RP_SET_HIGH_COLOR:
			this.highColor.readFrom(remoteMessage);
			this.invalidated = true;
			break;

		case RP_SET_OFFSETS:
			this.xOffset = remoteMessage.dataView.readInt32();
			this.yOffset = remoteMessage.dataView.readInt32();
			this.invalidated = true;
			break;

		case RP_SET_FONT:
			this.font = new RemoteFont(remoteMessage);
			this.invalidated = true;
			break;

		case RP_SET_TRANSFORM:
			this.transform = new RemoteTransform(remoteMessage);
			this.invalidated = true;
			break;

		case RP_SET_PATTERN:
			this.pattern = new RemotePattern(remoteMessage);
			this.invalidated = true;
			break;

		case RP_SET_PEN_SIZE:
			this.penSize = remoteMessage.dataView.readFloat32();
			this.invalidated = true;
			break;

		case RP_SET_STROKE_MODE:
			switch (remoteMessage.dataView.readUint32()) {
				case B_ROUND_CAP:
					this.lineCap = 'round';
					break;

				case B_BUTT_CAP:
					this.lineCap = 'butt';
					break;

				case B_SQUARE_CAP:
					this.lineCap = 'square';
					break;
			}

			var lineJoin = remoteMessage.dataView.readUint32();
			switch (lineJoin) {
				case B_ROUND_JOIN:
					this.lineJoin = 'round';
					break;

				case B_MITER_JOIN:
					this.lineJoin = 'miter';
					break;

				case B_BEVEL_JOIN:
					this.lineJoin = 'bevel';
					break;

				default:
					console.warn('line join not implemented: ' + join);
					break;
			}

			this.miterLimit = remoteMessage.dataView.readFloat32();
			this.invalidated = true;
			break;

		case RP_SET_BLENDING_MODE:
			var sourceAlpha = remoteMessage.dataView.readUint32();
			this.constantAlpha = sourceAlpha == B_CONSTANT_ALPHA;
			if (this.blendModesEnabled)
				this.unsetAlpha = this.constantAlpha;

			var alphaFunction = remoteMessage.dataView.readUint32();
			if (alphaFunction != B_ALPHA_OVERLAY)
				console.warn('alpha function not supported: ' + alphaFunction);

			this.invalidated = true;
			break;

		case RP_SET_DRAWING_MODE:
			var drawingMode = remoteMessage.dataView.readUint32();

			this.unsetAlpha = false;
			this.blendModesEnabled = false;
			this.invert = false;

			switch (drawingMode) {
				case B_OP_COPY:
					this.unsetAlpha = true;
					this.drawingMode = 'source-over';
					break;

				case B_OP_OVER:
					this.drawingMode = 'source-over';
					break;

				case B_OP_ALPHA:
					this.blendModesEnabled = true;
					this.unsetAlpha = this.constantAlpha;
					this.drawingMode = 'source-over';
					break;

				case B_OP_BLEND:
					this.drawingMode = 'lighter';
					break;

				case B_OP_MIN:
					this.drawingMode = 'darken';
					break;

				case B_OP_MAX:
					this.drawingMode = 'ligthen';
					break;

				case B_OP_INVERT:
					this.drawingMode = 'difference';
					this.invert = true;
					break;

				case B_OP_ADD:
					this.drawingMode = 'lighter';
					break;

/*
				case B_OP_ERASE:
					this.drawingMode = 'destination-out';
					break;

				case B_OP_SUBTRACT:
					this.drawingMode = 'difference';
					break;
*/

				default:
					console.warn('drawing mode not implemented: '
						+ drawingMode);
					this.drawingMode = 'source-over';
					break;
			}

			this.invalidated = true;
			break;

		case RP_CONSTRAIN_CLIPPING_REGION:
			var rectCount = remoteMessage.dataView.readUint32();
			this.clipRects = new Array(rectCount);
			for (var i = 0; i < rectCount; i++)
				this.clipRects[i] = new RemoteRect(remoteMessage);

			this.invalidated = true;
			break;

		case RP_INVERT_RECT:
			this.applyContext();

			var rect = new RemoteRect(remoteMessage);

			context.save();
			context.globalCompositeOperation = 'difference';
			context.fillStyle = 'white';
			this.prepareForRect();

			rect.apply(context.fillRect.bind(context));

			context.restore();
			break;

		case RP_DRAW_BITMAP:
			this.applyContext();

			var bitmapRect = new RemoteRect(remoteMessage);
			var viewRect = new RemoteRect(remoteMessage);
			var options = remoteMessage.dataView.readUint32();
				// TODO: Implement options.

			if (options != 0)
				console.warn('bitmap options not supported: ' + options);

			var bitmap = new RemoteBitmap(remoteMessage, this.unsetAlpha);
			context.drawImage(bitmap.canvas, bitmapRect.left, bitmapRect.top,
				bitmapRect.width(), bitmapRect.height(), viewRect.left,
				viewRect.top, viewRect.width(), viewRect.height());
			break;

		case RP_DRAW_BITMAP_RECTS:
			this.applyContext();

			var options = remoteMessage.dataView.readUint32();
				// TODO: Implement options.
			var colorSpace = remoteMessage.dataView.readUint32();
			var flags = remoteMessage.dataView.readUint32();

			if (options != 0)
				console.warn('bitmap options not supported: ' + options);

			var rectCount = remoteMessage.dataView.readUint32();
			for (var i = 0; i < rectCount; i++) {
				var rect = new RemoteRect(remoteMessage);
				var bitmap = new RemoteBitmap(remoteMessage, this.unsetAlpha,
					colorSpace, flags);

				context.drawImage(bitmap.canvas, 0, 0, bitmap.width,
					bitmap.height, rect.left, rect.top, rect.width(),
					rect.height());
			}
			break;

		case RP_DRAW_STRING:
			this.applyContext();

			var where = new RemotePoint(remoteMessage);
			var length = remoteMessage.dataView.readUint32();
			var string = remoteMessage.dataView.readString(length);

			context.save();
			context.fillStyle = this.highColor.toColor(this.unsetAlpha);
			context.fillText(string, where.x, where.y);

			var textMetric = context.measureText(string);
			where.x += textMetric.width;

			context.restore();

			reply.start(RP_DRAW_STRING_RESULT);
			reply.dataView.writeInt32(this.token);
			where.writeTo(reply);
			reply.flush();
			break;

		case RP_DRAW_STRING_WITH_OFFSETS:
			this.applyContext();

			var length = remoteMessage.dataView.readUint32();
			var string = remoteMessage.dataView.readString(length);

			context.save();
			context.fillStyle = this.highColor.toColor(this.unsetAlpha);

			var where;
			for (var i = 0; i < string.length; i++) {
				where = new RemotePoint(remoteMessage);
				context.fillText(string[i], where.x, where.y);
			}

			var textMetric = context.measureText(string[string.length - 1]);
			where.x += textMetric.width;

			context.restore();

			reply.start(RP_DRAW_STRING_RESULT);
			reply.dataView.writeInt32(this.token);
			where.writeTo(reply);
			reply.flush();
			break;

		case RP_STRING_WIDTH:
			this.applyContext();

			var length = remoteMessage.dataView.readUint32();
			var string = remoteMessage.dataView.readString(length);
			var textMetric = context.measureText(string);

			reply.start(RP_STRING_WIDTH_RESULT);
			reply.dataView.writeInt32(this.token);
			where.writeFloat32(textMetric.width);
			reply.flush();
			break;

		case RP_STROKE_ARC:
		case RP_FILL_ARC:
			this.applyContext();

			var rect = new RemoteRect(remoteMessage);
			var startAngle
				= remoteMessage.dataView.readFloat32() * Math.PI / 180;
			var invertStart = Math.PI * 2 - startAngle;
			startAngle += Math.PI / 2;

			var span = remoteMessage.dataView.readFloat32() * Math.PI / 180;
			var centerX = Math.round(rect.centerX());
			var centerY = Math.round(rect.centerY());
			var radius = rect.width() / 2;
			var maxSpan
				= remoteMessage.code() != RP_STROKE_ARC ? Math.PI / 2 : span;

			var arcStep = function(max) {
					max = Math.min(max, span);

					context.beginPath();
					context.arc(centerX, centerY, radius, invertStart,
						invertStart - max, true);

					switch (remoteMessage.code()) {
						case RP_STROKE_ARC:
							context.stroke();
							break;

						case RP_FILL_ARC:
							context.moveTo(centerX, centerY);
							var endAngle = startAngle + max;
							context.lineTo(
								centerX + radius * Math.sin(startAngle),
								centerY + radius * Math.cos(startAngle));
							context.lineTo(
								centerX + radius * Math.sin(endAngle),
								centerY + radius * Math.cos(endAngle));
							context.fill();
							break;
					}

					startAngle += max;
					invertStart -= max;
					span -= max;
				};

			while (span > 0)
				arcStep(maxSpan);

			break;

		case RP_STROKE_RECT:
		case RP_STROKE_ELLIPSE:
		case RP_FILL_RECT:
		case RP_FILL_ELLIPSE:
			this.applyContext();

			context.save();
			this.prepareForRect();

			var rect = new RemoteRect(remoteMessage);

			switch (remoteMessage.code()) {
				case RP_STROKE_RECT:
					rect.apply(context.strokeRect.bind(context));
					break;
				case RP_STROKE_ELLIPSE:
					rect.applyAsEllipse(context, context.stroke);
					break;
				case RP_FILL_RECT:
					rect.apply(context.fillRect.bind(context));
					break;
				case RP_FILL_ELLIPSE:
					rect.applyAsEllipse(context, context.fill);
					break;
			}

			context.restore();
			break;

		case RP_STROKE_ROUND_RECT:
		case RP_FILL_ROUND_RECT:
		case RP_FILL_ROUND_RECT_GRADIENT:
			this.applyContext();

			context.save();
			this.prepareForRect();

			var rect = new RemoteRect(remoteMessage);
			var xRadius = remoteMessage.dataView.readFloat32();
			var yRadius = remoteMessage.dataView.readFloat32();

			if (remoteMessage.code() == RP_FILL_ROUND_RECT_GRADIENT) {
				context.save();
				var gradient = new RemoteGradient(remoteMessage, context,
					this.unsetAlpha);
				context.fillStyle = gradient.gradient;
			}

			console.warn('round rects not implemented, falling back to rect');
			if (remoteMessage.code() == RP_STROKE_ROUND_RECT)
				rect.apply(context.strokeRect.bind(context));
			else
				rect.apply(context.fillRect.bind(context));

			if (remoteMessage.code() == RP_FILL_ROUND_RECT_GRADIENT)
				context.restore();

			context.restore();
			break;

		case RP_STROKE_LINE:
			this.applyContext();

			var from = new RemotePoint(remoteMessage);
			var to = new RemotePoint(remoteMessage);

			context.beginPath();
			context.moveTo(from.x, from.y);
			context.lineTo(to.x, to.y);
			context.stroke();
			break;

		case RP_STROKE_LINE_ARRAY:
			this.applyContext();

			context.save();
			context.lineCap = 'square';

			var numLines = remoteMessage.dataView.readUint32();
			for (var i = 0; i < numLines; i++) {
				var from = new RemotePoint(remoteMessage);
				var to = new RemotePoint(remoteMessage);
				context.strokeStyle = remoteMessage.readColor(this.unsetAlpha);
				context.beginPath();
				context.moveTo(from.x + 0.5, from.y + 0.5);
				context.lineTo(to.x + 0.5, to.y + 0.5);
				context.stroke();
			}

			context.restore();
			break;

		case RP_STROKE_POINT_COLOR:
			this.applyContext();

			var point = new RemotePoint(remoteMessage);

			context.save();
			context.fillStyle = remoteMessage.readColor(this.unsetAlpha);

			context.fillRect(point.x, point.y, 1, 1);
			context.restore();
			break;

		case RP_STROKE_LINE_1PX_COLOR:
			this.applyContext();

			var from = new RemotePoint(remoteMessage);
			var to = new RemotePoint(remoteMessage);

			context.save();
			context.strokeStyle = remoteMessage.readColor(this.unsetAlpha);
			context.lineWidth = 1;
			context.lineCap = 'square';

			context.beginPath();
			context.moveTo(from.x + 0.5, from.y + 0.5);
			context.lineTo(to.x + 0.5, to.y + 0.5);
			context.stroke();

			context.restore();
			break;

		case RP_STROKE_RECT_1PX_COLOR:
			this.applyContext();

			var rect = new RemoteRect(remoteMessage);

			context.save();
			this.prepareForRect();

			context.strokeStyle = remoteMessage.readColor(this.unsetAlpha);
			context.lineWidth = 1;

			rect.apply(context.strokeRect.bind(context));

			context.restore();
			break;

		case RP_STROKE_SHAPE:
		case RP_FILL_SHAPE:
		case RP_FILL_SHAPE_GRADIENT:
			this.applyContext();

			var shape = new RemoteShape(remoteMessage);
			var offset = new RemotePoint(remoteMessage);
			var scale = remoteMessage.dataView.readFloat32();

			context.save();
			if (remoteMessage.code() == RP_FILL_SHAPE_GRADIENT) {
				var gradient = new RemoteGradient(remoteMessage, context,
					this.unsetAlpha);
				context.fillStyle = gradient.gradient;
			}

			context.translate(offset.x + 0.5, offset.y + 0.5);
			context.scale(scale, scale);

			context.beginPath();

			shape.play(context);

			if (remoteMessage.code() == RP_STROKE_SHAPE)
				context.stroke();
			else
				context.fill();

			context.restore();
			break;

		case RP_STROKE_TRIANGLE:
		case RP_FILL_TRIANGLE:
		case RP_FILL_TRIANGLE_GRADIENT:
			this.applyContext();

			if (remoteMessage.code() == RP_FILL_TRIANGLE_GRADIENT)
				context.save();

			context.beginPath();
			var point = new RemotePoint(remoteMessage);
			context.moveTo(point.x + 0.5, point.y + 0.5);

			for (var i = 0; i < 2; i++) {
				point = new RemotePoint(remoteMessage);
				context.lineTo(point.x + 0.5, point.y + 0.5);
			}

			if (remoteMessage.code() == RP_FILL_TRIANGLE_GRADIENT) {
				var unusedBounds = new RemoteRect(remoteMessage);
				var gradient = new RemoteGradient(remoteMessage, context,
					this.unsetAlpha);
				context.fillStyle = gradient.gradient;
			}

			switch (remoteMessage.code()) {
				case RP_STROKE_TRIANGLE:
					context.closePath();
					context.stroke();
					break;

				case RP_FILL_TRIANGLE:
					context.fill();
					break;

				case RP_FILL_TRIANGLE_GRADIENT:
					context.fill();
					context.restore();
					break;
			}

			break;

		case RP_FILL_RECT_COLOR:
			this.applyContext();

			var rect = new RemoteRect(remoteMessage);

			context.save();
			this.prepareForRect();
			context.fillStyle = remoteMessage.readColor(this.unsetAlpha);

			rect.apply(context.fillRect.bind(context));

			context.restore();
			break;

		case RP_FILL_RECT_GRADIENT:
		case RP_FILL_ELLIPSE_GRADIENT:
			this.applyContext();

			var rect = new RemoteRect(remoteMessage);

			context.save();
			this.prepareForRect();

			var gradient = new RemoteGradient(remoteMessage, context,
				this.unsetAlpha);
			context.fillStyle = gradient.gradient;

			if (remoteMessage.code() == RP_FILL_RECT_GRADIENT)
				rect.apply(context.fillRect.bind(context));
			else
				rect.applyAsEllipse(context, context.fill);

			context.restore();
			break;

		case RP_FILL_REGION:
		case RP_FILL_REGION_GRADIENT:
			this.applyContext();

			var rectCount = remoteMessage.dataView.readUint32();
			var rects = new Array(rectCount);
			for (var i = 0; i < rectCount; i++)
				rects[i] = new RemoteRect(remoteMessage);

			if (remoteMessage.code() == RP_FILL_REGION_GRADIENT) {
				context.save();
				var gradient = new RemoteGradient(remoteMessage, context,
					this.unsetAlpha);
				context.fillStyle = gradient.gradient;
			}

			for (var i = 0; i < rectCount; i++)
				rects[i].apply(context.fillRect.bind(context));

			if (remoteMessage.code() == RP_FILL_REGION_GRADIENT)
				context.restore();

			break;

		case RP_READ_BITMAP:
			var bounds = new RemoteRect(remoteMessage);
			var drawCursor = remoteMessage.dataView.readUint8();
				// TODO: Support the drawCursor flag.

			if (drawCursor)
				console.warn('draw cursor in read bitmap not supported');

			var width = bounds.integerWidth() + 1;
			var height = bounds.integerHeight() + 1;
			var bytesPerPixel = 3;
			var bytesPerRow = (width * bytesPerPixel + 3) & ~7;
			var padding = bytesPerRow - width * bytesPerPixel;
			var bitsLength = height * bytesPerRow;

			reply.ensureBufferSize(bitsLength + 1024);

			reply.start(RP_READ_BITMAP_RESULT);
			reply.dataView.writeInt32(this.token);

			reply.dataView.writeInt32(width);
			reply.dataView.writeInt32(height);
			reply.dataView.writeInt32(bytesPerRow);
			reply.dataView.writeUint32(B_RGB24);
			reply.dataView.writeUint32(0); // Flags
			reply.dataView.writeUint32(bitsLength);

			var position = 0;
			var imageData
				= context.getImageData(bounds.left, bounds.top, width, height);
			for (var y = 0; y < height; y++) {
				for (var x = 0; x < width; x++, position += 4) {
					reply.dataView.writeUint8(imageData.data[position + 2]);
					reply.dataView.writeUint8(imageData.data[position + 1]);
					reply.dataView.writeUint8(imageData.data[position + 0]);
				}

				reply.dataView.pad(padding);
			}

			reply.flush();
			break;

		default:
			console.warn('unhandled message: code: ' + remoteMessage.code()
				+ '; size: ' + remoteMessage.size());
			break;
	}
}


function RemoteDesktopSession(targetElement, width, height, targetAddress,
	disconnectCallback)
{
	this.websocket = new WebSocket(targetAddress, 'binary');
	this.websocket.binaryType = 'arraybuffer';
	this.websocket.onopen = this.onOpen.bind(this);
	this.websocket.onmessage = this.onMessage.bind(this);
	this.websocket.onerror = this.onError.bind(this);
	this.websocket.onclose = this.onClose.bind(this);

	this.disconnectCallback = disconnectCallback;

	this.sendMessage = new RemoteMessage(this.websocket);
	this.sendMessage.allocate(1024);

	this.receiveMessage = new RemoteMessage();

	this.container = document.createElement('div');
	this.container.className = 'session';
	this.container.style.position = 'relative';
	targetElement.appendChild(this.container);

	this.canvas = document.createElement('canvas');
	this.canvas.className = 'session';
	this.canvas.width = width;
	this.canvas.height = height;
	this.container.appendChild(this.canvas);

	this.canvas.tabIndex = 0;
	this.canvas.focus();

	this.context = this.canvas.getContext('2d', { alpha: false });
	this.context.imageSmoothingEnabled = false;

	this.cursorVisible = true;
	this.cursorPosition = { x: 0, y: 0 };
	this.cursorHotspot = { x: 0, y: 0 };

	this.states = new Object();
	this.modifiers = 0;

	this.canvas.onmousemove = this.onMouseMove.bind(this);
	this.canvas.onmousedown = this.onMouseDown.bind(this);
	this.canvas.onmouseup = this.onMouseUp.bind(this);
	this.canvas.onwheel = this.onWheel.bind(this);

	this.canvas.onkeydown = this.onKeyDownUp.bind(this);
	this.canvas.onkeyup = this.onKeyDownUp.bind(this);
	this.canvas.onkeypress = this.onKeyPress.bind(this);

	this.canvas.oncontextmenu = function(event) {
			event.preventDefault();
		};

	this.canvas.onblur = function(event) {
			event.target.focus();
		};
}


RemoteDesktopSession.prototype.onOpen = function(open)
{
	console.log('open:', open);
	this.init();
}


RemoteDesktopSession.prototype.onMessage = function(message)
{
	var data = message.data;
	if (this.messageRemainder) {
		var combined = new Uint8Array(this.messageRemainder.byteLength
			+ data.byteLength);
		combined.set(new Uint8Array(this.messageRemainder), 0);
		combined.set(new Uint8Array(data), this.messageRemainder.byteLength);
		data = combined;

		this.messageRemainder = null;
	} else
		data = new Uint8Array(data);

	var byteOffset = 0;
	while (true) {
		try {
			if (!this.receiveMessage.attach(data, byteOffset))
				break;
		} catch (exception) {
			// Discard everything and hope for the best.
			console.error('stream invalid, discarding everything', exception,
				this.receiveMessage, data, byteOffset);
			return;
		}

		try {
			this.messageReceived(this.receiveMessage, this.sendMessage);
		} catch (exception) {
			console.error('exception during message processing:', exception);
		}

		byteOffset += this.receiveMessage.size();
	}

	if (data.byteLength > byteOffset)
		this.messageRemainder = data.slice(byteOffset);
}


RemoteDesktopSession.prototype.messageReceived = function(remoteMessage, reply)
{
	switch (remoteMessage.code()) {
		case RP_INIT_CONNECTION:
			console.log('init connection reply');
			this.sendMessage.start(RP_UPDATE_DISPLAY_MODE);
			this.sendMessage.dataView.writeUint32(this.canvas.width);
			this.sendMessage.dataView.writeUint32(this.canvas.height);
			this.sendMessage.flush();

			this.sendMessage.start(RP_GET_SYSTEM_PALETTE);
			this.sendMessage.flush();
			break;

		case RP_GET_SYSTEM_PALETTE_RESULT:
			var count = remoteMessage.dataView.readUint32();
			gSystemPalette = new Uint32Array(count);

			var color = new RemoteColor();
			for (var i = 0; i < gSystemPalette.length; i++)
				gSystemPalette[i] = color.readFrom(remoteMessage).toUint32();

			break;

		case RP_CREATE_STATE:
			var token = remoteMessage.dataView.readInt32();
			console.log('create state: ' + token);

			if (this.states.hasOwnProperty(token))
				console.error('create state for existing token: ' + token);

			this.states[token] = new RemoteState(this, token);
			break;

		case RP_DELETE_STATE:
			var token = remoteMessage.dataView.readInt32();
			console.log('delete state: ' + token);

			if (!this.states.hasOwnProperty(token)) {
				console.error('delete state for unknown token: ' + token);
				break;
			}

			delete this.states[token];
			break;

		case RP_INVALIDATE_RECT:
		case RP_INVALIDATE_REGION:
			break;

		case RP_SET_CURSOR:
			this.cursorHotspot = new RemotePoint(remoteMessage);
			var bitmap = new RemoteBitmap(remoteMessage);

			bitmap.canvas.style.position = 'absolute';
			if (this.cursorCanvas)
				this.cursorCanvas.remove();

			this.cursorCanvas = bitmap.canvas;
			this.cursorCanvas.style.pointerEvents = 'none';
			this.container.appendChild(this.cursorCanvas);
			this.container.style.cursor = 'none';
			this.updateCursor();
			break;

		case RP_MOVE_CURSOR_TO:
			this.cursorPosition.x = remoteMessage.dataView.readFloat32();
			this.cursorPosition.y = remoteMessage.dataView.readFloat32();
			this.updateCursor();
			break;

		case RP_SET_CURSOR_VISIBLE:
			this.cursorVisible = remoteMessage.dataView.readUint8();
			if (this.cursorCanvas) {
				this.cursorCanvas.style.visibility
					= this.cursorVisible ? 'visible' : 'hidden';
			}
			break;

		case RP_COPY_RECT_NO_CLIPPING:
			var xOffset = remoteMessage.dataView.readInt32();
			var yOffset = remoteMessage.dataView.readInt32();
			var rect = new RemoteRect(remoteMessage);

			var imageData = this.context.getImageData(rect.left, rect.top,
				rect.width(), rect.height());
			this.context.putImageData(imageData, rect.left + xOffset,
				rect.top + yOffset);
			break;

		case RP_FILL_REGION_COLOR_NO_CLIPPING:
			this.removeClipping();
			this.context.currentToken = -1;
			this.context.resetTransform();
			this.context.globalCompositeOperation = 'source-over';

			var rectCount = remoteMessage.dataView.readUint32();
			var rects = new Array(rectCount);
			for (var i = 0; i < rectCount; i++)
				rects[i] = new RemoteRect(remoteMessage);

			this.context.fillStyle = remoteMessage.readColor();

			for (var i = 0; i < rectCount; i++)
				rects[i].apply(this.context.fillRect.bind(this.context));

			break;

		default:
			var token = remoteMessage.dataView.readInt32();
			if (!this.states.hasOwnProperty(token)) {
				console.warn('no state for token: ' + token);
				this.states[token] = new RemoteState(this, token);
			}

			this.states[token].messageReceived(remoteMessage, reply);
			break;
	}
}


RemoteDesktopSession.prototype.onError = function(error)
{
	console.log('websocket error:', error);
	this.onDisconnect(error);
}


RemoteDesktopSession.prototype.onClose = function(close)
{
	console.log('websocket close:', close);
	this.onDisconnect(close);
}


RemoteDesktopSession.prototype.onDisconnect = function(reason)
{
	this.container.remove();
	if (this.disconnectCallback)
		this.disconnectCallback(reason);
}


RemoteDesktopSession.prototype.applyClipping = function(clipRects)
{
	this.removeClipping();

	if (!clipRects || clipRects.length == 0)
		return;

	this.context.save();
	this.context.beginPath();

	this.context.save();
	this.context.lineJoin = 'miter';
	this.context.miterLimit = 10;

	for (var i = 0; i < clipRects.length; i++)
		clipRects[i].apply(this.context.rect.bind(this.context));

	this.context.restore();

	this.context.clip();
	this.clippingApplied = true;
}


RemoteDesktopSession.prototype.removeClipping = function()
{
	if (!this.clippingApplied)
		return;

	this.context.restore();
}


RemoteDesktopSession.prototype.init = function()
{
	this.sendMessage.start(RP_INIT_CONNECTION);
	this.sendMessage.flush();
}


RemoteDesktopSession.prototype.updateCursor = function()
{
	if (!this.cursorVisible || !this.cursorCanvas)
		return;

	this.cursorCanvas.style.left
		= (this.cursorPosition.x - this.cursorHotspot.x) + 'px';
	this.cursorCanvas.style.top
		= (this.cursorPosition.y - this.cursorHotspot.y) + 'px';
}


RemoteDesktopSession.prototype.onMouseMove = function(event)
{
	this.sendMessage.start(RP_MOUSE_MOVED);
	this.sendMessage.dataView.writeFloat32(event.offsetX);
	this.sendMessage.dataView.writeFloat32(event.offsetY);
	this.sendMessage.flush();
	event.preventDefault();
}


RemoteDesktopSession.prototype.onMouseDown = function(event)
{
	this.canvas.focus();
	this.sendMessage.start(RP_MOUSE_DOWN);
	this.sendMessage.dataView.writeFloat32(event.offsetX);
	this.sendMessage.dataView.writeFloat32(event.offsetY);
	this.sendMessage.dataView.writeUint32(event.buttons);
	this.sendMessage.dataView.writeUint32(event.detail);
	this.sendMessage.flush();
	event.preventDefault();
}


RemoteDesktopSession.prototype.onMouseUp = function(event)
{
	this.sendMessage.start(RP_MOUSE_UP);
	this.sendMessage.dataView.writeFloat32(event.offsetX);
	this.sendMessage.dataView.writeFloat32(event.offsetY);
	this.sendMessage.dataView.writeUint32(event.buttons);
	this.sendMessage.flush();
	event.preventDefault();
}


RemoteDesktopSession.prototype.onKeyDownUp = function(event)
{
	var keyDown = event.type === 'keydown';
	var lockModifier = false;
	var modifiersChanged = 0;
	switch (event.code) {
		case 'ShiftLeft':
			modifiersChanged |= B_LEFT_SHIFT_KEY;
			if (event.shiftKey == keyDown)
				modifiersChanged |= B_SHIFT_KEY;
			break;

		case 'ShiftRight':
			modifiersChanged |= B_RIGHT_SHIFT_KEY;
			if (event.shiftKey == keyDown)
				modifiersChanged |= B_SHIFT_KEY;
			break;

		case 'ControlLeft':
			modifiersChanged |= B_LEFT_CONTROL_KEY;
			if (event.ctrlKey == keyDown)
				modifiersChanged |= B_CONTROL_KEY;
			break;

		case 'ControlRight':
			modifiersChanged |= B_RIGHT_CONTROL_KEY;
			if (event.ctrlKey == keyDown)
				modifiersChanged |= B_CONTROL_KEY;
			break;

		case 'AltLeft':
			modifiersChanged |= B_LEFT_COMMAND_KEY;
			if (event.altKey == keyDown)
				modifiersChanged |= B_COMMAND_KEY;
			break;

		case 'AltRight':
			modifiersChanged |= B_RIGHT_COMMAND_KEY;
			if (event.altKey == keyDown)
				modifiersChanged |= B_COMMAND_KEY;
			break;

		case 'ContextMenu':
			modifiersChanged |= B_MENU_KEY;
			break;

		case 'CapsLock':
			modifiersChanged |= B_CAPS_LOCK;
			lockModifier = true;
			break;

		case 'ScrollLock':
			modifiersChanged |= B_SCROLL_LOCK;
			lockModifier = true;
			break;

		case 'NumLock':
			modifiersChanged |= B_NUM_LOCK;
			lockModifier = true;
			break;
	}

	if (modifiersChanged != 0) {
		if (lockModifier) {
			if (((this.modifiers & modifiersChanged) == 0) == keyDown)
				this.modifiers ^= modifiersChanged;
		} else {
			if (keyDown)
				this.modifiers |= modifiersChanged;
			else
				this.modifiers &= ~modifiersChanged;
		}

		this.sendMessage.start(RP_MODIFIERS_CHANGED);
		this.sendMessage.dataView.writeUint32(this.modifiers);
		this.sendMessage.flush();
		event.preventDefault();
		return;
	}

	this.sendMessage.start(keyDown ? RP_KEY_DOWN : RP_KEY_UP);
	if (event.key.length == 1)
		this.sendMessage.dataView.writeString(event.key);
	else {
		this.sendMessage.dataView.writeUint32(1);
		this.sendMessage.dataView.writeUint8(event.keyCode);
	}

	if (event.keyCode) {
		this.sendMessage.dataView.writeUint32(0);
		this.sendMessage.dataView.writeUint32(event.keyCode);
	}

	this.sendMessage.flush();
	event.preventDefault();
}


RemoteDesktopSession.prototype.onKeyPress = function(event)
{
	this.sendMessage.start(RP_KEY_DOWN);
	this.sendMessage.dataView.writeUint32(1);
	this.sendMessage.dataView.writeUint8(event.which);
	this.sendMessage.flush();
	this.sendMessage.start(RP_KEY_UP);
	this.sendMessage.dataView.writeUint32(1);
	this.sendMessage.dataView.writeUint8(event.which);
	this.sendMessage.flush();
	event.preventDefault();
}


RemoteDesktopSession.prototype.onWheel = function(event)
{
	this.sendMessage.start(RP_MOUSE_WHEEL_CHANGED);
	this.sendMessage.dataView.writeFloat32(event.deltaX);
	this.sendMessage.dataView.writeFloat32(event.deltaY);
	this.sendMessage.flush();
	event.preventDefault();
}


function init()
{
	var targetAddressInput = document.querySelector('#targetAddress');
	var widthInput = document.querySelector('#width');
	var heightInput = document.querySelector('#height');

	if (localStorage.targetAddress)
		targetAddressInput.value = localStorage.targetAddress;
	if (localStorage.width)
		widthInput.value = localStorage.width;
	if (localStorage.height)
		heightInput.value = localStorage.height;

	var onDisconnect = function(reason) {
			document.body.classList.remove('connect');
			gSession = undefined;
		};

	document.querySelector('#connectButton').onclick = function() {
			document.body.classList.add('connect');

			localStorage.width = widthInput.value;
			localStorage.height = heightInput.value;
			localStorage.targetAddress = targetAddressInput.value;

			gSession = new RemoteDesktopSession(document.body, widthInput.value,
				heightInput.value, targetAddressInput.value, onDisconnect);
		};
}
