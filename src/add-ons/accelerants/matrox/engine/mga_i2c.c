/*
 * i2c interface for the G400 MAVEN under BeOS
 *
 * Provides I2CR,I2CW - functions to parallel DACW,DACR
 * Bus should be run at max. 100kHz: see original Philips I2C specification
 *	
 * Much help was provided by observing the Linux i2c code, 
 * so thanks go to: Gerd Knorr
 *
 * Other authors:
 * Mark Watson 6/2000,
 * Rudolf Cornelissen 12/2002
 */

#define MODULE_BIT 0x00004000

#include "mga_std.h"

/*which device on the bus is the MAVEN?*/
#define MAVEN_WRITE (0x1B<<1)
#define MAVEN_READ ((0x1B<<1)|1)

#define I2C_CLOCK 0x20
#define I2C_DATA 0x10

/* MGA-TVO I2C for G200, G400 */
#define I2C_CLOCK 0x20
#define I2C_DATA 0x10
/* primary head DDC for Mystique(?), G100, G200, G400 */
#define DDC1_CLK	0x08
#define DDC1_DATA	0x02
/* primary head DDC for Millennium, Millennium II */
#define DDC1B_CLK	0x10
#define DDC1B_DATA	0x04
/* secondary head DDC for G400, G450 and G550 */
#define DDC2_CLK	0x04
#define DDC2_DATA	0x01

status_t i2c_sec_tv_adapter()
{
	status_t result = B_ERROR;

	/* The secondary DDC channel only exist on dualhead cards */
	if (!si->ps.secondary_head) return result;

	/* make sure the output lines will be active-low when enabled
	 * (they will be pulled 'passive-high' when disabled) */
	DXIW(GENIODATA,0x00);
	/* send out B_STOP condition on secondary head DDC channel and use it to
	 * check for 'shortcut', indicating the Matrox VGA->TV adapter is connected */

	/* make sure SDA is low */
	DXIW(GENIOCTRL, (DXIR(GENIOCTRL) | DDC2_DATA));
	snooze(2);
	/* make sure SCL should be high */
	DXIW(GENIOCTRL, (DXIR(GENIOCTRL) & ~DDC2_CLK));
	snooze(2);
	/* if SCL is low then the bus is blocked by a TV adapter */
	if (!(DXIR(GENIODATA) & DDC2_CLK)) result = B_OK;
	snooze(5);
	/* set SDA while SCL should be set (generates actual bus-stop condition) */
	DXIW(GENIOCTRL, (DXIR(GENIOCTRL) & ~DDC2_DATA));
	snooze(5);

	return result;
}

/*-----------------------------
 *low level hardware access
 */
#define I2C_DELAY 2
#define I2C_TIMEOUT 100
static int i2c_set_lines(int clock,int data)
{
	int count=0;
	int program;
	int required;

	/*work out which bits to zero*/
	program = 
		(clock ? 0 : I2C_CLOCK)|
		(data ? 0 : I2C_DATA);

	/*what value do I require on data lines*/
	required = 
		(clock ? I2C_CLOCK : 0);

	/*set the bits to zero*/
	DXIW(GENIOCTRL,program); /*drive these bits*/
	DXIW(GENIODATA,0x00);    /*to zero*/

	/*wait a bit*/
	delay(I2C_DELAY);

	/*loop until the clock is as required*/
	while ((DXIR(GENIODATA)&I2C_CLOCK)!=required)
	{
		delay(I2C_DELAY);
		count++;
		if (count>I2C_TIMEOUT)
		{
			LOG(8,("I2C: Timeout on set lines - clock:%d data:%d actual:%x\n",clock,data,DXIR(GENIODATA)));
			return -1;
		}
	}

	return 0;
}

static int i2c_get_data()
{
	int data;
	int clock;
	int count=0;

	do
	{
		/*read the data and clock lines*/
		data = DXIR(GENIODATA);
		clock = (data&I2C_CLOCK) ? 1 : 0;
		data = (data&I2C_DATA) ? 1 : 0;
	
		/*manage timeout*/
		count++;
		if (count>I2C_TIMEOUT)
		{
			return -1;
		}

		/*wait a bit, so not hammering bus*/
		delay(I2C_DELAY);

	}while (!clock); /*wait for high clock*/

	return data;
}


/*----------------------- 
 *Standard I2C operations
 */
static void i2c_start()
{
	int error=0;

	error+= i2c_set_lines(0,1);
	error+= i2c_set_lines(1,1);
	error+= i2c_set_lines(1,0);
	error+= i2c_set_lines(0,0);

	if (error)
	{
		LOG(8,("I2C: start - %d\n",error));
	}
}

static void i2c_stop()
{
	int error=0;

	error+= i2c_set_lines(0,0);
	error+= i2c_set_lines(1,0);
	error+= i2c_set_lines(1,1);
	error+= i2c_set_lines(0,1);

	if (error)
	{
		LOG(8,("I2C: stop - %d\n",error));
	}
}

static void i2c_high()
{
	int error=0;

	error+= i2c_set_lines(0,1);
	error+= i2c_set_lines(1,1);
	error+= i2c_set_lines(0,1);

	if (error)
	{
		LOG(8,("I2C: high - %d\n",error));
	}
}

static void i2c_low()
{
	int error=0;

	error+= i2c_set_lines(0,0);
	error+= i2c_set_lines(1,0);
	error+= i2c_set_lines(0,0);

	if (error)
	{
		LOG(8,("I2C: low - %d\n",error));
	}
}

static int i2c_get_ack()
{
	int error=0;
	int ack;
    
	error+= i2c_set_lines(0,1);
	error+= i2c_set_lines(1,1);
	ack = i2c_get_data();
	error+= i2c_set_lines(0,1);

	if (error)
	{
		LOG(8,("I2C: get_ack - %d value:%x\n",error,ack));
	}

	return ack;
}

static void i2c_send_ack()
{
	int error=0;
    
	error+= i2c_set_lines(0,0);
	error+= i2c_set_lines(1,0);
	error+= i2c_set_lines(0,0);

	if (error)
	{
		LOG(8,("I2C: send_ack - %d\n",error));
	}
}

/*------------------------------
 *use above functions to send and receive bytes
 */

static int i2c_sendbyte(unsigned char data)
{
	int i;

	for (i=7; i>=0; i--)
	{
		if (data&(1<<i)) 
		{
			i2c_high();
		}
		else
		{
			i2c_low();
		}
	}

	return i2c_get_ack();
}

static unsigned char i2c_readbyte(int ack_required)
{
	int i;
	unsigned char data=0;
    
	/*read data*/
	i2c_set_lines(0,1);
	for (i=7; i>=0; i--) 
	{
		i2c_set_lines(1,1);
		if (i2c_get_data()==1)
			data |= (1<<i);
		i2c_set_lines(0,1);
	}

	/*send acknowledge*/
	if (ack_required) i2c_send_ack();

	return data;
}

/*-------------------------------------------
 *PUBLIC functions
 */ 
int i2c_maven_read(unsigned char address)
{
	int error=0;
	int data;

	i2c_start();
	{
		error+=i2c_sendbyte(MAVEN_READ);
		error+=i2c_sendbyte(address);
		data = i2c_readbyte(0);
	}	
	i2c_stop();
	if (error>0) LOG(8,("I2C: MAVR ERROR - %x\n",error));
	return data;
}

void i2c_maven_write(unsigned char address, unsigned char data)
{
	int error=0;

	i2c_start();
	{
		error+=i2c_sendbyte(MAVEN_WRITE);
		error+=i2c_sendbyte(address);
		error+=i2c_sendbyte(data);
	}	
	i2c_stop();
	if (error>0) LOG(8,("I2C: MAVW ERROR - %x\n",error));
}

status_t i2c_init(void)
{
	/*init g400 i2c*/
	DXIW(GENIODATA,0x00); /*to zero*/
	DXIW(GENIOCTRL,0x30); /*drive clock and data*/
	DXIW(GENIOCTRL,0x00); /*stop driving*/

	return B_OK;
}

status_t i2c_maven_probe(void)
{
	int ack;

	/*scan the bus for the MAVEN*/
	i2c_start();
	{
		ack = i2c_sendbyte(MAVEN_READ);
	}
	i2c_stop();
	if (ack==0) 
	{
		return B_OK;
	}
	else
	{
		return B_ERROR;
	}
}
