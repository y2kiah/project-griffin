varying float depth;

#ifdef _VERTEX_

	void main()
	{
		vec4 viewPos = gl_ModelViewMatrix * gl_Vertex; // this will transform the vertex into eyespace
		depth = (-viewPos.z-near) / (far-near); // will map near..far to 0..1
		gl_Position = ftransform();
	}

#endif

#ifdef _FRAGMENT_

	void main()
	{
		gl_FragColor.r = depth;
	}

#endif