/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 * Portional Copyright (c) 1999 Cameron Grant <gandalf@vilnya.demon.co.uk>
 * 
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
#include "sis7018.h"
#include "sis7018hw.h"
#include "t4dwave.h"
#include "ac97.h"
//#include "pcm.h"

#define TR_TIMEOUT_CDC  0xffff

//typedef bool (*ac97init_proc)(sis7018_dev *dev, bool bOn);
/* init functions*/
status_t ac97init_CS4299(sis7018_dev *dev, bool bOn);
status_t ac97init_default(sis7018_dev *dev, bool bOn);

static struct ac97_codecid
{
  uint32 id;
  int noext;
  char *name;
  status_t (*ac97init)(sis7018_dev *dev, bool bOn);
} ac97codecid[] = {
  { 0x41445303, 0, "Analog Devices AD1819", NULL },
  { 0x41445340, 0, "Analog Devices AD1881", NULL },
  { 0x41445348, 0, "Analog Devices AD1881A", NULL },
  { 0x41445360, 0, "Analog Devices AD1885", NULL },
  { 0x414b4d00, 1, "Asahi Kasei AK4540", NULL },
  { 0x414b4d01, 1, "Asahi Kasei AK4542", NULL },
  { 0x414b4d02, 1, "Asahi Kasei AK4543", NULL },
  { 0x414c4710, 0, "Avance Logic ALC200/200P", NULL },
  { 0x43525900, 0, "Cirrus Logic CS4297", NULL },
  { 0x43525903, 0, "Cirrus Logic CS4297", NULL },
  { 0x43525913, 0, "Cirrus Logic CS4297A", NULL },
  { 0x43525914, 0, "Cirrus Logic CS4297B", NULL },
  { 0x43525923, 0, "Cirrus Logic CS4294C", NULL },
  { 0x4352592b, 0, "Cirrus Logic CS4298C", NULL },
  { 0x43525931, 0, "Cirrus Logic CS4299A", ac97init_CS4299 },
  { 0x43525933, 0, "Cirrus Logic CS4299C", ac97init_CS4299 },
  { 0x43525934, 0, "Cirrus Logic CS4299D", ac97init_CS4299 },
  { 0x43525941, 0, "Cirrus Logic CS4201A", NULL },
  { 0x43525951, 0, "Cirrus Logic CS4205A", NULL },
  { 0x43525961, 0, "Cirrus Logic CS4291A", NULL },
  { 0x45838308, 0, "ESS Technology ES1921", NULL },
  { 0x49434511, 0, "ICEnsemble ICE1232", NULL },
  { 0x4e534331, 0, "National Semiconductor LM4549", NULL },
  { 0x83847600, 0, "SigmaTel STAC9700/9783/9784", NULL },
  { 0x83847604, 0, "SigmaTel STAC9701/9703/9704/9705", NULL },
  { 0x83847605, 0, "SigmaTel STAC9704", NULL },
  { 0x83847608, 0, "SigmaTel STAC9708/9711", NULL },
  { 0x83847609, 0, "SigmaTel STAC9721/9723", NULL },
  { 0x83847644, 0, "SigmaTel STAC9744", NULL },
  { 0x83847656, 0, "SigmaTel STAC9756/9757", NULL },
  { 0x53494c22, 0, "Silicon Laboratory Si3036", NULL },
  { 0x53494c23, 0, "Silicon Laboratory Si3038", NULL },
  { 0x54524103, 0, "TriTech TR?????", NULL },
  { 0x54524106, 0, "TriTech TR28026", NULL },
  { 0x54524108, 0, "TriTech TR28028", NULL },
  { 0x54524123, 0, "TriTech TR28602", NULL },
  { 0x574d4c00, 0, "Wolfson WM9701A", NULL },
  { 0x574d4c03, 0, "Wolfson WM9703/9704", NULL },
  { 0x574d4c04, 0, "Wolfson WM9704 (quad)", NULL },
  { 0, 0, NULL }
};

static char *ac97enhancement[] = {
  "no 3D Stereo Enhancement",
  "Analog Devices Phat Stereo",
  "Creative Stereo Enhancement",
  "National Semi 3D Stereo Enhancement",
  "Yamaha Ymersion",
  "BBE 3D Stereo Enhancement",
  "Crystal Semi 3D Stereo Enhancement",
  "Qsound QXpander",
  "Spatializer 3D Stereo Enhancement",
  "SRS 3D Stereo Enhancement",
  "Platform Tech 3D Stereo Enhancement",
  "AKM 3D Audio",
  "Aureal Stereo Enhancement",
  "Aztech 3D Enhancement",
  "Binaura 3D Audio Enhancement",
  "ESS Technology Stereo Enhancement",
  "Harman International VMAx",
  "Nvidea 3D Stereo Enhancement",
  "Philips Incredible Sound",
  "Texas Instruments 3D Stereo Enhancement",
  "VLSI Technology 3D Stereo Enhancement",
  "TriTech 3D Stereo Enhancement",
  "Realtek 3D Stereo Enhancement",
  "Samsung 3D Stereo Enhancement",
  "Wolfson Microelectronics 3D Enhancement",
  "Delta Integration 3D Enhancement",
  "SigmaTel 3D Enhancement",
  "KS Waves",
  "Rockwell 3D Stereo Enhancement",
  "Reserved 29",
  "Reserved 30",
  "Reserved 31"
};

static char *ac97feature[] = {
  "mic channel",
  "reserved",
  "tone",
  "simulated stereo",
  "headphone",
  "bass boost",
  "18 bit DAC",
  "20 bit DAC",
  "18 bit ADC",
  "20 bit ADC"
};

static char *ac97extfeature[] = {
  "variable rate PCM",
  "double rate PCM",
  "reserved 1",
  "variable rate mic",
  "reserved 2",
  "reserved 3",
  "center DAC",
  "surround DAC",
  "LFE DAC",
  "AMAP",
  "reserved 4",
  "reserved 5",
  "reserved 6",
  "reserved 7",
};

int ac97read(sis7018_dev *dev, int regno)
{
  cpu_status cp;
  int i, j, reg, rw;
  if(b_trace_ac97)
    TRACE("  ac97read: regno=%x\n", regno);
  
  switch (dev->type->ids.chip_id){
  case SPA_PCI_ID:
    reg=SPA_REG_CODECRD;
    rw=SPA_CDC_RWSTAT;
    break;
  case ALI_PCI_ID:
		if(dev->info.revision > 0x01)
		  reg=TDX_REG_CODECWR;
		else
		  reg=TDX_REG_CODECRD;
		rw=TDX_CDC_RWSTAT;
		break; 
  case TDX_PCI_ID:
  case xDX_PCI_ID:
    reg=TDX_REG_CODECRD;
    rw=TDX_CDC_RWSTAT;
    break;
  case TNX_PCI_ID:
    reg=(regno & 0x100)? TNX_REG_CODEC2RD : TNX_REG_CODEC1RD;
    rw=TNX_CDC_RWSTAT;
    break;
  default:
    TRACE("ac97read defaulted !!!\n");
    return ENODEV;
  }
  
  cp = disable_interrupts();
  acquire_spinlock(&dev->hardware);

  regno &= 0x7f;

  if(dev->type->ids.chip_id == ALI_PCI_ID){
		j = rw;
    for (i=TR_TIMEOUT_CDC; (i > 0) && (j & rw); i--){
			j = hw_read(dev, reg, 4);
    }
		if(i > 0){
			uint32 c1 = hw_read(dev, 0xc8, 4);
			uint32 c2 = hw_read(dev, 0xc8, 4);
      for(i=TR_TIMEOUT_CDC; (i > 0) && (c1 == c2); i--){
				c2 = hw_read(dev, 0xc8, 4);
      }
		}
  } 
  if(dev->type->ids.chip_id != ALI_PCI_ID || i > 0){
	  hw_write(dev, reg, regno | rw, 4);
    j=rw;
    for (i=TR_TIMEOUT_CDC; (i > 0) && (j & rw); i--){  
      j=hw_read(dev, reg, 4);
    }
  }

  release_spinlock(&dev->hardware);
  restore_interrupts(cp);
  
  if (i == 0)
    TRACE_ALWAYS("ac97 codec timeout during read of register %x\n", regno);
  if(b_trace_ac97)
    TRACE("  ac97read: regno=%x ret = %x\n", regno, j);
  return (j >> TR_CDC_DATA) & 0xffff;
}

status_t ac97write(sis7018_dev *dev, int regno, uint32 data)
{
  cpu_status cp;
  int i, j, reg, rw;
 
  if(b_trace_ac97)
    TRACE("\nac97write: regno=%x, data=%lx\n", regno, data);
  
  switch (dev->type->ids.chip_id){
  case SPA_PCI_ID:
    reg=SPA_REG_CODECWR;
    rw=SPA_CDC_RWSTAT;
    break;
  case ALI_PCI_ID:
  case TDX_PCI_ID:
  case xDX_PCI_ID:
    reg=TDX_REG_CODECWR;
    rw=TDX_CDC_RWSTAT;
    break;
  case TNX_PCI_ID:
    reg=TNX_REG_CODECWR;
    rw=TNX_CDC_RWSTAT | ((regno & 0x100)? TNX_CDC_SEC : 0);
    break;
  default:
    TRACE("ac97write defaulted !!!");
    return ENODEV;
  }
  
  regno &= 0x7f;
  if(b_trace_ac97)
    TRACE("ac97write: reg %x was %x\n", regno, ac97read(dev, regno));
  
  cp = disable_interrupts();
  acquire_spinlock(&dev->hardware);
  j=rw;

  if(dev->type->ids.chip_id == ALI_PCI_ID){
    for(i = TR_TIMEOUT_CDC; (i > 0) && (j & rw); i--){
			j = hw_read(dev, reg, 4);
    }
		if(i > 0) {
			uint32 c1 = hw_read(dev, 0xc8, 4);
			uint32 c2 = hw_read(dev, 0xc8, 4);
			for (i = TR_TIMEOUT_CDC; (i > 0) && (c1 == c2);	i--)
				c2 = hw_read(dev, 0xc8, 4);
		}
  } 
  if(dev->type->ids.chip_id != ALI_PCI_ID || i > 0){
    for(i=TR_TIMEOUT_CDC; (i>0) && (j & rw); i--){
      j=hw_read(dev, reg, 4);
    }
		if(dev->type->ids.chip_id == ALI_PCI_ID && dev->info.revision > 0x01)
		  rw |= 0x0100;
    hw_write(dev, reg, (data << TR_CDC_DATA) | regno | rw, 4);
  }

  release_spinlock(&dev->hardware);
  restore_interrupts(cp);

  if(b_trace_ac97)
    TRACE("ac97write: wrote %x, now %x\n", data, ac97read(dev, regno));
  if (i==0)
    TRACE_ALWAYS("ac97 codec timeout writing %x, data %lx\n", regno, data);
  if(b_trace_ac97)
    TRACE("ac97write: ret = %x\n\n", i);
  return (i > 0)? B_OK : B_TIMED_OUT;
}

status_t ac97init(sis7018_dev *card)
{
//  cpu_status cp;
  status_t err=B_OK;
  int i, /*j,*/ caps, se, noext, extcaps, extid, extstat, reg=0;
  int32 id;
/* old manner
  err |= ac97write(card, AC97_REG_POWER, 0);
  err |= ac97write(card, AC97_REG_RESET, 0);
  
  err |= ac97write(card, AC97_MIX_PCM, 0x707);
  err |= ac97write(card, AC97_MIX_BEEP, 0x06);
  err |= ac97write(card, AC97_MIX_MIC, 0x8000);
  err |= ac97write(card, AC97_MIX_PHONE, 0x707);
  err |= ac97write(card, AC97_MIX_PHONES, 0x707);
*/

 //BSD init manner 
  err |= ac97write(card, AC97_REG_POWER, 0x0000);
  err |= ac97write(card, AC97_REG_RESET, 0);
  snooze(100000);
  //snooze(50000);
  err |= ac97write(card, AC97_REG_POWER, 0x0000);
  err |= ac97write(card, AC97_REG_GEN, 0x0000);

/* Marcus's init manner*/
//  err |= ac97write(card, AC97_REG_RESET, 0);
//  snooze(50000); // 50 ms

  if (err != B_OK){
    TRACE("ac97 codec resetting failed: %lx\n", err);
    return ENODEV;
  }

//  cp = disable_interrupts();
//  acquire_spinlock(&card->hardware);

  i = ac97read(card, AC97_REG_RESET);
  caps = i & 0x03ff;
  se =  (i & 0x7c00) >> 10;

  id = (ac97read(card, AC97_REG_ID1) << 16) | ac97read(card, AC97_REG_ID2);
//  codec->rev = id & 0x000000ff;

//  release_spinlock(&card->hardware);
//  restore_interrupts(cp);
#if 1
  if (id == 0 || id == 0xffffffff){
    TRACE("ac97 codec invalid or not present (id == %lx)\n", id);
    return ENODEV;
  }
#endif
  noext = 0;
  for (i = 0; ac97codecid[i].id; i++){
    if (ac97codecid[i].id == id){
      status_t err = B_OK;
      noext = ac97codecid[i].noext;
      if (ac97codecid[i].name)
         TRACE("\nac97 codec id 0x%08lx (%s)\n", id, ac97codecid[i].name);
       if(ac97codecid[i].ac97init)
         err |= (*ac97codecid[i].ac97init)(card, true);
       else
         err |= ac97init_default(card, true);
       if(err != B_OK)
         TRACE("ac97 codec initializatin error\n");
    }
  }
  
  TRACE("ac97 codec features:\n");
  
    for (i = 0; i < 10; i++)
      if (caps & (1 << i))
        TRACE("> %s\n", ac97feature[i]);
    TRACE("> %s\n", ac97enhancement[se]);
  TRACE("\n");
  
  extcaps = 0;
  extid = 0;
  extstat = 0;
  if (!noext){
    i = ac97read(card, AC97_REGEXT_ID);
    if (i != 0xffff){
      extcaps = i & 0x3fff;
      extid =  (i & 0xc000) >> 14;
      extstat = ac97read(card, AC97_REGEXT_STAT) & AC97_EXTCAPS;
    }
  }

    if (extcaps != 0 || extid){
      TRACE("\nac97 %s codec %s:\n", extid? "secondary" : "primary",
                                   extcaps? "extended features":"");
      for (i = 0; i < 14; i++)
        if (extcaps & (1 << i))
          TRACE("> %s\n", ac97extfeature[i]);
      TRACE("\n"); 
    }

  if ((ac97read(card, AC97_REG_POWER) & 2) == 0)
    TRACE("ac97 codec reports dac not ready\n");

// some addition
 
  reg = ac97read(card, AC97_MIX_MASTER);
  err |= ac97write(card, AC97_MIX_MASTER, 0x3f3f);
  if(0x3f3f == ac97read(card, AC97_MIX_MASTER)){
    TRACE("AC97 master gain is wide!\n");
    card->ac97.wide_main_gain = true;
  }
  err |= ac97write(card, AC97_MIX_MASTER, reg);
 
// BSD manner
  err |= ac97write(card, AC97_MIX_PCM, 0x707);
//  err |= ac97write(card, AC97_MIX_BEEP, 0x06);
//  err |= ac97write(card, AC97_MIX_MIC, 0x8000);
//  err |= ac97write(card, AC97_MIX_PHONE, 0x707);
//  err |= ac97write(card, AC97_MIX_PHONES, 0x707);

/*Marcus's manner*/
 // err |= ac97write(card, AC97_MIX_MASTER, 0x00);
 // err |= ac97write(card, AC97_MIX_PCM, 0x404);

  return err;
}

status_t ac97get_params(sis7018_dev *dev, sound_setup *sound)
{
  uint16 reg = 0;
  int main_gain_multiplier = dev->ac97.wide_main_gain ? 1 : 2;
     
  sound->sample_rate = kHz_48_0;
  sound->dither_enable = false;
  sound->output_boost = 0;
  sound->highpass_enable = 0;
      
  reg=ac97read(dev, AC97_MIX_MONO);
  sound->mono_gain = (char)reg;
  sound->mono_mute = (char)(reg & AC97_MUTE) ? 1 : 0;

  reg=ac97read(dev, AC97_REG_RECSEL);
  switch (reg&0x07){
  case 4:  sound->right.adc_source = line; break;
  case 3:  sound->right.adc_source = aux1; break;
  case 0:  sound->right.adc_source = mic; break;
  default: sound->right.adc_source = loopback; break;
  }
      
  switch ((reg >> 8) & 0x07){
  case 4:  sound->left.adc_source = line; break;
  case 3:  sound->left.adc_source = aux1; break;
  case 0:  sound->left.adc_source = mic; break;
  default: sound->left.adc_source = loopback; break;
  }

  reg = ac97read(dev, AC97_MIX_MIC);
  sound->loop_attn = (reg & 0x1f)*2;
  sound->loop_enable = (reg & AC97_MUTE) ? 0 : 1;
  sound->left.mic_gain_enable = 
  sound->right.mic_gain_enable = (reg >> 6) & 0x01;

  reg = ac97read(dev, AC97_MIX_RGAIN);
  sound->left.adc_gain = (reg >> 0x8) & 0xf;
  sound->right.adc_gain  = reg & 0xf;
  
  reg = ac97read(dev, AC97_MIX_CD);
  sound->left.aux1_mix_gain = ((reg >> 8) & 0x1f);
  sound->right.aux1_mix_gain = (reg & 0x1f);
  sound->left.aux1_mix_mute =  
  sound->right.aux1_mix_mute = (reg & AC97_MUTE) ? 1 : 0;
      
  reg = ac97read(dev, AC97_MIX_AUX);
  sound->left.aux2_mix_gain = ((reg >> 8) & 0x1f);
  sound->right.aux2_mix_gain = (reg & 0x1f);
  sound->left.aux2_mix_mute =  
  sound->right.aux2_mix_mute = (reg & AC97_MUTE) ? 1 : 0;
      
        
  reg = ac97read(dev, AC97_MIX_LINE);
  sound->left.line_mix_gain = ((reg >> 8) & 0x1f);
  sound->right.line_mix_gain = (reg & 0x1f);
  sound->left.line_mix_mute =  
  sound->right.line_mix_mute = (reg & AC97_MUTE) ? 1 : 0;
      
  reg = ac97read(dev, AC97_MIX_MASTER);
  sound->left.dac_attn = ((reg >> 8) & 0x1f) * main_gain_multiplier;
  sound->right.dac_attn = (reg & 0x1f) * main_gain_multiplier;
  sound->left.dac_mute =  
  sound->right.dac_mute = (reg & AC97_MUTE) ? 1 : 0;  
      
  return B_OK;
}

status_t ac97set_params(sis7018_dev *dev, sound_setup * sound)
{
  status_t err = B_OK;
  uint16 reg = 0;
  int main_gain_divider = dev->ac97.wide_main_gain ? 1 : 2;

#if 0 //DEBUG
  TRACE("\nsound setup (left/right)\n");
  TRACE("adc_source %x/%x\n", sound->left.adc_source, sound->right.adc_source);
  TRACE("adc_gain %x/%x\n", sound->left.adc_gain, sound->right.adc_gain);
  TRACE("mic_gain_enable %x/%x\n", sound->left.mic_gain_enable, sound->right.mic_gain_enable);
  TRACE("aux1_mix_gain %x/%x\n", sound->left.aux1_mix_gain, sound->right.aux1_mix_gain);
  TRACE("aux1_mix_mute %x/%x\n", sound->left.aux1_mix_mute, sound->right.aux1_mix_mute);
  TRACE("aux2_mix_gain %x/%x\n", sound->left.aux2_mix_gain, sound->right.aux2_mix_gain);
  TRACE("aux2_mix_mute %x/%x\n", sound->left.aux2_mix_mute, sound->right.aux2_mix_mute);
  TRACE("line_mix_gain %x/%x\n", sound->left.line_mix_gain, sound->right.line_mix_gain);
  TRACE("line_mix_mute %x/%x\n", sound->left.line_mix_mute, sound->right.line_mix_mute);
  TRACE("dac_attn %x/%x\n", sound->left.dac_attn, sound->right.dac_attn);
  TRACE("dac_mute %x/%x\n", sound->left.dac_mute, sound->right.dac_mute);
  TRACE("sound setup\n");
  TRACE("sample_rate %x\n", sound->sample_rate);
  TRACE("playback_format %x\n", sound->playback_format);
  TRACE("capture_format %x\n", sound->capture_format);
  TRACE("dither_enable %x\n", sound->dither_enable);
  TRACE("loop_attn %x\n", sound->loop_attn);
  TRACE("loop_enable %x\n", sound->loop_enable);
  TRACE("output_boost %x\n", sound->output_boost);
  TRACE("highpass_enable %x\n", sound->highpass_enable);
  TRACE("mono_gain %x\n", sound->mono_gain);
  TRACE("mono_mute %x\n", sound->mono_mute);
#endif
//TRACE("sample-rate:%d\n", sound->sample_rate);
  sound->sample_rate = kHz_48_0;
  sound->dither_enable = false;
  sound->output_boost = 0;
  sound->highpass_enable = 0;

  reg = sound->mono_gain | ((sound->mono_mute) ? AC97_MUTE : 0);  
  err |= ac97write(dev, AC97_MIX_MONO, reg);
    
  reg = sound->loop_attn / 2;
  reg |= ((sound->left.mic_gain_enable || sound->right.mic_gain_enable) ? 0x01 : 0x00) << 6;
  reg |= (sound->loop_enable) ? 0 : AC97_MUTE;  
  err |= ac97write(dev, AC97_MIX_MIC, reg);
      
  reg = 0;
  switch(sound->left.adc_source){
  case line: reg = 4; break;
  case aux1: reg = 3; break;
  case mic:  reg = 0; break;
  default:   reg = 0; break; 
  }
      
  reg <<= 8;
  switch(sound->right.adc_source){
  case line: reg |= 4; break;
  case aux1: reg |= 3; break;
  case mic:  reg |= 0; break;
  default:   reg |= 0; break; 
  }
      
  err |= ac97write(dev, AC97_REG_RECSEL, reg);

  reg = (sound->left.adc_gain) << 8;
  reg |= sound->right.adc_gain;
  err |= ac97write(dev, AC97_MIX_RGAIN, reg);

  reg = (sound->left.aux1_mix_gain) << 0x08;
  reg |= sound->right.aux1_mix_gain;
  reg |= (sound->left.aux1_mix_mute || sound->right.aux1_mix_mute) ? AC97_MUTE : 0 ; 
  err |= ac97write(dev, AC97_MIX_CD, reg);

  reg = (sound->left.aux2_mix_gain) << 0x08;
  reg |= sound->right.aux2_mix_gain;
  reg |= (sound->left.aux2_mix_mute || sound->right.aux2_mix_mute) ? AC97_MUTE : 0 ; 
  err |= ac97write(dev, AC97_MIX_AUX, reg);
      
  reg = (sound->left.line_mix_gain) << 0x08;
  reg |= sound->right.line_mix_gain;
  reg |= (sound->left.line_mix_mute || sound->right.line_mix_mute) ? AC97_MUTE : 0 ; 
  err |= ac97write(dev, AC97_MIX_LINE, reg);

  reg = ((sound->left.dac_attn) / main_gain_divider) << 0x08;
  reg |= (sound->right.dac_attn) / main_gain_divider;
  reg |= (sound->left.dac_mute || sound->right.dac_mute) ? AC97_MUTE : 0 ; 
  err |= ac97write(dev, AC97_MIX_MASTER, reg);

  return err;      
}

status_t ac97init_default(sis7018_dev *dev, bool bOn)
{
  status_t err = B_OK;
  //TRACE("powerdown register was = %#04x\n",ich_codec_read(AC97_POWERDOWN)));
//  if (bOn)
//    ac97write(dev, AC97_REG_POWER, ac97read(dev, AC97_REG_POWER) & ~0x8000); /* switch on (low active) */
//  else
//    ac97write(dev, AC97_REG_POWER, ac97read(dev, AC97_REG_POWER) | 0x8000); /* switch off */
  //LOG(("powerdown register is = %#04x\n",ich_codec_read(AC97_POWERDOWN)));
  return err;
}

status_t ac97init_CS4299(sis7018_dev *dev, bool bOn)
{
  status_t err = B_OK;
  TRACE("Use CS4299 init\n");
 // if (bOn)
//    err |= ac97write(dev, 0x68, 0x8004);
//  else
//    err |= ac97write(dev, 0x68, 0);
    return err;
}

#if DEBUG
static struct
{
  int16 reg;
  char  name[DEVNAME];  
} ac97_regs[]=
{
  {AC97_REG_RESET, "Reset"},           {AC97_MIX_MASTER, "Master Volume"}, 
  {AC97_MIX_PHONES, "AUX Out volume"}, {AC97_MIX_MONO, "Mono Volume"}, 
  {AC97_MIX_TONE, "Master Tone"},    {AC97_MIX_BEEP, "PC Beep"}, 
  {AC97_MIX_PHONE, "Phone volume"},  {AC97_MIX_MIC, "Mio Volume"}, 
  {AC97_MIX_LINE, "Line In Volume"}, {AC97_MIX_CD, "CD Volume"}, 
  {AC97_MIX_VIDEO, "Video Volume"},  {AC97_MIX_AUX, "AUX In Volume"}, 
  {AC97_MIX_PCM, "PCM Out Volume"},  {AC97_REG_RECSEL, "Record Select"}, 
  {AC97_MIX_RGAIN, "Record Gain"},   {AC97_MIX_MGAIN, "Record Gain Mio"}, 
  {AC97_REG_GEN, "General Purpose"}, {AC97_REG_3D, "3D Control"}, 
  {0x24, "Audio Int & Paging"}, {AC97_REG_POWER, "Powerdown Ctrl/Stat"}, 
  {AC97_REG_ID1, "Vendor ID1"}, {AC97_REG_ID2, "VendorID 2"}, 
};

void ac97dump(int card_id)
{
  int i;
  for(i=0; i < sizeof(ac97_regs)/sizeof(ac97_regs[0]); i++)
    kprintf("%s : %x\n", ac97_regs[i].name, ac97read(&cards[card_id], ac97_regs[i].reg));
}

#endif //DEBUG

