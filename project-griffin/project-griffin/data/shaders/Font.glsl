#ifdef _VERTEX_
 
	attribute vec4 coord;
	varying vec2 texcoord;
 
	void main(void) {
		gl_Position = vec4(coord.xy, 0, 1);
		texcoord = coord.zw;
	}

#endif

#ifdef _FRAGMENT_

	varying vec2 texcoord;
	uniform sampler2D tex;
	uniform vec4 color;
 
	void main(void) {
		gl_FragColor = vec4(1, 1, 1, texture2D(tex, texcoord).r) * color;
	}

#endif