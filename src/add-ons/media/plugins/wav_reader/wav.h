
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

struct format_struct 
{ 
	uint16 format_tag; 
	uint16 channels; 
	uint32 samples_per_sec; 
	uint32 avg_bytes_per_sec; 
	uint16 block_align; 
	uint16 bits_per_sample;
}; 

struct format_struct_extensible
{ 
	uint16 format_tag; // 0xfffe for extensible format
	uint16 channels; 
	uint32 samples_per_sec; 
	uint32 avg_bytes_per_sec; 
	uint16 block_align; 
	uint16 bits_per_sample;
	uint16 ext_size;
	uint16 valid_bits_per_sample;
	uint32 channel_mask;
	uint8  guid[16]; // first two bytes are format code
}; 

struct fact_struct 
{ 
	uint32 sample_length; 
}; 
