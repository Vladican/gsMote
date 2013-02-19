/*
 * ADC.h
 *
 * Created: 2/16/2013 8:56:03 PM
 *  Author: Vlad
 */ 


#ifndef ADC_H_
#define ADC_H_

// AD7767
#define ADC_VREF 2500000  // 2.5volts in microvolts
#define ADC_MAX 0x7FFFFF  // 24-bit ADC.  2^23 because signed
#define ADC_DRIVER_GAIN_NUMERATOR 3  // gain of ADC stage
#define ADC_DRIVER_GAIN_DENOMINATOR 2

//ADC software defines
#define ADC_DISCARD 128
#define NUM_SAMPLES 1024

// Sample frequency (samples per second)
#define SPS_32_gc 0x05
#define SPS_64_gc 0x06
#define SPS_128_gc 0x07
#define SPS_256_gc 0x08
#define SPS_512_gc 0x09
#define SPS_1K_gc 0x0A
#define SPS_2K_gc 0x0B
#define SPS_4K_gc 0x0C
#define SPS_MAX_gc SPS_4K_gc

// Sample frequency (subsamples per second)
#define SSPS_SE_32_gc 0x0B
#define SSPS_SE_64_gc 0x0A
#define SSPS_SE_128_gc 0x09
#define SSPS_SE_256_gc 0x08
#define SSPS_SE_512_gc 0x07
#define SSPS_SE_1K_gc 0x06
#define SSPS_SE_2K_gc 0x05
#define SSPS_SE_4K_gc 0x04
#define SSPS_SE_8K_gc 0x03
#define SSPS_SE_16K_gc 0x02
#define SSPS_SE_32K_gc 0x01
#define SSPS_SE_64K_gc 0x00
#define SSPS_SE_MAX_gc SSPS_SE_32_gc

// ADC channels
#define ADC_CH_1_gc 0x00
#define ADC_CH_2_gc 0x01
#define ADC_CH_3_gc 0x02
#define ADC_CH_4_gc 0x03
#define ADC_CH_5_gc 0x04
#define ADC_CH_6_gc 0x05
#define ADC_CH_7_gc 0x06
#define ADC_CH_8_gc 0x07
#define ADC_SPI_CONFIG_gc 0x54

// Gain settings
#define GAIN_1_gc 0x00
#define GAIN_2_gc 0x01
#define GAIN_4_gc 0x02
#define GAIN_8_gc 0x03
#define GAIN_16_gc 0x04
#define GAIN_32_gc 0x05
#define GAIN_64_gc 0x06
#define GAIN_128_gc 0x07


// Hardware filter config
#define FILTER_CH_1AND5_bm 0x01
#define FILTER_CH_2AND6_bm 0x02
#define FILTER_CH_3AND7_bm 0x04
#define FILTER_CH_4AND8_bm 0x08
#define FILTER_HP_0_bm 0x80
#define FILTER_HP_2_bm 0x00
#define FILTER_LP_INF_gc 0x00
#define FILTER_LP_32K_gc 0x10
#define FILTER_LP_6K_gc 0x20
#define FILTER_LP_600_gc 0x40

//ADC related global vars
volatile uint8_t channelStatus;  // copy of channel filter configuration to allow bit level changes
volatile int32_t data24Bit[NUM_SAMPLES];  // storage for ADC samples
volatile uint8_t SPIBuffer[13];  // space for 24-bit ADC conversion plus dummy byte
volatile uint8_t SPICount, discardCount;
volatile int32_t *temp32;  // for parsing SPI transactions
volatile int64_t *temp64; // for parsing SPI transactions from 8bit pieces to 64bit whole
volatile int64_t sumFRAM[3];  // sum of all FRAM samples for averaging
volatile int64_t var;
volatile uint16_t sampleCount;  // sample and discard counter for array offset
volatile uint16_t TotalSampleCount;

//ADC sampling functions
void CO_collectTemp(uint16_t *avgV, uint16_t *minV, uint16_t *maxV);
void CO_collectBatt(uint16_t *avgV, uint16_t *minV, uint16_t *maxV);
void CO_collectSP(uint8_t channel, int32_t *averageV, int32_t *minV,
int32_t *maxV, uint8_t gainExponent);
void CO_collectADC(uint8_t channel, uint8_t filterConfig, int32_t *avgV, int32_t *minV,
int32_t *maxV, uint8_t gainExponent, uint8_t spsExponent);
//collect ADC data and send it over the radio every 128 samples
void CO_collectADC_cont(uint8_t channel, uint8_t filterConfig, uint8_t gainExponent, uint8_t spsExponent);
void CO_collectSeismic3Channel(uint8_t filterConfig, uint8_t gain[], uint8_t subsamplesPerSecond,
uint8_t subsamplesPerChannel, uint8_t DCPassEnable, uint16_t averagingPtA, uint16_t averagingPtB,
uint16_t averagingPtC, uint16_t averagingPtD);
void CO_collectSeismic1Channel(uint8_t channel, uint8_t filterConfig, uint8_t gain,
uint8_t subsamplesPerSecond, uint8_t subsamplesPerSample, uint8_t DCPassEnable,
uint16_t averagingPtA, uint16_t averagingPtB, uint16_t averagingPtC,
uint16_t averagingPtD);
void CO_collectSeismic3Channel_continuous(uint8_t filterConfig, uint8_t gain[], uint8_t subsamplesPerSecond,
uint8_t subsamplesPerChannel, uint8_t DCPassEnable, uint16_t averagingPtA, uint16_t averagingPtB,
uint16_t averagingPtC, uint16_t averagingPtD);
void sampleCurrentChannel();
void writeSE2FRAM();

//ADC config functions
void ADCPower(uint8_t on);
void set_filter(uint8_t filterConfig);
void setADCInput(uint8_t channel);
void enableADCMUX(uint8_t on);
void set_ampGain(uint8_t channel, uint8_t gainExponent);
void ACC_DCPassEnable(uint8_t enable);
void ADC_Pause_Sampling();
void ADC_Resume_Sampling();


#endif /* ADC_H_ */