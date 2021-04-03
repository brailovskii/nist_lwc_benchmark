//
// NIST-developed software is provided by NIST as a public service.
// You may use, copy and distribute copies of the software in any medium,
// provided that you keep intact this entire notice. You may improve, 
// modify and create derivative works of the software or any portion of
// the software, and you may copy and distribute such modifications or
// works. Modified works should carry a notice stating that you changed
// the software and should note the date and nature of any such change.
// Please explicitly acknowledge the National Institute of Standards and 
// Technology as the source of the software.
//
// NIST-developed software is expressly provided "AS IS." NIST MAKES NO 
// WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION
// OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT AND DATA ACCURACY. NIST
// NEITHER REPRESENTS NOR WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE 
// UNINTERRUPTED OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST 
// DOES NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE SOFTWARE
// OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE CORRECTNESS, ACCURACY,
// RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
//
// You are solely responsible for determining the appropriateness of using and 
// distributing the software and you assume all risks associated with its use, 
// including but not limited to the risks and costs of program errors, compliance 
// with applicable laws, damage to or loss of data, programs or equipment, and 
// the unavailability or interruption of operation. This software is not intended
// to be used in any situation where a failure could cause risk of injury or 
// damage to property. The software developed by NIST employees is not subject to
// copyright protection within the United States.
//

// disable deprecation for sprintf and fopen
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include "crypto_aead.h"
#include "crypto_hash.h"
#include <stdarg.h>
#include "lwc_config.h"
#include "lwc_api.h"



#define KAT_SUCCESS          0
#define KAT_FILE_OPEN_ERROR -1
#define KAT_DATA_ERROR      -3
#define KAT_CRYPTO_FAILURE  -4

#define MAX_FILE_NAME				256



void tick_msr_start(void);
uint32_t tick_msr_end(void);
void lwc_printf(const char* format, ...);


void init_buffer(unsigned char *buffer, unsigned long long numbytes);
int aead_generate_test_vectors();
int hash_generate_test_vectors();




static uint32_t gb_tick_cnt;
char algo_name_str[128] = ALGO_NAME_STR;



int genkat_benchmark_hash_aead(void)
{
#ifdef LWC_ALGO_AEAD
	int ret = aead_generate_test_vectors();
#endif

#ifdef LWC_ALGO_HASH
	int ret = hash_generate_test_vectors();
#endif

	return ret;
}



#ifdef LWC_ALGO_AEAD

#define MAX_MESSAGE_LENGTH			32
#define MAX_ASSOCIATED_DATA_LENGTH	32


int aead_generate_test_vectors()
{
	unsigned char       key[CRYPTO_KEYBYTES];
	unsigned char		nonce[CRYPTO_NPUBBYTES];
	unsigned char       msg[MAX_MESSAGE_LENGTH];
	unsigned char       msg2[MAX_MESSAGE_LENGTH];
	unsigned char		ad[MAX_ASSOCIATED_DATA_LENGTH];
	unsigned char		ct[MAX_MESSAGE_LENGTH + CRYPTO_ABYTES];
	unsigned long long  clen, mlen2;
	int                 func_ret = 0, ret_val = KAT_SUCCESS;


	init_buffer(key, sizeof(key));
	init_buffer(nonce, sizeof(nonce));
	init_buffer(msg, sizeof(msg));
	init_buffer(ad, sizeof(ad));


	lwc_printf("\n\n\n\nStarting...\nOptimization: %s\nAlgorithm: %s\n", OPTIMIZATION_LEVEL, algo_name_str );

	for (unsigned long long mlen = 0; (mlen <= MAX_MESSAGE_LENGTH) && (ret_val == KAT_SUCCESS); mlen += 8) {

		for (unsigned long long adlen = 0; adlen <= MAX_ASSOCIATED_DATA_LENGTH; adlen += 8) {

			lwc_printf("msg_len:%02d ad_len:%02d  ", (int)mlen, (int)adlen);

			tick_msr_start();
			func_ret = crypto_aead_encrypt(ct, &clen, msg, mlen, ad, adlen, NULL, nonce, key);
			tick_msr_end();

			lwc_printf( "enc:%08lu us:%08lu ", gb_tick_cnt, gb_tick_cnt/16);

			if ( func_ret != 0) {
				ret_val = KAT_CRYPTO_FAILURE;
				break;
			}

			tick_msr_start();
			func_ret = crypto_aead_decrypt(msg2, &mlen2, NULL, ct, clen, ad, adlen, nonce, key);
			tick_msr_end();

			lwc_printf( "dec:%08lu us:%08lu \n", gb_tick_cnt, gb_tick_cnt/16);

			if ( (func_ret != 0) || (mlen != mlen2) || (memcmp(msg, msg2, mlen) != 0) ) {
				ret_val = KAT_CRYPTO_FAILURE;
				break;
			}

		}
	}

	if(ret_val !=0 ){
		lwc_printf( "Error occurred\n");
	}

	return ret_val;
}
#endif

#ifdef LWC_ALGO_HASH

#define MAX_MESSAGE_LENGTH			1024


int hash_generate_test_vectors(){


	unsigned char       msg[MAX_MESSAGE_LENGTH];
	unsigned char		digest[CRYPTO_BYTES];
	int                 ret_val = KAT_SUCCESS;

	init_buffer(msg, sizeof(msg));

	lwc_printf("\n\n\n\nStarting...\nOptimization: %s\nAlgorithm: %s\n", OPTIMIZATION_LEVEL, algo_name_str );

	for (unsigned long long mlen = 16; mlen <= MAX_MESSAGE_LENGTH; mlen += 256) {


		lwc_printf("msg_len:%04d ", (int)mlen);

		tick_msr_start();
		ret_val = crypto_hash(digest, msg, mlen);
		tick_msr_end();

		if(ret_val == 0) {
			lwc_printf( "hash:%08lu us:%08lu \n", gb_tick_cnt, gb_tick_cnt/16);
		}else{
			ret_val = KAT_CRYPTO_FAILURE;
			break;
		}
	}

	return ret_val;
}

#endif




void init_buffer(unsigned char *buffer, unsigned long long numbytes)
{
	for (unsigned long long i = 0; i < numbytes; i++)
		buffer[i] = (unsigned char)i + '0';
}






#define PLATFORM_CORTEX_M

#ifdef PLATFORM_CORTEX_M


#ifdef STM32F303xC
#include "stm32f3xx_hal.h"
#endif
#ifdef STM32F051x8
#include "stm32f0xx_hal.h"
#endif

uint32_t tim_per_elp_cnt = 0;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart1;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	tim_per_elp_cnt++;
}

void tick_msr_start(void){

	tim_per_elp_cnt = 0;
	htim6.Instance->CNT = 0;
	HAL_TIM_Base_Start_IT(&htim6);
}

uint32_t tick_msr_end(void){

	HAL_TIM_Base_Stop_IT(&htim6);
	gb_tick_cnt = tim_per_elp_cnt<<16 | htim6.Instance->CNT;
	return gb_tick_cnt;
}

void lwc_printf(const char* format, ...){

	static char dbg_out_buf[512];
	va_list args;

	va_start(args,format);
	vsprintf (dbg_out_buf, format, args );
	va_end(args);

	HAL_UART_Transmit(&huart1, (uint8_t *)dbg_out_buf, strlen(dbg_out_buf), 100);
}

#endif


