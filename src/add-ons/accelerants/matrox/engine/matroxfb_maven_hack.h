struct matrox_pll_features {
	unsigned int	vco_freq_min;
	unsigned int	ref_freq;
	unsigned int	feed_div_min;
	unsigned int	feed_div_max;
	unsigned int	in_div_min;
	unsigned int	in_div_max;
	unsigned int	post_shift_max;
};

struct matrox_pll_features2 {
	unsigned int	vco_freq_min;
	unsigned int	vco_freq_max;
	unsigned int	feed_div_min;
	unsigned int	feed_div_max;
	unsigned int	in_div_min;
	unsigned int	in_div_max;
	unsigned int	post_shift_max;
};

struct matrox_pll_ctl {
	unsigned int	ref_freq;
	unsigned int	den;
};

struct mavenregs {
	uint8 regs[256];
	int	 mode;
	int	 vlines;
	int	 xtal;
	int	 fv;

	uint16 htotal;
	uint16 hcorr;
};

struct my_timming {
	unsigned int pixclock;
	unsigned int HDisplay;
	unsigned int HSyncStart;
	unsigned int HSyncEnd;
	unsigned int HTotal;
	unsigned int VDisplay;
	unsigned int VSyncStart;
	unsigned int VSyncEnd;
	unsigned int VTotal;
	unsigned int sync;
	int	     dblscan;
	int	     interlaced;
	unsigned int delay;	/* CRTC delay */
};

int matroxfb_PLL_mavenclock(const struct matrox_pll_features2* pll,
		const struct matrox_pll_ctl* ctl,
		unsigned int htotal, unsigned int vtotal,
		unsigned int* in, unsigned int* feed, unsigned int* post,
		unsigned int* h2);

unsigned int matroxfb_mavenclock(const struct matrox_pll_ctl* ctl,
		unsigned int htotal, unsigned int vtotal,
		unsigned int* in, unsigned int* feed, unsigned int* post,
		unsigned int* htotal2);

void DAC1064_calcclock(unsigned int freq, unsigned int fmax,
		unsigned int* in, unsigned int* feed, unsigned int* post);

void maven_init_TVdata ( struct mavenregs* data); 
void maven_init_TV ( const struct mavenregs* m);

int maven_find_exact_clocks(unsigned int ht, unsigned int vt,
		struct mavenregs* m);

int maven_compute_timing ( struct my_timming* mt, struct mavenregs* m) ;


int maven_program_timing ( const struct mavenregs* m); 

int maven_resync();


//MY INTERFACE
int maven_set_mode(int mode);

int maven_out_compute(struct my_timming* mt, struct mavenregs* mr) ;

int maven_out_program(const struct mavenregs* mr) ;

int maven_out_start() ;
