/**
 * @file stb.c
 *
 * @brief High-level security tool box
 *
 * This file implements the security tool box including
 * CCM* encryption and decryption using the SAL API.
 *
 * $Id: stb.c 20253 2010-02-09 09:00:20Z sschneid $
 *
 */
/**
 * @author
 *      Atmel Corporation: http://www.atmel.com
 *      Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

#ifdef STB_ON_SAL

/* === Includes ============================================================ */

#include <string.h>
#include "tal.h"
#include "ieee_const.h"
#include "stb.h"
#include "stb_internal.h"

/* === Macros ============================================================== */

/* === Types =============================================================== */

/* === Globals ============================================================= */

/* True indicates if key has to be setup. */
static bool key_change = true;
static uint8_t last_key[AES_BLOCKSIZE];
static bool stb_restart_required = false;

/* === Implementation ====================================================== */

/**
 * @brief STB Initialization
 *
 * This function initializes the STB.
 */
void stb_init(void)
{
    sal_init();
}


/**
 * @brief STB Restart
 *
 * This function re-starts the STB after power down.
 *
 * @ingroup apiStbApi
 */
void stb_restart(void)
{
    /*
     * Re-use key_change flag indicating that the AES engine has been power-
     * cycled and needs to be restarted.
     */
    stb_restart_required = true;
}


/**
 * @brief Secure one block with CCM*
 *
 * This functions secures one block with CCM* according to 802.15.4.
 *
 * @param[in,out] buffer Input: plaintext header and payload concatenated;
 *                       for encryption: MUST HAVE 'AES_BLOCKSIZE'
 *                       BYTES SPACE AT THE END FOR THE MIC!
 *                       Output: frame secured (with MIC at end)/unsecured
 * @param[in]  nonce   The nonce: Initialization Vector (IV) as used in
 *                     cryptography; the ZigBee nonce (13 bytes long)
 *                     are the bytes 2...14 of this nonce
 * @param[in] key The key to be used; if NULL, use the current key
 * @param[in] hdr_len Length of plaintext header (will not be encrypted)
 * @param[in] pld_len Length of payload to be encrypted; if 0, then only MIC
 *                    authentication implies
 * @param[in] sec_level Security level according to IEEE 802.15.4,
 *                    7.6.2.2.1, Table 95:
 *                    - the value may be 0 ... 7;
 *                    - the two LSBs contain the MIC length in bytes
 *                      (0, 4, 8 or 16);
 *                    - bit 2 indicates whether encryption applies or not
 * @param[in] aes_dir AES_DIR_ENCRYPT if secure, AES_DIR_DECRYPT if unsecure
 *
 * @return STB CCM Status
 */
stb_ccm_t stb_ccm_secure(uint8_t *buffer,
                         uint8_t nonce[AES_BLOCKSIZE],
                                 uint8_t *key,
                         uint8_t hdr_len,
                         uint8_t pld_len,
                         uint8_t sec_level,
                         uint8_t aes_dir)
{
    uint8_t nonce_0;    /* nonce[0] for MIC computation. */
    uint8_t mic_len;
    uint8_t enc_flag;

    if (stb_restart_required)
    {
        sal_aes_restart();
        stb_restart_required = false;
    }

    if (sec_level & 3)
    {
        mic_len = 1 << ((sec_level & 3) + 1);
    }
    else
    {
        mic_len = 0;
    }

    enc_flag = sec_level & 4;

    /* Test on correct parameters. */

    if ((sec_level & ~0x7) ||
        (buffer == NULL) ||
        (nonce == NULL) ||
        ((uint16_t)pld_len + (uint16_t)hdr_len + (uint16_t)mic_len > aMaxPHYPacketSize)
       )
    {
        sal_aes_clean_up();
        return (STB_CCM_ILLPARM);
    }

    if (key_change && (key == NULL))
    {
        sal_aes_clean_up();
        return (STB_CCM_KEYMISS);   /* Initial call, but no key given. */
    }

    /* Setup key if necessary. */

    if(!key_change && key != NULL)    /* There was some previous key. */
    {
        uint8_t i;

        /* Test on changed key. */
        for (i = AES_BLOCKSIZE; i--; /* */)
        {
            key_change |= (last_key[i] ^ key[i]);
        }
    }

    if (key_change)
    {
        /*
         * Key must be non-NULL because of test above, and
         * ECB encryption is always the initial encryption mode.
         */
        sal_aes_setup(key, AES_MODE_ECB, AES_DIR_ENCRYPT);
        memcpy(last_key, key, AES_KEYSIZE);
        key_change = false;
    }

    /* Prepare nonce. */

    nonce[0] = 1;   /* Always 2 bytes for length field. */

    if (mic_len > 0)
    {
        nonce[0] |= ((mic_len - 2) >> 1) << 3;
    }

    if (hdr_len)
    {
        nonce[0] |= 1 << 6;
    }

    nonce_0 = nonce[0];
    nonce[AES_BLOCKSIZE -  2] = 0;

    if (aes_dir == AES_DIR_ENCRYPT)
    {
        /* Authenticate. */
        if (mic_len > 0)
        {
            nonce[AES_BLOCKSIZE - 1] = pld_len;

            compute_mic(buffer,
                        buffer + hdr_len + pld_len,
                        nonce,
                        hdr_len,
                        pld_len);
        }

        /* encrypt payload and MIC */
        if (enc_flag)
        {
            nonce[0] = 1;
            encrypt_pldmic(buffer + hdr_len, nonce, mic_len, pld_len);
        }
    }
    else
    {
        /* Decrypt payload and MIC. */
        if (enc_flag)
        {
            nonce[0] = 1;
            encrypt_pldmic(buffer + hdr_len, nonce, mic_len, pld_len);
        }

        /* Check MIC. */
        if (mic_len > 0)
        {
            uint8_t rcvd_mic[AES_BLOCKSIZE];      /* maximal MIC size */

            nonce[0] = nonce_0;
            nonce[AES_BLOCKSIZE - 1] = pld_len;

            compute_mic(buffer,
                        rcvd_mic,
                        nonce,
                        hdr_len,
                        pld_len);

            buffer += hdr_len + pld_len;

            if(memcmp(buffer, rcvd_mic, mic_len))
            {
                return STB_CCM_MICERR;
            }
        }
    }

    sal_aes_clean_up();
    return (STB_CCM_OK);
}


#endif /* #ifdef STB_ON_SAL */

/* EOF */
