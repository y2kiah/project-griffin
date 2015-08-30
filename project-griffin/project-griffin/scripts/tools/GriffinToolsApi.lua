local ffi = require("ffi")
ffi.cdef[[


	// Functions

	
	uint64_t griffin_tools_importMesh(const char* filename, bool preTransformVertices, bool flipUVs);

	
	bool griffin_tools_saveMesh(uint64_t mesh, const char* filename);

	
	uint64_t griffin_tools_convertMesh(const char* sourceFilename, const char* destFilename,
									   bool preTransformVertices, bool flipUVs);


]]