
struct riff_struct
{ 
	uint32 riff_id; // 'RIFF'
	uint32 len;
	uint32 wave_id;	// 'WAVE'
}; 

struct chunk_struct 
{ 
	uint32 fourcc;
	uint32 len;
};

struct common_struct 
{ 
	uint16 format_tag; 
	uint16 channels; 
	uint32 samples_per_sec; 
	uint32 avg_bytes_per_sec; 
	uint16 block_align; 
	uint16 bits_per_sample;
}; 

struct wave_header
{ 
	riff_struct   riff;
	chunk_struct  format;
	common_struct common;
	chunk_struct  data;
};
